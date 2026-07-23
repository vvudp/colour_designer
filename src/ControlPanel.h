#pragma once

#include "RecolorState.h"

#include <QVector>
#include <QWidget>

class ColorPickerWidget;
class QLabel;
class QPushButton;
class QSlider;
class QTimer;
class QToolButton;

class ControlPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = nullptr);

    [[nodiscard]] RecolorState state() const { return state_; }
    void setState(const RecolorState &state, bool addToHistory = true);

signals:
    void stateChanged(const RecolorState &state);
    void exportRequested();
    void savePaletteRequested();
    void loadPaletteRequested();

private:
    enum class Region { Upper, Lower };

    struct SliderRow
    {
        QSlider *slider {nullptr};
        QLabel *valueLabel {nullptr};
    };

    SliderRow createSliderRow(
        const QString &title,
        int minimum,
        int maximum,
        int value,
        const QString &suffix = QString()
    );

    void selectRegion(Region region);
    void updatePickerFromRegion();
    void updateMaterialEditors();
    void applyMaterialEditors();
    void emitStateAndScheduleHistory();
    void commitHistory();
    void restoreHistoryIndex(int index);
    void updateHistoryButtons();

    RecolorState state_;
    Region region_ {Region::Upper};
    bool updating_ {false};

    ColorPickerWidget *picker_ {nullptr};
    QToolButton *upperButton_ {nullptr};
    QToolButton *lowerButton_ {nullptr};
    SliderRow matteRow_;
    SliderRow highlightRow_;
    SliderRow shadowRow_;
    SliderRow brightnessRow_;
    QPushButton *undoButton_ {nullptr};
    QPushButton *redoButton_ {nullptr};
    QTimer *historyTimer_ {nullptr};
    QVector<RecolorState> history_;
    int historyIndex_ {-1};
};
