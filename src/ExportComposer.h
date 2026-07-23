#pragma once

#include "RecolorState.h"

#include <QDateTime>
#include <QImage>
#include <QString>

class ExportComposer final
{
public:
    [[nodiscard]] static QImage composeAnnotatedImage(
        const QImage &machineImage,
        const RecolorState &state,
        const QDateTime &exportedAt
    );

    [[nodiscard]] static QString metadataJson(
        const RecolorState &state,
        const QSize &renderedImageSize,
        const QDateTime &exportedAt
    );

    [[nodiscard]] static QString defaultFileName(const RecolorState &state);
};
