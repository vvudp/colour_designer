#include "RecolorState.h"

#include <QJsonValue>
#include <QtGlobal>

#include <cmath>

namespace {
float readFloat(const QJsonObject &object, const QString &key, float fallback)
{
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return fallback;
    }
    return static_cast<float>(value.toDouble());
}

QColor readColor(const QJsonObject &object, const QString &key, const QColor &fallback)
{
    const QColor parsed(object.value(key).toString());
    return parsed.isValid() ? parsed : fallback;
}
} // namespace

QJsonObject RecolorState::toJson() const
{
    return {
        {QStringLiteral("upperColor"), upperColor.name(QColor::HexRgb).toUpper()},
        {QStringLiteral("lowerColor"), lowerColor.name(QColor::HexRgb).toUpper()},
        {QStringLiteral("matte"), matte},
        {QStringLiteral("highlight"), highlight},
        {QStringLiteral("shadow"), shadow},
        {QStringLiteral("brightness"), brightness}
    };
}

RecolorState RecolorState::fromJson(const QJsonObject &object)
{
    const RecolorState fallback;
    RecolorState result = fallback;
    result.upperColor = readColor(object, QStringLiteral("upperColor"), fallback.upperColor);
    result.lowerColor = readColor(object, QStringLiteral("lowerColor"), fallback.lowerColor);
    result.matte = qBound(0.0F, readFloat(object, QStringLiteral("matte"), fallback.matte), 1.0F);
    result.highlight = qBound(0.0F, readFloat(object, QStringLiteral("highlight"), fallback.highlight), 1.0F);
    result.shadow = qBound(0.0F, readFloat(object, QStringLiteral("shadow"), fallback.shadow), 2.0F);
    result.brightness = qBound(0.35F, readFloat(object, QStringLiteral("brightness"), fallback.brightness), 1.65F);
    result.showOriginal = false;
    return result;
}

bool RecolorState::approximatelyEquals(const RecolorState &other) const
{
    constexpr float epsilon = 0.0005F;
    return upperColor == other.upperColor
        && lowerColor == other.lowerColor
        && std::abs(matte - other.matte) < epsilon
        && std::abs(highlight - other.highlight) < epsilon
        && std::abs(shadow - other.shadow) < epsilon
        && std::abs(brightness - other.brightness) < epsilon;
}
