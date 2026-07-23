#include "ImageRecolorer.h"

#include <QColor>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace {
float srgbChannelToLinear(float channel)
{
    return channel <= 0.04045F
        ? channel / 12.92F
        : std::pow((channel + 0.055F) / 1.055F, 2.4F);
}

float linearChannelToSrgb(float channel)
{
    channel = std::max(0.0F, channel);
    return channel <= 0.0031308F
        ? channel * 12.92F
        : 1.055F * std::pow(channel, 1.0F / 2.4F) - 0.055F;
}

std::array<float, 3> colorToLinear(const QColor &color)
{
    return {
        srgbChannelToLinear(color.redF()),
        srgbChannelToLinear(color.greenF()),
        srgbChannelToLinear(color.blueF())
    };
}

float luminance(const std::array<float, 3> &linear)
{
    return linear[0] * 0.2126F + linear[1] * 0.7152F + linear[2] * 0.0722F;
}

float smoothRamp(float edge0, float edge1, float value)
{
    float t = (value - edge0) / (edge1 - edge0);
    t = std::clamp(t, 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

std::array<float, 3> applyRegion(
    const std::array<float, 3> &sourceLinear,
    float sourceLuminance,
    const std::array<float, 3> &targetLinear,
    float referenceLuminance,
    float mask,
    const RecolorState &state)
{
    if (mask <= 0.0001F) {
        return sourceLinear;
    }

    const float relativeLight = std::clamp(
        sourceLuminance / std::max(referenceLuminance, 0.0001F),
        0.055F,
        2.4F);
    const float exponent = 0.82F + 0.20F * std::clamp(state.shadow, 0.0F, 2.0F);
    float shade = std::pow(relativeLight, exponent);
    shade = std::min(shade, 1.85F - std::clamp(state.matte, 0.0F, 1.0F) * 0.35F);

    const float highlightMask = smoothRamp(1.02F, 1.72F, relativeLight);
    const float neutralStrength = highlightMask
        * std::clamp(state.highlight, 0.0F, 1.0F)
        * (1.0F - std::clamp(state.matte, 0.0F, 1.0F) * 0.45F);

    std::array<float, 3> result {};
    for (int channel = 0; channel < 3; ++channel) {
        float painted = targetLinear[channel] * state.brightness * shade;
        painted = std::max(painted, sourceLuminance * neutralStrength);
        painted = std::clamp(painted, 0.0F, 1.0F);
        result[channel] = sourceLinear[channel] * (1.0F - mask) + painted * mask;
    }
    return result;
}
} // namespace

ImageRecolorer::ImageRecolorer()
{
    original_ = QImage(QStringLiteral(":/assets/machine_original.png"))
        .convertToFormat(QImage::Format_RGBA8888);
    upperMask_ = QImage(QStringLiteral(":/assets/upper_mask.png"))
        .convertToFormat(QImage::Format_RGBA8888);
    lowerMask_ = QImage(QStringLiteral(":/assets/lower_mask.png"))
        .convertToFormat(QImage::Format_RGBA8888);

    if (original_.isNull()
        || upperMask_.isNull()
        || lowerMask_.isNull()) {
        errorString_ = QStringLiteral("无法读取内置机器图片或蒙版资源。");
        return;
    }

    if (upperMask_.size() != original_.size()
        || lowerMask_.size() != original_.size()) {
        errorString_ = QStringLiteral("机器图片与蒙版尺寸不一致。");
        return;
    }

    upperReference_ = calculateReferenceLuminance(original_, upperMask_);
    lowerReference_ = calculateReferenceLuminance(original_, lowerMask_);
}

bool ImageRecolorer::isValid() const
{
    return errorString_.isEmpty() && !original_.isNull();
}

QImage ImageRecolorer::render(const RecolorState &state) const
{
    if (!isValid()) {
        return {};
    }

    QImage output(original_.size(), QImage::Format_RGBA8888);
    const auto upperTarget = colorToLinear(state.upperColor);
    const auto lowerTarget = colorToLinear(state.lowerColor);

    for (int y = 0; y < original_.height(); ++y) {
        const auto *sourceLine = original_.constScanLine(y);
        const auto *upperLine = upperMask_.constScanLine(y);
        const auto *lowerLine = lowerMask_.constScanLine(y);
        auto *outputLine = output.scanLine(y);

        for (int x = 0; x < original_.width(); ++x) {
            const int offset = x * 4;
            const std::array<float, 3> sourceLinear {
                srgbChannelToLinear(sourceLine[offset + 0] / 255.0F),
                srgbChannelToLinear(sourceLine[offset + 1] / 255.0F),
                srgbChannelToLinear(sourceLine[offset + 2] / 255.0F)
            };
            const float sourceLum = luminance(sourceLinear);

            std::array<float, 3> result = sourceLinear;
            result = applyRegion(
                result,
                sourceLum,
                upperTarget,
                upperReference_,
                upperLine[offset] / 255.0F,
                state);
            result = applyRegion(
                result,
                sourceLum,
                lowerTarget,
                lowerReference_,
                lowerLine[offset] / 255.0F,
                state);

            outputLine[offset + 0] = static_cast<uchar>(qBound(0, qRound(linearChannelToSrgb(result[0]) * 255.0F), 255));
            outputLine[offset + 1] = static_cast<uchar>(qBound(0, qRound(linearChannelToSrgb(result[1]) * 255.0F), 255));
            outputLine[offset + 2] = static_cast<uchar>(qBound(0, qRound(linearChannelToSrgb(result[2]) * 255.0F), 255));
            outputLine[offset + 3] = sourceLine[offset + 3];
        }
    }

    return output;
}

float ImageRecolorer::calculateReferenceLuminance(
    const QImage &original,
    const QImage &mask)
{
    constexpr int binCount = 4096;
    std::array<double, binCount> histogram {};
    double totalWeight = 0.0;

    for (int y = 0; y < original.height(); ++y) {
        const auto *sourceLine = original.constScanLine(y);
        const auto *maskLine = mask.constScanLine(y);
        for (int x = 0; x < original.width(); ++x) {
            const int offset = x * 4;
            const float weight = maskLine[offset] / 255.0F;
            if (weight <= 0.01F) {
                continue;
            }
            const std::array<float, 3> linear {
                srgbChannelToLinear(sourceLine[offset + 0] / 255.0F),
                srgbChannelToLinear(sourceLine[offset + 1] / 255.0F),
                srgbChannelToLinear(sourceLine[offset + 2] / 255.0F)
            };
            const float lum = std::clamp(luminance(linear), 0.0F, 1.0F);
            const int bin = std::min(binCount - 1, static_cast<int>(lum * (binCount - 1)));
            histogram[bin] += weight;
            totalWeight += weight;
        }
    }

    if (totalWeight <= 0.0) {
        return 0.5F;
    }

    const double half = totalWeight * 0.5;
    double accumulated = 0.0;
    for (int bin = 0; bin < binCount; ++bin) {
        accumulated += histogram[bin];
        if (accumulated >= half) {
            return static_cast<float>(bin) / static_cast<float>(binCount - 1);
        }
    }
    return 0.5F;
}
