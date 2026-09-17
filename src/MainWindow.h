#pragma once

#include <QJsonArray>
#include <QMainWindow>
#include <QList>

#include "Character.h"

class ChatPage;
class CharactersPage;
class SettingsPage;
class KoboldClient;
class GenerationSettings;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    ChatPage *chatPage;
    CharactersPage *charactersPage;
    SettingsPage *settingsPage;

    KoboldClient *kobold;
    GenerationSettings *generationSettings;

    QList<Character> characters;

    Character defaultCharacter;
    Character *selectedCharacter;

    QJsonArray conversation;

    void createDefaultCharacter();

    void loadCharacters();
    void selectCharacter(int index);

    void buildCharacterPrompt();
    QString replacePlaceholders(const QString &text) const;
    void displayConversation();

    void sendMessage();
    void retryLastResponse();
    void updateRetryButtonState();

    void saveConversation();
    void loadConversation();

    void connectToKobold();

    void loadGenerationSettings();
    void saveGenerationSettings();
    void updateGenerationSettingsFromUi();

    void setupPages();
    void setupConnections();
};

