#pragma once

#include "RecolorState.h"

#include <QString>

class PaletteSerializer
{
public:
    static bool save(
        const QString &fileName,
        const QString &paletteName,
        const RecolorState &state,
        QString *errorMessage = nullptr
    );

    static bool load(
        const QString &fileName,
        QString *paletteName,
        RecolorState *state,
        QString *errorMessage = nullptr
    );
};
