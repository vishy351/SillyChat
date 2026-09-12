#pragma once

#include <QString>
#include <QStringList>

class Character
{
public:
    QString name;
    QString description;
    QString personality;
    QString scenario;
    QString firstMessage;
    QString exampleDialogue;
    QString characterSystemPrompt;
    QStringList alternateGreetings;

    QString imagePath;
};

