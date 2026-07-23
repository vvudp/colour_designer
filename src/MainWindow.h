#pragma once

#include <QMainWindow>

class ControlPanel;
class PreviewGLWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void exportImage();
    void savePalette();
    void loadPalette();

    PreviewGLWidget *preview_ {nullptr};
    ControlPanel *controlPanel_ {nullptr};
};
