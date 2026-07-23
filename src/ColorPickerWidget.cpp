#include "ColorPickerWidget.h"

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>

SaturationValuePicker::SaturationValuePicker(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 250);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::CrossCursor);
}

void SaturationValuePicker::setHue(int hue)
{
    hue_ = std::clamp(hue, 0, 359);
    rebuildGradient();
    update();
}

void SaturationValuePicker::setSaturationValue(int saturation, int value)
{
    saturation_ = std::clamp(saturation, 0, 255);
    value_ = std::clamp(value, 0, 255);
    update();
}

void SaturationValuePicker::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawImage(rect(), gradient_);

    const qreal x = (static_cast<qreal>(saturation_) / 255.0) * (width() - 1);
    const qreal y = (1.0 - static_cast<qreal>(value_) / 255.0) * (height() - 1);

    painter.setPen(QPen(Qt::white, 3.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(x, y), 10.0, 10.0);
    painter.setPen(QPen(QColor(0, 0, 0, 150), 1.0));
    painter.drawEllipse(QPointF(x, y), 12.0, 12.0);
}

void SaturationValuePicker::resizeEvent(QResizeEvent *)
{
    rebuildGradient();
}

void SaturationValuePicker::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        updateFromPosition(event->position());
    }
}

void SaturationValuePicker::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::LeftButton)) {
        updateFromPosition(event->position());
    }
}

void SaturationValuePicker::rebuildGradient()
{
    if (width() <= 0 || height() <= 0) {
        return;
    }

    gradient_ = QImage(width(), height(), QImage::Format_RGB32);
    for (int y = 0; y < height(); ++y) {
        const int value = qRound(255.0 * (1.0 - static_cast<double>(y) / std::max(1, height() - 1)));
        auto *line = reinterpret_cast<QRgb *>(gradient_.scanLine(y));
        for (int x = 0; x < width(); ++x) {
            const int saturation = qRound(255.0 * static_cast<double>(x) / std::max(1, width() - 1));
            line[x] = QColor::fromHsv(hue_, saturation, value).rgb();
        }
    }
}

void SaturationValuePicker::updateFromPosition(const QPointF &position)
{
    const qreal x = std::clamp(position.x(), 0.0, static_cast<qreal>(width() - 1));
    const qreal y = std::clamp(position.y(), 0.0, static_cast<qreal>(height() - 1));
    saturation_ = qRound(255.0 * x / std::max(1, width() - 1));
    value_ = qRound(255.0 * (1.0 - y / std::max(1, height() - 1)));
    update();
    emit saturationValueChanged(saturation_, value_);
}

HueSlider::HueSlider(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(34);
    setCursor(Qt::PointingHandCursor);
}

void HueSlider::setHue(int hue)
{
    hue_ = std::clamp(hue, 0, 359);
    update();
}

void HueSlider::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient gradient(0, 0, width(), 0);
    for (int hue = 0; hue <= 360; hue += 30) {
        gradient.setColorAt(static_cast<qreal>(hue) / 360.0, QColor::fromHsv(hue % 360, 255, 255));
    }

    const QRectF barRect(8.0, 10.0, std::max(1, width() - 16), 14.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawRoundedRect(barRect, 7.0, 7.0);

    const qreal x = 8.0 + (static_cast<qreal>(hue_) / 359.0) * (width() - 16);
    painter.setBrush(QColor::fromHsv(hue_, 255, 255));
    painter.setPen(QPen(Qt::white, 3.0));
    painter.drawEllipse(QPointF(x, 17.0), 10.0, 10.0);
    painter.setPen(QPen(QColor(0, 0, 0, 120), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(x, 17.0), 12.0, 12.0);
}

void HueSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        updateFromPosition(event->position());
    }
}

void HueSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::LeftButton)) {
        updateFromPosition(event->position());
    }
}

void HueSlider::updateFromPosition(const QPointF &position)
{
    const qreal x = std::clamp(position.x(), 8.0, static_cast<qreal>(std::max(8, width() - 8)));
    hue_ = qRound(359.0 * (x - 8.0) / std::max(1, width() - 16));
    update();
    emit hueChanged(hue_);
}

ColorPickerWidget::ColorPickerWidget(QWidget *parent)
    : QWidget(parent)
{
    svPicker_ = new SaturationValuePicker(this);
    hueSlider_ = new HueSlider(this);

    swatch_ = new QWidget(this);
    swatch_->setFixedSize(52, 34);
    swatch_->setObjectName(QStringLiteral("currentColorSwatch"));

    hexEdit_ = new QLineEdit(this);
    hexEdit_->setMaxLength(7);
    hexEdit_->setAlignment(Qt::AlignCenter);
    hexEdit_->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^#[0-9A-Fa-f]{6}$")),
        hexEdit_));

    auto makeByteSpin = [this]() {
        auto *spin = new QSpinBox(this);
        spin->setRange(0, 255);
        spin->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        return spin;
    };

    redSpin_ = makeByteSpin();
    greenSpin_ = makeByteSpin();
    blueSpin_ = makeByteSpin();
    hueSpin_ = new QSpinBox(this);
    hueSpin_->setRange(0, 359);
    hueSpin_->setSuffix(QStringLiteral("°"));
    saturationSpin_ = makeByteSpin();
    valueSpin_ = makeByteSpin();
    hslLabel_ = new QLabel(this);
    hslLabel_->setAlignment(Qt::AlignCenter);

    auto *hexRow = new QHBoxLayout;
    hexRow->setContentsMargins(0, 0, 0, 0);
    hexRow->addWidget(new QLabel(QStringLiteral("当前颜色"), this));
    hexRow->addWidget(swatch_);
    hexRow->addSpacing(8);
    hexRow->addWidget(new QLabel(QStringLiteral("HEX"), this));
    hexRow->addWidget(hexEdit_, 1);

    auto *valuesGrid = new QGridLayout;
    valuesGrid->setHorizontalSpacing(8);
    valuesGrid->setVerticalSpacing(6);
    valuesGrid->addWidget(new QLabel(QStringLiteral("R"), this), 0, 0);
    valuesGrid->addWidget(redSpin_, 0, 1);
    valuesGrid->addWidget(new QLabel(QStringLiteral("G"), this), 0, 2);
    valuesGrid->addWidget(greenSpin_, 0, 3);
    valuesGrid->addWidget(new QLabel(QStringLiteral("B"), this), 0, 4);
    valuesGrid->addWidget(blueSpin_, 0, 5);
    valuesGrid->addWidget(new QLabel(QStringLiteral("H"), this), 1, 0);
    valuesGrid->addWidget(hueSpin_, 1, 1);
    valuesGrid->addWidget(new QLabel(QStringLiteral("S"), this), 1, 2);
    valuesGrid->addWidget(saturationSpin_, 1, 3);
    valuesGrid->addWidget(new QLabel(QStringLiteral("V"), this), 1, 4);
    valuesGrid->addWidget(valueSpin_, 1, 5);
    valuesGrid->addWidget(hslLabel_, 2, 0, 1, 6);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(svPicker_, 1);
    layout->addWidget(hueSlider_);
    layout->addLayout(hexRow);
    layout->addLayout(valuesGrid);

    connect(svPicker_, &SaturationValuePicker::saturationValueChanged,
            this, [this](int saturation, int value) {
        applyHsv(color_.hsvHue() < 0 ? 0 : color_.hsvHue(), saturation, value, true);
    });
    connect(hueSlider_, &HueSlider::hueChanged, this, [this](int hue) {
        applyHsv(hue, color_.hsvSaturation(), color_.value(), true);
    });

    connect(hexEdit_, &QLineEdit::editingFinished, this, [this]() {
        if (updating_) {
            return;
        }
        const QColor parsed(hexEdit_->text());
        if (parsed.isValid()) {
            setColor(parsed);
            emit colorChanged(color_);
        } else {
            updateEditors();
        }
    });

    for (QSpinBox *spin : {redSpin_, greenSpin_, blueSpin_}) {
        connect(spin, &QSpinBox::valueChanged, this, [this](int) {
            updateFromRgbEditors();
        });
    }
    for (QSpinBox *spin : {hueSpin_, saturationSpin_, valueSpin_}) {
        connect(spin, &QSpinBox::valueChanged, this, [this](int) {
            updateFromHsvEditors();
        });
    }

    setColor(color_);
}

void ColorPickerWidget::setColor(const QColor &color)
{
    if (!color.isValid()) {
        return;
    }
    color_ = color.toRgb();
    updateEditors();
}

void ColorPickerWidget::applyHsv(
    int hue,
    int saturation,
    int value,
    bool emitSignal)
{
    color_ = QColor::fromHsv(
        std::clamp(hue, 0, 359),
        std::clamp(saturation, 0, 255),
        std::clamp(value, 0, 255));
    updateEditors();
    if (emitSignal) {
        emit colorChanged(color_);
    }
}

void ColorPickerWidget::updateEditors()
{
    updating_ = true;

    int hue = 0;
    int saturation = 0;
    int value = 0;
    color_.getHsv(&hue, &saturation, &value);
    if (hue < 0) {
        hue = 0;
    }

    svPicker_->setHue(hue);
    svPicker_->setSaturationValue(saturation, value);
    hueSlider_->setHue(hue);

    swatch_->setStyleSheet(QStringLiteral(
        "#currentColorSwatch { background: %1; border: 1px solid #A9B1BA; border-radius: 7px; }"
    ).arg(color_.name(QColor::HexRgb)));
    hexEdit_->setText(color_.name(QColor::HexRgb).toUpper());
    redSpin_->setValue(color_.red());
    greenSpin_->setValue(color_.green());
    blueSpin_->setValue(color_.blue());
    hueSpin_->setValue(hue);
    saturationSpin_->setValue(saturation);
    valueSpin_->setValue(value);

    int hslHue = 0;
    int hslSaturation = 0;
    int lightness = 0;
    color_.getHsl(&hslHue, &hslSaturation, &lightness);
    if (hslHue < 0) {
        hslHue = 0;
    }
    hslLabel_->setText(QStringLiteral("HSL  %1°, %2%, %3%")
        .arg(hslHue)
        .arg(qRound(hslSaturation * 100.0 / 255.0))
        .arg(qRound(lightness * 100.0 / 255.0)));

    updating_ = false;
}

void ColorPickerWidget::updateFromRgbEditors()
{
    if (updating_) {
        return;
    }
    color_ = QColor(redSpin_->value(), greenSpin_->value(), blueSpin_->value());
    updateEditors();
    emit colorChanged(color_);
}

void ColorPickerWidget::updateFromHsvEditors()
{
    if (updating_) {
        return;
    }
    applyHsv(hueSpin_->value(), saturationSpin_->value(), valueSpin_->value(), true);
}
