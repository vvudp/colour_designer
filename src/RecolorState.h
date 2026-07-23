#pragma once

#include <QColor>
#include <QJsonObject>
#include <QMetaType>

struct RecolorState
{
    QColor upperColor {QStringLiteral("#D0D4D7")};
    QColor lowerColor {QStringLiteral("#68A6C4")};

    float matte {0.88F};
    float highlight {0.22F};
    float shadow {1.00F};
    float brightness {1.00F};

    bool showOriginal {false};

    [[nodiscard]] QJsonObject toJson() const;
    [[nodiscard]] static RecolorState fromJson(const QJsonObject &object);

    [[nodiscard]] bool approximatelyEquals(const RecolorState &other) const;
};

Q_DECLARE_METATYPE(RecolorState)
