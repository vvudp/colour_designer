#include "ExportComposer.h"

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPen>
#include <QTextOption>

#include <algorithm>

namespace {
constexpr auto kApplicationName = "MachineColorDesigner";
constexpr auto kApplicationVersion = "1.2.0";

QString colorHex(const QColor &color)
{
    return color.name(QColor::HexRgb).toUpper();
}

QString hueText(int hue)
{
    return hue < 0 ? QStringLiteral("—") : QString::number(hue) + QStringLiteral("°");
}

QString rgbText(const QColor &color)
{
    return QStringLiteral("%1, %2, %3")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue());
}

QString hsvText(const QColor &color)
{
    return QStringLiteral("%1, %2%, %3%")
        .arg(hueText(color.hsvHue()))
        .arg(qRound(color.hsvSaturationF() * 100.0))
        .arg(qRound(color.valueF() * 100.0));
}

QString hslText(const QColor &color)
{
    return QStringLiteral("%1, %2%, %3%")
        .arg(hueText(color.hslHue()))
        .arg(qRound(color.hslSaturationF() * 100.0))
        .arg(qRound(color.lightnessF() * 100.0));
}

QFont sizedFont(const QFont &base, int pixelSize, QFont::Weight weight = QFont::Normal)
{
    QFont result = base;
    result.setPixelSize(pixelSize);
    result.setWeight(weight);
    return result;
}

void drawTextLine(
    QPainter &painter,
    const QRect &rect,
    const QString &left,
    const QString &right,
    const QFont &labelFont,
    const QFont &valueFont,
    const QColor &labelColor,
    const QColor &valueColor)
{
    const int labelWidth = qRound(rect.width() * 0.34);
    painter.setFont(labelFont);
    painter.setPen(labelColor);
    painter.drawText(QRect(rect.left(), rect.top(), labelWidth, rect.height()),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     left);

    painter.setFont(valueFont);
    painter.setPen(valueColor);
    painter.drawText(QRect(rect.left() + labelWidth, rect.top(), rect.width() - labelWidth, rect.height()),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     right);
}

void drawColorCard(
    QPainter &painter,
    const QRect &rect,
    const QString &title,
    const QColor &color,
    const QFont &baseFont)
{
    painter.setPen(QPen(QColor(QStringLiteral("#D7DDE4")), 2));
    painter.setBrush(QColor(QStringLiteral("#FFFFFF")));
    painter.drawRoundedRect(rect, 22, 22);

    const int horizontalPadding = 34;
    const QRect content = rect.adjusted(horizontalPadding, 28, -horizontalPadding, -28);

    painter.setFont(sizedFont(baseFont, 30, QFont::DemiBold));
    painter.setPen(QColor(QStringLiteral("#20262D")));
    painter.drawText(QRect(content.left(), content.top(), content.width(), 44),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     title);

    const QRect swatchRect(content.left(), content.top() + 64, content.width(), 116);
    painter.setPen(QPen(QColor(QStringLiteral("#B8C0C9")), 2));
    painter.setBrush(color);
    painter.drawRoundedRect(swatchRect, 16, 16);

    const QFont labelFont = sizedFont(baseFont, 22, QFont::Normal);
    const QFont valueFont = sizedFont(baseFont, 23, QFont::DemiBold);
    const QColor labelColor(QStringLiteral("#68717C"));
    const QColor valueColor(QStringLiteral("#242A31"));

    int lineY = swatchRect.bottom() + 24;
    constexpr int lineHeight = 42;
    const int lineWidth = content.width();

    drawTextLine(painter,
                 QRect(content.left(), lineY, lineWidth, lineHeight),
                 QStringLiteral("HEX"),
                 colorHex(color),
                 labelFont,
                 valueFont,
                 labelColor,
                 valueColor);
    lineY += lineHeight;

    drawTextLine(painter,
                 QRect(content.left(), lineY, lineWidth, lineHeight),
                 QStringLiteral("RGB"),
                 rgbText(color),
                 labelFont,
                 valueFont,
                 labelColor,
                 valueColor);
    lineY += lineHeight;

    drawTextLine(painter,
                 QRect(content.left(), lineY, lineWidth, lineHeight),
                 QStringLiteral("HSV"),
                 hsvText(color),
                 labelFont,
                 valueFont,
                 labelColor,
                 valueColor);
    lineY += lineHeight;

    drawTextLine(painter,
                 QRect(content.left(), lineY, lineWidth, lineHeight),
                 QStringLiteral("HSL"),
                 hslText(color),
                 labelFont,
                 valueFont,
                 labelColor,
                 valueColor);
}

void drawMaterialCard(
    QPainter &painter,
    const QRect &rect,
    const RecolorState &state,
    const QFont &baseFont)
{
    painter.setPen(QPen(QColor(QStringLiteral("#D7DDE4")), 2));
    painter.setBrush(QColor(QStringLiteral("#FFFFFF")));
    painter.drawRoundedRect(rect, 22, 22);

    const QRect content = rect.adjusted(34, 26, -34, -24);
    painter.setFont(sizedFont(baseFont, 30, QFont::DemiBold));
    painter.setPen(QColor(QStringLiteral("#20262D")));
    painter.drawText(QRect(content.left(), content.top(), content.width(), 44),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("材质与光影参数"));

    const QFont labelFont = sizedFont(baseFont, 22, QFont::Normal);
    const QFont valueFont = sizedFont(baseFont, 23, QFont::DemiBold);
    const QColor labelColor(QStringLiteral("#68717C"));
    const QColor valueColor(QStringLiteral("#242A31"));

    int lineY = content.top() + 66;
    constexpr int lineHeight = 46;
    const auto percentText = [](float value) {
        return QStringLiteral("%1%").arg(qRound(value * 100.0F));
    };

    drawTextLine(painter, QRect(content.left(), lineY, content.width(), lineHeight),
                 QStringLiteral("哑光程度"), percentText(state.matte),
                 labelFont, valueFont, labelColor, valueColor);
    lineY += lineHeight;
    drawTextLine(painter, QRect(content.left(), lineY, content.width(), lineHeight),
                 QStringLiteral("高光保留"), percentText(state.highlight),
                 labelFont, valueFont, labelColor, valueColor);
    lineY += lineHeight;
    drawTextLine(painter, QRect(content.left(), lineY, content.width(), lineHeight),
                 QStringLiteral("阴影深度"), percentText(state.shadow),
                 labelFont, valueFont, labelColor, valueColor);
    lineY += lineHeight;
    drawTextLine(painter, QRect(content.left(), lineY, content.width(), lineHeight),
                 QStringLiteral("整体明度"), percentText(state.brightness),
                 labelFont, valueFont, labelColor, valueColor);
}

QJsonObject colorObject(const QColor &color)
{
    return {
        {QStringLiteral("hex"), colorHex(color)},
        {QStringLiteral("red"), color.red()},
        {QStringLiteral("green"), color.green()},
        {QStringLiteral("blue"), color.blue()},
        {QStringLiteral("hsvHue"), color.hsvHue()},
        {QStringLiteral("hsvSaturation"), color.hsvSaturationF()},
        {QStringLiteral("hsvValue"), color.valueF()},
        {QStringLiteral("hslHue"), color.hslHue()},
        {QStringLiteral("hslSaturation"), color.hslSaturationF()},
        {QStringLiteral("hslLightness"), color.lightnessF()}
    };
}
} // namespace

QImage ExportComposer::composeAnnotatedImage(
    const QImage &machineImage,
    const RecolorState &state,
    const QDateTime &exportedAt)
{
    if (machineImage.isNull()) {
        return {};
    }

    const int panelWidth = std::max(500, qRound(machineImage.width() * 0.74));
    QImage output(machineImage.width() + panelWidth,
                  machineImage.height(),
                  QImage::Format_RGBA8888);
    output.fill(QColor(QStringLiteral("#F2F4F7")));

    QPainter painter(&output);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter.fillRect(QRect(0, 0, machineImage.width(), machineImage.height()), Qt::white);
    painter.drawImage(QPoint(0, 0), machineImage);

    const int panelLeft = machineImage.width();
    painter.fillRect(QRect(panelLeft, 0, panelWidth, output.height()),
                     QColor(QStringLiteral("#F2F4F7")));
    painter.setPen(QPen(QColor(QStringLiteral("#D6DCE3")), 2));
    painter.drawLine(panelLeft, 0, panelLeft, output.height());

    QFont baseFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    const int margin = 46;
    const int contentWidth = panelWidth - margin * 2;

    painter.setFont(sizedFont(baseFont, 42, QFont::Bold));
    painter.setPen(QColor(QStringLiteral("#171B20")));
    painter.drawText(QRect(panelLeft + margin, 62, contentWidth, 62),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("机器配色方案记录"));

    painter.setFont(sizedFont(baseFont, 22, QFont::Normal));
    painter.setPen(QColor(QStringLiteral("#6C7580")));
    painter.drawText(QRect(panelLeft + margin, 126, contentWidth, 42),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Machine Color Design Parameters"));

    painter.setPen(QPen(QColor(QStringLiteral("#D8DEE5")), 2));
    painter.drawLine(panelLeft + margin, 188,
                     panelLeft + panelWidth - margin, 188);

    const int cardLeft = panelLeft + margin;
    const int cardWidth = contentWidth;
    const int colorCardHeight = 404;
    const int gap = 30;

    int y = 226;
    drawColorCard(painter,
                  QRect(cardLeft, y, cardWidth, colorCardHeight),
                  QStringLiteral("上部机壳颜色"),
                  state.upperColor,
                  baseFont);
    y += colorCardHeight + gap;

    drawColorCard(painter,
                  QRect(cardLeft, y, cardWidth, colorCardHeight),
                  QStringLiteral("下部机壳颜色"),
                  state.lowerColor,
                  baseFont);
    y += colorCardHeight + gap;

    const int materialCardHeight = 286;
    drawMaterialCard(painter,
                     QRect(cardLeft, y, cardWidth, materialCardHeight),
                     state,
                     baseFont);
    y += materialCardHeight + gap;

    const int footerTop = y;
    painter.setPen(QPen(QColor(QStringLiteral("#D8DEE5")), 2));
    painter.drawLine(cardLeft, footerTop,
                     cardLeft + cardWidth, footerTop);

    painter.setFont(sizedFont(baseFont, 20, QFont::Normal));
    painter.setPen(QColor(QStringLiteral("#727B86")));
    painter.drawText(QRect(cardLeft, footerTop + 22, cardWidth, 34),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("导出时间"));

    painter.setFont(sizedFont(baseFont, 22, QFont::DemiBold));
    painter.setPen(QColor(QStringLiteral("#252B32")));
    painter.drawText(QRect(cardLeft, footerTop + 58, cardWidth, 36),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     exportedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));

    painter.setFont(sizedFont(baseFont, 18, QFont::Normal));
    painter.setPen(QColor(QStringLiteral("#8A929C")));
    QTextOption noteOption;
    noteOption.setWrapMode(QTextOption::WordWrap);
    painter.drawText(QRect(cardLeft, footerTop + 112, cardWidth, 92),
                     QStringLiteral("PNG 文件内部同时写入可机器读取的 JSON 参数，便于后续归档、核对和自动提取。"),
                     noteOption);

    painter.end();
    return output;
}

QString ExportComposer::metadataJson(
    const RecolorState &state,
    const QSize &renderedImageSize,
    const QDateTime &exportedAt)
{
    const QJsonObject root {
        {QStringLiteral("application"), QString::fromLatin1(kApplicationName)},
        {QStringLiteral("version"), QString::fromLatin1(kApplicationVersion)},
        {QStringLiteral("exportedAt"), exportedAt.toString(Qt::ISODateWithMs)},
        {QStringLiteral("renderedImage"), QJsonObject {
            {QStringLiteral("width"), renderedImageSize.width()},
            {QStringLiteral("height"), renderedImageSize.height()}
        }},
        {QStringLiteral("upperColor"), colorObject(state.upperColor)},
        {QStringLiteral("lowerColor"), colorObject(state.lowerColor)},
        {QStringLiteral("material"), QJsonObject {
            {QStringLiteral("matte"), state.matte},
            {QStringLiteral("highlight"), state.highlight},
            {QStringLiteral("shadow"), state.shadow},
            {QStringLiteral("brightness"), state.brightness}
        }}
    };

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

QString ExportComposer::defaultFileName(const RecolorState &state)
{
    QString upper = colorHex(state.upperColor);
    QString lower = colorHex(state.lowerColor);
    upper.remove(QLatin1Char('#'));
    lower.remove(QLatin1Char('#'));
    return QStringLiteral("machine_U-%1_L-%2.png").arg(upper, lower);
}
