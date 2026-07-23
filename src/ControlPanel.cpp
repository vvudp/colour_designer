#include "ControlPanel.h"

#include "ColorPickerWidget.h"

#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
QFrame *makeSection(QWidget *parent, const QString &title, QLayout *content)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("sectionCard"));
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    auto *label = new QLabel(title, frame);
    label->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(label);
    layout->addLayout(content);
    return frame;
}

} // namespace

ControlPanel::ControlPanel(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(420);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(12, 12, 16, 18);
    contentLayout->setSpacing(12);

    auto *header = new QLabel(QStringLiteral("机器配色设计器"), content);
    header->setObjectName(QStringLiteral("panelHeader"));
    auto *subHeader = new QLabel(
        QStringLiteral("选择上部或下部机壳，然后拖动调色盘。左侧预览会实时保留原始光影。"),
        content);
    subHeader->setObjectName(QStringLiteral("panelSubHeader"));
    subHeader->setWordWrap(true);
    contentLayout->addWidget(header);
    contentLayout->addWidget(subHeader);

    upperButton_ = new QToolButton(content);
    lowerButton_ = new QToolButton(content);
    for (QToolButton *button : {upperButton_, lowerButton_}) {
        button->setCheckable(true);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setMinimumHeight(44);
    }
    upperButton_->setText(QStringLiteral("上部机壳"));
    lowerButton_->setText(QStringLiteral("下部机壳"));
    upperButton_->setChecked(true);

    auto *regionGroup = new QButtonGroup(this);
    regionGroup->setExclusive(true);
    regionGroup->addButton(upperButton_);
    regionGroup->addButton(lowerButton_);

    auto *regionLayout = new QHBoxLayout;
    regionLayout->addWidget(upperButton_);
    regionLayout->addWidget(lowerButton_);
    contentLayout->addWidget(makeSection(content, QStringLiteral("1. 选择着色区域"), regionLayout));

    picker_ = new ColorPickerWidget(content);
    auto *pickerLayout = new QVBoxLayout;
    pickerLayout->addWidget(picker_);
    contentLayout->addWidget(makeSection(content, QStringLiteral("2. 选择颜色"), pickerLayout));

    matteRow_ = createSliderRow(QStringLiteral("哑光程度"), 0, 100, 88, QStringLiteral("%"));
    highlightRow_ = createSliderRow(QStringLiteral("高光保留"), 0, 100, 22, QStringLiteral("%"));
    shadowRow_ = createSliderRow(QStringLiteral("阴影深度"), 0, 200, 100, QStringLiteral("%"));
    brightnessRow_ = createSliderRow(QStringLiteral("整体明度"), 35, 165, 100, QStringLiteral("%"));

    auto *materialLayout = new QVBoxLayout;
    auto addSlider = [materialLayout](const QString &name, const SliderRow &row) {
        auto *titleRow = new QHBoxLayout;
        titleRow->addWidget(new QLabel(name));
        titleRow->addStretch();
        titleRow->addWidget(row.valueLabel);
        materialLayout->addLayout(titleRow);
        materialLayout->addWidget(row.slider);
    };
    addSlider(QStringLiteral("哑光程度"), matteRow_);
    addSlider(QStringLiteral("高光保留"), highlightRow_);
    addSlider(QStringLiteral("阴影深度"), shadowRow_);
    addSlider(QStringLiteral("整体明度"), brightnessRow_);
    contentLayout->addWidget(makeSection(content, QStringLiteral("3. 材质与光影"), materialLayout));

    const QList<QColor> presets {
        QColor(QStringLiteral("#D2D6D8")),
        QColor(QStringLiteral("#AEB8BE")),
        QColor(QStringLiteral("#83B6C9")),
        QColor(QStringLiteral("#709CB8")),
        QColor(QStringLiteral("#7B82A9")),
        QColor(QStringLiteral("#587A70")),
        QColor(QStringLiteral("#3E474D")),
        QColor(QStringLiteral("#D6C9B7"))
    };
    auto *presetLayout = new QGridLayout;
    presetLayout->setSpacing(8);
    for (int i = 0; i < presets.size(); ++i) {
        auto *button = new QToolButton(content);
        button->setFixedSize(42, 32);
        button->setToolTip(presets[i].name(QColor::HexRgb).toUpper());
        button->setStyleSheet(QStringLiteral(
            "QToolButton { background: %1; border: 1px solid #AAB2BC; border-radius: 7px; }"
            "QToolButton:hover { border: 2px solid #2D6A8A; }"
        ).arg(presets[i].name(QColor::HexRgb)));
        connect(button, &QToolButton::clicked, this, [this, color = presets[i]]() {
            picker_->setColor(color);
            if (region_ == Region::Upper) {
                state_.upperColor = color;
            } else {
                state_.lowerColor = color;
            }
            emitStateAndScheduleHistory();
        });
        presetLayout->addWidget(button, i / 4, i % 4);
    }
    contentLayout->addWidget(makeSection(content, QStringLiteral("常用工业配色"), presetLayout));

    undoButton_ = new QPushButton(QStringLiteral("撤销"), content);
    redoButton_ = new QPushButton(QStringLiteral("重做"), content);
    auto *resetButton = new QPushButton(QStringLiteral("恢复默认"), content);
    auto *compareButton = new QPushButton(QStringLiteral("按住查看原图"), content);
    auto *saveButton = new QPushButton(QStringLiteral("保存方案"), content);
    auto *loadButton = new QPushButton(QStringLiteral("载入方案"), content);
    auto *exportButton = new QPushButton(QStringLiteral("导出带参数 PNG"), content);
    exportButton->setObjectName(QStringLiteral("primaryButton"));

    auto *actionLayout = new QGridLayout;
    actionLayout->addWidget(undoButton_, 0, 0);
    actionLayout->addWidget(redoButton_, 0, 1);
    actionLayout->addWidget(resetButton, 0, 2);
    actionLayout->addWidget(compareButton, 1, 0, 1, 3);
    actionLayout->addWidget(saveButton, 2, 0);
    actionLayout->addWidget(loadButton, 2, 1);
    actionLayout->addWidget(exportButton, 2, 2);
    contentLayout->addWidget(makeSection(content, QStringLiteral("方案与导出"), actionLayout));
    contentLayout->addStretch();

    scroll->setWidget(content);
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(scroll);

    historyTimer_ = new QTimer(this);
    historyTimer_->setSingleShot(true);
    historyTimer_->setInterval(220);

    connect(upperButton_, &QToolButton::clicked, this, [this]() {
        selectRegion(Region::Upper);
    });
    connect(lowerButton_, &QToolButton::clicked, this, [this]() {
        selectRegion(Region::Lower);
    });
    connect(picker_, &ColorPickerWidget::colorChanged, this, [this](const QColor &color) {
        if (updating_) {
            return;
        }
        if (region_ == Region::Upper) {
            state_.upperColor = color;
        } else {
            state_.lowerColor = color;
        }
        emitStateAndScheduleHistory();
    });

    for (QSlider *slider : {
        matteRow_.slider,
        highlightRow_.slider,
        shadowRow_.slider,
        brightnessRow_.slider
    }) {
        connect(slider, &QSlider::valueChanged, this, [this](int) {
            applyMaterialEditors();
        });
    }

    connect(historyTimer_, &QTimer::timeout, this, &ControlPanel::commitHistory);
    connect(undoButton_, &QPushButton::clicked, this, [this]() {
        restoreHistoryIndex(historyIndex_ - 1);
    });
    connect(redoButton_, &QPushButton::clicked, this, [this]() {
        restoreHistoryIndex(historyIndex_ + 1);
    });
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        setState(RecolorState{}, true);
    });
    connect(compareButton, &QPushButton::pressed, this, [this]() {
        state_.showOriginal = true;
        emit stateChanged(state_);
    });
    connect(compareButton, &QPushButton::released, this, [this]() {
        state_.showOriginal = false;
        emit stateChanged(state_);
    });
    connect(saveButton, &QPushButton::clicked, this, &ControlPanel::savePaletteRequested);
    connect(loadButton, &QPushButton::clicked, this, &ControlPanel::loadPaletteRequested);
    connect(exportButton, &QPushButton::clicked, this, &ControlPanel::exportRequested);

    setState(state_, true);
}

void ControlPanel::setState(const RecolorState &state, bool addToHistory)
{
    updating_ = true;
    state_ = state;
    state_.showOriginal = false;
    updatePickerFromRegion();
    updateMaterialEditors();
    updating_ = false;

    emit stateChanged(state_);

    if (addToHistory) {
        if (historyTimer_->isActive()) {
            historyTimer_->stop();
        }
        commitHistory();
    }
}

ControlPanel::SliderRow ControlPanel::createSliderRow(
    const QString &,
    int minimum,
    int maximum,
    int value,
    const QString &suffix)
{
    SliderRow row;
    row.slider = new QSlider(Qt::Horizontal, this);
    row.slider->setRange(minimum, maximum);
    row.slider->setValue(value);
    row.valueLabel = new QLabel(QStringLiteral("%1%2").arg(value).arg(suffix), this);
    row.valueLabel->setMinimumWidth(50);
    row.valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    connect(row.slider, &QSlider::valueChanged, this,
            [label = row.valueLabel, suffix](int newValue) {
        label->setText(QStringLiteral("%1%2").arg(newValue).arg(suffix));
    });
    return row;
}

void ControlPanel::selectRegion(Region region)
{
    region_ = region;
    upperButton_->setChecked(region == Region::Upper);
    lowerButton_->setChecked(region == Region::Lower);
    updatePickerFromRegion();
}

void ControlPanel::updatePickerFromRegion()
{
    const bool wasUpdating = updating_;
    updating_ = true;
    picker_->setColor(region_ == Region::Upper ? state_.upperColor : state_.lowerColor);
    updating_ = wasUpdating;
}

void ControlPanel::updateMaterialEditors()
{
    matteRow_.slider->setValue(qRound(state_.matte * 100.0F));
    highlightRow_.slider->setValue(qRound(state_.highlight * 100.0F));
    shadowRow_.slider->setValue(qRound(state_.shadow * 100.0F));
    brightnessRow_.slider->setValue(qRound(state_.brightness * 100.0F));
}

void ControlPanel::applyMaterialEditors()
{
    if (updating_) {
        return;
    }
    state_.matte = matteRow_.slider->value() / 100.0F;
    state_.highlight = highlightRow_.slider->value() / 100.0F;
    state_.shadow = shadowRow_.slider->value() / 100.0F;
    state_.brightness = brightnessRow_.slider->value() / 100.0F;
    emitStateAndScheduleHistory();
}

void ControlPanel::emitStateAndScheduleHistory()
{
    state_.showOriginal = false;
    emit stateChanged(state_);
    historyTimer_->start();
}

void ControlPanel::commitHistory()
{
    RecolorState clean = state_;
    clean.showOriginal = false;

    if (historyIndex_ >= 0
        && history_[historyIndex_].approximatelyEquals(clean)) {
        updateHistoryButtons();
        return;
    }

    while (history_.size() > historyIndex_ + 1) {
        history_.removeLast();
    }
    history_.append(clean);
    historyIndex_ = history_.size() - 1;
    updateHistoryButtons();
}

void ControlPanel::restoreHistoryIndex(int index)
{
    if (index < 0 || index >= history_.size()) {
        return;
    }
    historyIndex_ = index;
    updating_ = true;
    state_ = history_[historyIndex_];
    updatePickerFromRegion();
    updateMaterialEditors();
    updating_ = false;
    emit stateChanged(state_);
    updateHistoryButtons();
}

void ControlPanel::updateHistoryButtons()
{
    undoButton_->setEnabled(historyIndex_ > 0);
    redoButton_->setEnabled(historyIndex_ >= 0 && historyIndex_ + 1 < history_.size());
}
