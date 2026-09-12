#pragma once

#include "Character.h"

#include <QList>
#include <QString>

class CharacterLoader
{
public:
    static QList<Character> loadCharacters(
        const QString &directoryPath
    );
};

