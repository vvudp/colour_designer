#pragma once

#include "ImageRecolorer.h"
#include "RecolorState.h"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPointF>

class PreviewGLWidget final
    : public QOpenGLWidget
    , protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit PreviewGLWidget(QWidget *parent = nullptr);
    ~PreviewGLWidget() override;

    void setState(const RecolorState &state);
    void resetView();

    [[nodiscard]] const ImageRecolorer &recolorer() const { return recolorer_; }

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QOpenGLTexture *createTexture(const QString &resourcePath);
    void destroyGlResources();
    void updateQuad();

    RecolorState state_;
    ImageRecolorer recolorer_;

    QOpenGLShaderProgram program_;
    QOpenGLBuffer vertexBuffer_ {QOpenGLBuffer::VertexBuffer};
    QOpenGLVertexArrayObject vertexArray_;
    QOpenGLTexture *originalTexture_ {nullptr};
    QOpenGLTexture *upperMaskTexture_ {nullptr};
    QOpenGLTexture *lowerMaskTexture_ {nullptr};

    QSize imageSize_;
    double zoom_ {1.0};
    QPointF pan_;
    QPointF lastMousePosition_;
    bool dragging_ {false};
    bool glReady_ {false};
    QString glError_;
};
