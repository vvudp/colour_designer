#include "MainWindow.h"

#include "ControlPanel.h"
#include "PaletteSerializer.h"
#include "PreviewGLWidget.h"

#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Machine Color Designer - 机器配色设计器"));
    setMinimumSize(1120, 720);
    resize(1380, 900);

    preview_ = new PreviewGLWidget(this);
    controlPanel_ = new ControlPanel(this);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(preview_);
    splitter->addWidget(controlPanel_);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({880, 500});
    setCentralWidget(splitter);

    connect(controlPanel_, &ControlPanel::stateChanged,
            preview_, &PreviewGLWidget::setState);
    connect(controlPanel_, &ControlPanel::exportRequested,
            this, &MainWindow::exportImage);
    connect(controlPanel_, &ControlPanel::savePaletteRequested,
            this, &MainWindow::savePalette);
    connect(controlPanel_, &ControlPanel::loadPaletteRequested,
            this, &MainWindow::loadPalette);

    auto *fileMenu = menuBar()->addMenu(QStringLiteral("文件"));
    auto *exportAction = fileMenu->addAction(QStringLiteral("导出 PNG…"));
    auto *saveAction = fileMenu->addAction(QStringLiteral("保存配色方案…"));
    auto *loadAction = fileMenu->addAction(QStringLiteral("载入配色方案…"));
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(QStringLiteral("退出"));

    auto *viewMenu = menuBar()->addMenu(QStringLiteral("视图"));
    auto *resetViewAction = viewMenu->addAction(QStringLiteral("恢复适配视图"));

    connect(exportAction, &QAction::triggered, this, &MainWindow::exportImage);
    connect(saveAction, &QAction::triggered, this, &MainWindow::savePalette);
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadPalette);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    connect(resetViewAction, &QAction::triggered, preview_, &PreviewGLWidget::resetView);

    statusBar()->showMessage(QStringLiteral("就绪"));
    preview_->setState(controlPanel_->state());
}

void MainWindow::exportImage()
{
    if (!preview_->recolorer().isValid()) {
        QMessageBox::critical(this,
                              QStringLiteral("导出失败"),
                              preview_->recolorer().errorString());
        return;
    }

    const QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出配色效果图"),
        QStringLiteral("machine_color_design.png"),
        QStringLiteral("PNG 图片 (*.png)"));
    if (fileName.isEmpty()) {
        return;
    }

    statusBar()->showMessage(QStringLiteral("正在渲染原始分辨率图片…"));
    QImage image = preview_->recolorer().render(controlPanel_->state());
    if (image.isNull() || !image.save(fileName, "PNG")) {
        QMessageBox::critical(this,
                              QStringLiteral("导出失败"),
                              QStringLiteral("无法写入 PNG 文件。请检查路径和权限。"));
        statusBar()->showMessage(QStringLiteral("导出失败"), 3000);
        return;
    }

    statusBar()->showMessage(QStringLiteral("已导出：%1").arg(fileName), 5000);
}

void MainWindow::savePalette()
{
    bool accepted = false;
    const QString name = QInputDialog::getText(
        this,
        QStringLiteral("方案名称"),
        QStringLiteral("请输入配色方案名称："),
        QLineEdit::Normal,
        QStringLiteral("新配色方案"),
        &accepted);
    if (!accepted) {
        return;
    }

    const QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存配色方案"),
        name + QStringLiteral(".mcdpalette.json"),
        QStringLiteral("配色方案 (*.mcdpalette.json *.json)"));
    if (fileName.isEmpty()) {
        return;
    }

    QString error;
    if (!PaletteSerializer::save(fileName, name, controlPanel_->state(), &error)) {
        QMessageBox::critical(this, QStringLiteral("保存失败"), error);
        return;
    }
    statusBar()->showMessage(QStringLiteral("配色方案已保存"), 3000);
}

void MainWindow::loadPalette()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("载入配色方案"),
        QString(),
        QStringLiteral("配色方案 (*.mcdpalette.json *.json)"));
    if (fileName.isEmpty()) {
        return;
    }

    QString name;
    QString error;
    RecolorState loaded;
    if (!PaletteSerializer::load(fileName, &name, &loaded, &error)) {
        QMessageBox::critical(this, QStringLiteral("载入失败"), error);
        return;
    }

    controlPanel_->setState(loaded, true);
    statusBar()->showMessage(
        name.isEmpty()
            ? QStringLiteral("配色方案已载入")
            : QStringLiteral("已载入方案：%1").arg(name),
        4000);
}
