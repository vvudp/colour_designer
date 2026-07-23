#include "PaletteSerializer.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

bool PaletteSerializer::save(
    const QString &fileName,
    const QString &paletteName,
    const RecolorState &state,
    QString *errorMessage)
{
    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("MachineColorDesignerPalette"));
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("name"), paletteName);
    root.insert(QStringLiteral("state"), state.toJson());

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    const QJsonDocument document(root);
    if (file.write(document.toJson(QJsonDocument::Indented)) < 0) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    if (!file.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

bool PaletteSerializer::load(
    const QString &fileName,
    QString *paletteName,
    RecolorState *state,
    QString *errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = parseError.errorString();
        }
        return false;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString()
        != QStringLiteral("MachineColorDesignerPalette")) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("不是有效的机器配色方案文件。");
        }
        return false;
    }

    if (paletteName != nullptr) {
        *paletteName = root.value(QStringLiteral("name")).toString();
    }
    if (state != nullptr) {
        *state = RecolorState::fromJson(root.value(QStringLiteral("state")).toObject());
    }
    return true;
}
