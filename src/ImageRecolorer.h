#pragma once

#include "RecolorState.h"

#include <QImage>
#include <QSize>
#include <QString>

class ImageRecolorer
{
public:
    ImageRecolorer();

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] QString errorString() const { return errorString_; }
    [[nodiscard]] QSize sourceSize() const { return original_.size(); }

    [[nodiscard]] QImage render(const RecolorState &state) const;

    [[nodiscard]] float upperReference() const { return upperReference_; }
    [[nodiscard]] float lowerReference() const { return lowerReference_; }

private:
    static float calculateReferenceLuminance(
        const QImage &original,
        const QImage &mask
    );

    QImage original_;
    QImage upperMask_;
    QImage lowerMask_;
    QString errorString_;

    float upperReference_ {0.72F};
    float lowerReference_ {0.15F};
};
