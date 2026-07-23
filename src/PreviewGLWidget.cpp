#include "PreviewGLWidget.h"

#include <QFile>
#include <QMouseEvent>
#include <QPainter>
#include <QSurfaceFormat>
#include <QWheelEvent>
#include <QVector3D>

#include <algorithm>
#include <array>
#include <cmath>

PreviewGLWidget::PreviewGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(560, 640);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::OpenHandCursor);
    imageSize_ = recolorer_.sourceSize();
}

PreviewGLWidget::~PreviewGLWidget()
{
    makeCurrent();
    destroyGlResources();
    doneCurrent();
}

void PreviewGLWidget::setState(const RecolorState &state)
{
    state_ = state;
    update();
}

void PreviewGLWidget::resetView()
{
    zoom_ = 1.0;
    pan_ = {};
    update();
}

void PreviewGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.965F, 0.970F, 0.975F, 1.0F);

    if (!recolorer_.isValid()) {
        glError_ = recolorer_.errorString();
        return;
    }

    QFile vertexFile(QStringLiteral(":/shaders/recolor.vert"));
    QFile fragmentFile(QStringLiteral(":/shaders/recolor.frag"));
    if (!vertexFile.open(QIODevice::ReadOnly)
        || !fragmentFile.open(QIODevice::ReadOnly)) {
        glError_ = QStringLiteral("无法读取内置 GLSL 着色器。");
        return;
    }

    if (!program_.addShaderFromSourceCode(
            QOpenGLShader::Vertex,
            vertexFile.readAll())
        || !program_.addShaderFromSourceCode(
            QOpenGLShader::Fragment,
            fragmentFile.readAll())
        || !program_.link()) {
        glError_ = program_.log();
        return;
    }

    if (!vertexArray_.create() || !vertexBuffer_.create()) {
        glError_ = QStringLiteral("无法创建 OpenGL 顶点资源。");
        return;
    }

    vertexArray_.bind();
    vertexBuffer_.bind();
    vertexBuffer_.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    vertexBuffer_.allocate(static_cast<int>(sizeof(float) * 16));

    program_.bind();
    program_.enableAttributeArray(0);
    program_.enableAttributeArray(1);
    program_.setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * static_cast<int>(sizeof(float)));
    program_.setAttributeBuffer(1, GL_FLOAT, 2 * static_cast<int>(sizeof(float)), 2,
                                4 * static_cast<int>(sizeof(float)));
    program_.release();
    vertexBuffer_.release();
    vertexArray_.release();

    originalTexture_ = createTexture(QStringLiteral(":/assets/machine_original.png"));
    upperMaskTexture_ = createTexture(QStringLiteral(":/assets/upper_mask.png"));
    lowerMaskTexture_ = createTexture(QStringLiteral(":/assets/lower_mask.png"));

    if (originalTexture_ == nullptr
        || upperMaskTexture_ == nullptr
        || lowerMaskTexture_ == nullptr) {
        glError_ = QStringLiteral("无法创建 OpenGL 图片纹理。");
        destroyGlResources();
        return;
    }

    glReady_ = true;
}

void PreviewGLWidget::resizeGL(int, int)
{
    updateQuad();
}

void PreviewGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!glReady_) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setPen(QColor(QStringLiteral("#9F2F2F")));
        painter.drawText(rect().adjusted(30, 30, -30, -30),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         QStringLiteral("预览初始化失败\n%1").arg(glError_));
        return;
    }

    updateQuad();

    program_.bind();
    vertexArray_.bind();

    originalTexture_->bind(0);
    upperMaskTexture_->bind(1);
    lowerMaskTexture_->bind(2);

    program_.setUniformValue("uOriginal", 0);
    program_.setUniformValue("uUpperMask", 1);
    program_.setUniformValue("uLowerMask", 2);
    program_.setUniformValue("uUpperColor", QVector3D(
        state_.upperColor.redF(),
        state_.upperColor.greenF(),
        state_.upperColor.blueF()));
    program_.setUniformValue("uLowerColor", QVector3D(
        state_.lowerColor.redF(),
        state_.lowerColor.greenF(),
        state_.lowerColor.blueF()));
    program_.setUniformValue("uUpperReference", recolorer_.upperReference());
    program_.setUniformValue("uLowerReference", recolorer_.lowerReference());
    program_.setUniformValue("uMatte", state_.matte);
    program_.setUniformValue("uHighlight", state_.highlight);
    program_.setUniformValue("uShadow", state_.shadow);
    program_.setUniformValue("uBrightness", state_.brightness);
    program_.setUniformValue("uShowOriginal", state_.showOriginal);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    lowerMaskTexture_->release(2);
    upperMaskTexture_->release(1);
    originalTexture_->release(0);
    vertexArray_.release();
    program_.release();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QColor(QStringLiteral("#65717C")));
    painter.drawText(QRect(16, height() - 36, width() - 32, 24),
                     Qt::AlignCenter,
                     QStringLiteral("滚轮缩放 · 左键拖动 · 双击恢复适配"));
}

void PreviewGLWidget::wheelEvent(QWheelEvent *event)
{
    const double factor = std::pow(1.0015, event->angleDelta().y());
    zoom_ = std::clamp(zoom_ * factor, 0.70, 8.0);
    update();
    event->accept();
}

void PreviewGLWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        lastMousePosition_ = event->position();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
}

void PreviewGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging_) {
        pan_ += event->position() - lastMousePosition_;
        lastMousePosition_ = event->position();
        update();
        event->accept();
    }
}

void PreviewGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
    }
}

void PreviewGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        resetView();
        event->accept();
    }
}

QOpenGLTexture *PreviewGLWidget::createTexture(const QString &resourcePath)
{
    QImage image(resourcePath);
    if (image.isNull()) {
        return nullptr;
    }

    image = image.convertToFormat(QImage::Format_RGBA8888).flipped(Qt::Vertical);
    auto *texture = new QOpenGLTexture(image);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    return texture;
}

void PreviewGLWidget::destroyGlResources()
{
    delete originalTexture_;
    delete upperMaskTexture_;
    delete lowerMaskTexture_;
    originalTexture_ = nullptr;
    upperMaskTexture_ = nullptr;
    lowerMaskTexture_ = nullptr;

    if (vertexBuffer_.isCreated()) {
        vertexBuffer_.destroy();
    }
    if (vertexArray_.isCreated()) {
        vertexArray_.destroy();
    }
    glReady_ = false;
}

void PreviewGLWidget::updateQuad()
{
    if (!vertexBuffer_.isCreated() || imageSize_.isEmpty() || width() <= 0 || height() <= 0) {
        return;
    }

    constexpr double margin = 34.0;
    const double availableWidth = std::max(1.0, width() - margin * 2.0);
    const double availableHeight = std::max(1.0, height() - margin * 2.0 - 26.0);
    const double fitScale = std::min(
        availableWidth / imageSize_.width(),
        availableHeight / imageSize_.height());
    const double scale = fitScale * zoom_;
    const QSizeF displayedSize(imageSize_.width() * scale, imageSize_.height() * scale);
    const QPointF center(width() * 0.5 + pan_.x(),
                         (height() - 24.0) * 0.5 + pan_.y());

    const double leftPx = center.x() - displayedSize.width() * 0.5;
    const double rightPx = center.x() + displayedSize.width() * 0.5;
    const double topPx = center.y() - displayedSize.height() * 0.5;
    const double bottomPx = center.y() + displayedSize.height() * 0.5;

    const float left = static_cast<float>(2.0 * leftPx / width() - 1.0);
    const float right = static_cast<float>(2.0 * rightPx / width() - 1.0);
    const float top = static_cast<float>(1.0 - 2.0 * topPx / height());
    const float bottom = static_cast<float>(1.0 - 2.0 * bottomPx / height());

    const std::array<float, 16> vertices {
        left,  bottom, 0.0F, 0.0F,
        right, bottom, 1.0F, 0.0F,
        left,  top,    0.0F, 1.0F,
        right, top,    1.0F, 1.0F
    };

    vertexBuffer_.bind();
    vertexBuffer_.write(0, vertices.data(), static_cast<int>(sizeof(vertices)));
    vertexBuffer_.release();
}
