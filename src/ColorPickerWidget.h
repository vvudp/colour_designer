#pragma once

#include <QColor>
#include <QImage>
#include <QPointF>
#include <QWidget>

class QLabel;
class QLineEdit;
class QSpinBox;

class SaturationValuePicker final : public QWidget
{
    Q_OBJECT

public:
    explicit SaturationValuePicker(QWidget *parent = nullptr);

    void setHue(int hue);
    void setSaturationValue(int saturation, int value);

signals:
    void saturationValueChanged(int saturation, int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void rebuildGradient();
    void updateFromPosition(const QPointF &position);

    int hue_ {0};
    int saturation_ {200};
    int value_ {235};
    QImage gradient_;
};

class HueSlider final : public QWidget
{
    Q_OBJECT

public:
    explicit HueSlider(QWidget *parent = nullptr);

    void setHue(int hue);

signals:
    void hueChanged(int hue);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void updateFromPosition(const QPointF &position);

    int hue_ {0};
};

class ColorPickerWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPickerWidget(QWidget *parent = nullptr);

    [[nodiscard]] QColor color() const { return color_; }
    void setColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);

private:
    void applyHsv(int hue, int saturation, int value, bool emitSignal);
    void updateEditors();
    void updateFromRgbEditors();
    void updateFromHsvEditors();

    QColor color_ {QStringLiteral("#EB4034")};
    bool updating_ {false};

    SaturationValuePicker *svPicker_ {nullptr};
    HueSlider *hueSlider_ {nullptr};
    QWidget *swatch_ {nullptr};
    QLineEdit *hexEdit_ {nullptr};
    QSpinBox *redSpin_ {nullptr};
    QSpinBox *greenSpin_ {nullptr};
    QSpinBox *blueSpin_ {nullptr};
    QSpinBox *hueSpin_ {nullptr};
    QSpinBox *saturationSpin_ {nullptr};
    QSpinBox *valueSpin_ {nullptr};
    QLabel *hslLabel_ {nullptr};
};
