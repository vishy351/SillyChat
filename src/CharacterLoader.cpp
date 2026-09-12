#include "CharacterLoader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

QList<Character> CharacterLoader::loadCharacters(
    const QString &directoryPath
)
{
    QList<Character> characters;

    QDir directory(directoryPath);

    if (!directory.exists())
        return characters;

    const QStringList jsonFiles =
        directory.entryList(
            QStringList() << "*.json",
            QDir::Files,
            QDir::Name
        );

    for (const QString &jsonFileName : jsonFiles)
    {
        const QString jsonPath =
            directory.filePath(jsonFileName);

        QFile file(jsonPath);

        if (!file.open(QIODevice::ReadOnly))
            continue;

        QJsonParseError parseError;

        const QJsonDocument document =
            QJsonDocument::fromJson(
                file.readAll(),
                &parseError
            );

        if (parseError.error != QJsonParseError::NoError)
            continue;

        if (!document.isObject())
            continue;

        const QJsonObject rootObject =
            document.object();

        QJsonObject characterObject =
            rootObject;

        const QJsonValue dataValue =
            rootObject.value("data");

        const QString spec =
            rootObject.value("spec").toString();

        const bool isV2 =
            spec == "chara_card_v2" &&
            dataValue.isObject();

        if (isV2)
        {
            characterObject =
                dataValue.toObject();
        }

        Character character;

        character.name =
            characterObject.value(
                "name"
            ).toString();

        if (character.name.isEmpty())
        {
            character.name =
                QFileInfo(
                    jsonPath
                ).completeBaseName();
        }

        character.description =
            characterObject.value(
                "description"
            ).toString();

        character.personality =
            characterObject.value(
                "personality"
            ).toString();

        character.scenario =
            characterObject.value(
                "scenario"
            ).toString();

        character.firstMessage =
            characterObject.value(
                "first_mes"
            ).toString();

        character.exampleDialogue =
            characterObject.value(
                "mes_example"
            ).toString();

        if (isV2)
        {
            character.characterSystemPrompt =
                characterObject.value(
                    "system_prompt"
                ).toString();

            const QJsonValue alternateGreetingsValue =
                characterObject.value(
                    "alternate_greetings"
                );

            if (alternateGreetingsValue.isArray())
            {
                const QJsonArray alternateGreetings =
                    alternateGreetingsValue.toArray();

                for (const QJsonValue &greetingValue :
                     alternateGreetings)
                {
                    if (!greetingValue.isString())
                        continue;

                    const QString greeting =
                        greetingValue.toString();

                    if (greeting.isEmpty())
                        continue;

                    character.alternateGreetings.append(
                        greeting
                    );
                }
            }
        }

        const QString baseName =
            QFileInfo(
                jsonPath
            ).completeBaseName();

        const QStringList imageExtensions =
        {
            "png",
            "jpg",
            "jpeg"
        };

        for (const QString &extension :
             imageExtensions)
        {
            const QString imagePath =
                directory.filePath(
                    baseName +
                    "." +
                    extension
                );

            if (QFileInfo::exists(imagePath))
            {
                character.imagePath =
                    imagePath;

                break;
            }
        }

        characters.append(
            character
        );
    }

    return characters;
}

