#include "MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Machine Color Designer"));
    application.setOrganizationName(QStringLiteral("Org"));
    application.setApplicationVersion(QStringLiteral("1.0.0"));
    application.setFont(QFont(QStringLiteral("Noto Sans CJK SC"), 10));

    QFile styleFile(QStringLiteral(":/styles/app.qss"));
    if (styleFile.open(QIODevice::ReadOnly)) {
        application.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    MainWindow window;
    window.show();
    return application.exec();
}
