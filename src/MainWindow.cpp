#include "MainWindow.h"

#include "ChatPage.h"
#include "CharactersPage.h"
#include "SettingsPage.h"
#include "KoboldClient.h"
#include "GenerationSettings.h"
#include "CharacterLoader.h"

#include <QStatusBar>
#include <QLineEdit>
#include <QAction>
#include <QTimer>
#include <QScrollBar>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTextEdit>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QVector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      chatPage(nullptr),
      charactersPage(nullptr),
      settingsPage(nullptr),
      kobold(new KoboldClient(this)),
      generationSettings(new GenerationSettings()),
      selectedCharacter(nullptr)
{
    setWindowTitle(
        "SillyChat"
    );

    resize(
        1000,
        800
    );

    setupPages();
    setupConnections();

    loadCharacters();

    connectToKobold();
}

void MainWindow::setupPages()
{
    auto *tabs =
        new QTabWidget(
            this
        );

    chatPage =
        new ChatPage(
            tabs
        );

    charactersPage =
        new CharactersPage(
            tabs
        );

    settingsPage =
        new SettingsPage(
            kobold,
            generationSettings,
            tabs
        );

    tabs->addTab(
        chatPage,
        "Chat"
    );

    tabs->addTab(
        charactersPage,
        "Characters"
    );

    tabs->addTab(
        settingsPage,
        "Settings"
    );

    setCentralWidget(
        tabs
    );

    /*
     * Conversation file actions remain available
     * without recreating the chat controls that now
     * belong to ChatPage.
     */

    auto *conversationMenu =
        menuBar()->addMenu(
            "Conversation"
        );

    auto *saveAction =
        conversationMenu->addAction(
            "Save Conversation"
        );

    auto *loadAction =
        conversationMenu->addAction(
            "Load Conversation"
        );

    auto *editAction =
        conversationMenu->addAction(
            "Edit Conversation"
        );

    connect(
        saveAction,
        &QAction::triggered,
        this,
        &MainWindow::saveConversation
    );

    connect(
        loadAction,
        &QAction::triggered,
        this,
        &MainWindow::loadConversation
    );

    connect(
        editAction,
        &QAction::triggered,
        this,
        [this]()
        {
            if (!selectedCharacter)
            {
                QMessageBox::information(
                    this,
                    "Edit Conversation",
                    "Please select a character first."
                );

                return;
            }

            if (chatPage->sendButton()->text() == "Stop")
            {
                QMessageBox::information(
                    this,
                    "Edit Conversation",
                    "Please stop the current generation before editing the conversation."
                );

                return;
            }

            if (conversation.isEmpty())
            {
                QMessageBox::information(
                    this,
                    "Edit Conversation",
                    "There is no conversation to edit."
                );

                return;
            }

            QDialog dialog(
                this
            );

            dialog.setWindowTitle(
                "Edit Conversation"
            );

            dialog.resize(
                800,
                700
            );

            auto *dialogLayout =
                new QVBoxLayout(
                    &dialog
                );

            auto *instructions =
                new QLabel(
                    "Edit, delete, or add messages. "
                    "The character instructions are protected."
                );

            instructions->setWordWrap(
                true
            );

            dialogLayout->addWidget(
                instructions
            );

            auto *scrollArea =
                new QScrollArea();

            scrollArea->setWidgetResizable(
                true
            );

            scrollArea->setHorizontalScrollBarPolicy(
                Qt::ScrollBarAlwaysOff
            );

            auto *messageContainer =
                new QWidget();

            auto *messageLayout =
                new QVBoxLayout(
                    messageContainer
                );

            messageLayout->setAlignment(
                Qt::AlignTop
            );

            struct EditorMessage
            {
                QComboBox *role;
                QTextEdit *content;
                QWidget *row;
                bool system;
                bool deleted;
            };

            QVector<EditorMessage> editorMessages;

            auto addMessageEditor =
                [
                    &,
                    messageLayout
                ](
                    const QString &role,
                    const QString &content,
                    bool system
                )
                {
                    auto *row =
                        new QFrame(
                            messageContainer
                        );

                    row->setFrameShape(
                        QFrame::StyledPanel
                    );

                    auto *rowLayout =
                        new QVBoxLayout(
                            row
                        );

                    auto *topLayout =
                        new QHBoxLayout();

                    auto *roleLabel =
                        new QLabel(
                            system
                                ? "System:"
                                : "Role:"
                        );

                    auto *roleCombo =
                        new QComboBox();

                    roleCombo->addItem(
                        "User",
                        "user"
                    );

                    roleCombo->addItem(
                        selectedCharacter
                            ? selectedCharacter->name
                            : "Character",
                        "assistant"
                    );

                    auto *deleteButton =
                        new QPushButton(
                            "Delete"
                        );

                    if (system)
                    {
                        roleCombo->setCurrentIndex(
                            0
                        );

                        roleCombo->setEnabled(
                            false
                        );

                        deleteButton->setEnabled(
                            false
                        );
                    }
                    else if (role == "assistant")
                    {
                        roleCombo->setCurrentIndex(
                            1
                        );
                    }
                    else
                    {
                        roleCombo->setCurrentIndex(
                            0
                        );
                    }

                    topLayout->addWidget(
                        roleLabel
                    );

                    topLayout->addWidget(
                        roleCombo
                    );

                    topLayout->addStretch();

                    topLayout->addWidget(
                        deleteButton
                    );

                    rowLayout->addLayout(
                        topLayout
                    );

                    auto *contentEdit =
                        new QTextEdit();

                    contentEdit->setPlainText(
                        content
                    );

                    contentEdit->setMinimumHeight(
                        100
                    );

                    if (system)
                    {
                        contentEdit->setReadOnly(
                            true
                        );
                    }

                    rowLayout->addWidget(
                        contentEdit
                    );

                    messageLayout->addWidget(
                        row
                    );

                    EditorMessage editorMessage;

                    editorMessage.role =
                        roleCombo;

                    editorMessage.content =
                        contentEdit;

                    editorMessage.row =
                        row;

                    editorMessage.system =
                        system;

                    editorMessage.deleted =
                        false;

                    editorMessages.append(
                        editorMessage
                    );

                    const int editorIndex =
                        editorMessages.size() - 1;

                    connect(
                        deleteButton,
                        &QPushButton::clicked,
                        row,
                        [&, editorIndex]()
                        {
                            if (editorIndex < 0 ||
                                editorIndex >=
                                    editorMessages.size())
                            {
                                return;
                            }

                            editorMessages[
                                editorIndex
                            ].deleted = true;

                            editorMessages[
                                editorIndex
                            ].row->hide();
                        }
                    );
                };

            for (const QJsonValue &value :
                 conversation)
            {
                if (!value.isObject())
                    continue;

                const QJsonObject message =
                    value.toObject();

                const QString role =
                    message.value(
                        "role"
                    ).toString();

                const QString content =
                    message.value(
                        "content"
                    ).toString();

                if (role == "system" ||
                    role == "user" ||
                    role == "assistant")
                {
                    addMessageEditor(
                        role,
                        content,
                        role == "system"
                    );
                }
            }

            scrollArea->setWidget(
                messageContainer
            );

            QTimer::singleShot(
                0,
                scrollArea,
                [scrollArea]()
                {
                    scrollArea->verticalScrollBar()->setValue(
                        scrollArea->verticalScrollBar()->maximum()
                    );
                }
            );

            dialogLayout->addWidget(
                scrollArea
            );

            auto *addMessageButton =
                new QPushButton(
                    "Add Message"
                );

            dialogLayout->addWidget(
                addMessageButton
            );

            connect(
                addMessageButton,
                &QPushButton::clicked,
                &dialog,
                [&addMessageEditor]()
                {
                    addMessageEditor(
                        "user",
                        QString(),
                        false
                    );
                }
            );

            auto *buttonLayout =
                new QHBoxLayout();

            auto *cancelButton =
                new QPushButton(
                    "Cancel"
                );

            auto *applyButton =
                new QPushButton(
                    "Apply Changes"
                );

            buttonLayout->addStretch();

            buttonLayout->addWidget(
                cancelButton
            );

            buttonLayout->addWidget(
                applyButton
            );

            dialogLayout->addLayout(
                buttonLayout
            );

            connect(
                cancelButton,
                &QPushButton::clicked,
                &dialog,
                &QDialog::reject
            );

            connect(
                applyButton,
                &QPushButton::clicked,
                &dialog,
                &QDialog::accept
            );

            if (dialog.exec() != QDialog::Accepted)
                return;

            QJsonArray editedConversation;

            for (const EditorMessage &editorMessage :
                 editorMessages)
            {
                if (editorMessage.deleted)
                    continue;

                const QString content =
                    editorMessage.content
                        ->toPlainText();

                if (content.trimmed().isEmpty())
                    continue;

                QJsonObject message;

                if (editorMessage.system)
                {
                    message["role"] =
                        "system";
                }
                else
                {
                    const QString role =
                        editorMessage.role
                            ->currentData()
                            .toString();

                    if (role != "user" &&
                        role != "assistant")
                    {
                        continue;
                    }

                    message["role"] =
                        role;
                }

                message["content"] =
                    content;

                editedConversation.append(
                    message
                );
            }

            if (editedConversation.isEmpty())
            {
                QMessageBox::warning(
                    this,
                    "Edit Conversation",
                    "The conversation cannot be empty."
                );

                return;
            }

            bool hasSystemMessage =
                false;

            for (const QJsonValue &value :
                 editedConversation)
            {
                if (!value.isObject())
                    continue;

                if (value.toObject()
                        .value("role")
                        .toString() == "system")
                {
                    hasSystemMessage = true;
                    break;
                }
            }

            if (!hasSystemMessage)
            {
                QMessageBox::warning(
                    this,
                    "Edit Conversation",
                    "The character's system instructions must remain in the conversation."
                );

                return;
            }

            conversation =
                editedConversation;

            displayConversation();

            statusBar()->showMessage(
                "Conversation edited."
            );

            chatPage
                ->messageInput()
                ->setFocus();
        }
    );

    connect(
        chatPage,
        &ChatPage::editConversationRequested,
        editAction,
        &QAction::trigger
    );

    connect(
        chatPage,
        &ChatPage::saveConversationRequested,
        saveAction,
        &QAction::trigger
    );

    connect(
        chatPage,
        &ChatPage::loadConversationRequested,
        loadAction,
        &QAction::trigger
    );

}

void MainWindow::setupConnections()
{
    /*
     * Character selection.
     */

    connect(
        charactersPage,
        &CharactersPage::characterSelected,
        this,
        &MainWindow::selectCharacter
    );

    /*
     * Character import is currently handled by the
     * CharactersPage UI. Keep the signal connected
     * so the action has a clear response until the
     * character importer is implemented.
     */

    connect(
        charactersPage,
        &CharactersPage::importRequested,
        this,
        [this]()
        {
            QMessageBox::information(
                this,
                "Import Character",
                "Character import is not available yet."
            );
        }
    );

    /*
     * Greeting selection.
     */

    connect(
        chatPage->greetingSelector(),
        QOverload<int>::of(
            &QComboBox::currentIndexChanged
        ),
        this,
        [this](
            int index
        )
        {
            auto *selector =
                chatPage->greetingSelector();

            if (!selectedCharacter ||
                index < 0)
            {
                return;
            }

            if (selector->property(
                    "updating"
                ).toBool())
            {
                return;
            }

            if (chatPage->sendButton()->text() == "Stop")
            {
                QMessageBox::information(
                    this,
                    "Greeting",
                    "Please stop the current generation before changing the greeting."
                );

                const int previousIndex =
                    selector->property(
                        "previousIndex"
                    ).toInt();

                QSignalBlocker blocker(
                    selector
                );

                selector->setCurrentIndex(
                    previousIndex
                );

                return;
            }

            bool hasVisibleConversation =
                false;

            for (const QJsonValue &value :
                 conversation)
            {
                if (!value.isObject())
                    continue;

                const QString role =
                    value.toObject()
                        .value("role")
                        .toString();

                if (role == "user" ||
                    role == "assistant")
                {
                    hasVisibleConversation = true;
                    break;
                }
            }

            if (hasVisibleConversation)
            {
                const QMessageBox::StandardButton result =
                    QMessageBox::question(
                        this,
                        "Change Greeting",
                        "Changing the greeting will start a new conversation.\n\n"
                        "Do you want to continue?",
                        QMessageBox::Yes |
                        QMessageBox::No
                    );

                if (result != QMessageBox::Yes)
                {
                    const int previousIndex =
                        selector->property(
                            "previousIndex"
                        ).toInt();

                    QSignalBlocker blocker(
                        selector
                    );

                    selector->setCurrentIndex(
                        previousIndex
                    );

                    return;
                }
            }

            selector->setProperty(
                "previousIndex",
                index
            );

            conversation =
                QJsonArray();

            buildCharacterPrompt();

            QString greeting;

            if (index == 0)
            {
                greeting =
                    replacePlaceholders(
                        selectedCharacter->firstMessage
                    );
            }
            else
            {
                const int alternateIndex =
                    index - 1;

                if (alternateIndex >= 0 &&
                    alternateIndex <
                        selectedCharacter
                            ->alternateGreetings
                            .size())
                {
                    greeting =
                        replacePlaceholders(
                            selectedCharacter
                                ->alternateGreetings
                                .at(
                                    alternateIndex
                                )
                        );
                }
            }

            if (!greeting.isEmpty())
            {
                QJsonObject greetingMessage;

                greetingMessage["role"] =
                    "assistant";

                greetingMessage["content"] =
                    greeting;

                conversation.append(
                    greetingMessage
                );
            }

            displayConversation();

            statusBar()->showMessage(
                "Selected greeting: " +
                selectedCharacter->name
            );

            chatPage
                ->messageInput()
                ->clear();

            chatPage
                ->messageInput()
                ->setFocus();

            updateRetryButtonState();
        }
    );

    /*
     * Send / Stop.
     */

    connect(
        chatPage->sendButton(),
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (chatPage->sendButton()->text() == "Stop")
            {
                kobold->abortGeneration();
                return;
            }

            sendMessage();
        }
    );

    /*
     * Retry.
     */

    connect(
        chatPage->retryButton(),
        &QPushButton::clicked,
        this,
        &MainWindow::retryLastResponse
    );

    /*
     * KoboldCpp connection status.
     * SettingsPage owns the actual connection UI.
     */

    connect(
        settingsPage,
        &SettingsPage::connectionStatusChanged,
        this,
        [this](
            bool connected,
            const QString &version
        )
        {
            if (connected)
            {
                statusBar()->showMessage(
                    "KoboldCpp connected - " +
                    version
                );
            }
            else
            {
                statusBar()->showMessage(
                    "KoboldCpp disconnected"
                );
            }
        }
    );

    connect(
        settingsPage,
        &SettingsPage::chatFontSizeChanged,
        this,
        [this](
            int fontSize
        )
        {
            chatPage->setChatFontSize(
                fontSize
            );
        }
    );

chatPage->setChatFontSize(
    settingsPage->chatFontSize()
);

    /*
     * Generation started.
     */

    connect(
        kobold,
        &KoboldClient::generationStarted,
        this,
        [this]()
        {
            chatPage
                ->sendButton()
                ->setEnabled(true);

            chatPage
                ->sendButton()
                ->setText("Stop");

            chatPage
                ->retryButton()
                ->setEnabled(false);

            statusBar()->showMessage(
                "KoboldCpp: Generating..."
            );

            auto *view =
                chatPage->chatView();

            view->moveCursor(
                QTextCursor::End
            );

            const QString characterName =
                selectedCharacter
                    ? selectedCharacter->name
                    : "Assistant";

            chatPage->beginAssistantMessage(
                characterName
            );
        }
    );

    /*
     * Streaming tokens.
     */

    connect(
        kobold,
        &KoboldClient::generationToken,
        this,
        [this](
            const QString &text
        )
        {
            auto *view =
                chatPage->chatView();

            view->insertPlainText(
                text
            );

            view->ensureCursorVisible();
        }
    );

    /*
     * Generation finished.
     */

    connect(
        kobold,
        &KoboldClient::generationFinished,
        this,
        [this](
            const QString &response
        )
        {
            if (!response.isEmpty())
            {
                QJsonObject assistantMessage;

                assistantMessage["role"] =
                    "assistant";

                assistantMessage["content"] =
                    response;

                conversation.append(
                    assistantMessage
                );
            }

            chatPage
                ->sendButton()
                ->setEnabled(true);

            chatPage
                ->sendButton()
                ->setText("Send");

            statusBar()->showMessage(
                "KoboldCpp: Connected"
            );

            displayConversation();
        }
    );

    /*
     * Generation error.
     */

    connect(
        kobold,
        &KoboldClient::generationError,
        this,
        [this](
            const QString &error
        )
        {
            chatPage
                ->chatView()
                ->append(
                    QString(
                        "<b>Error:</b> %1"
                    ).arg(
                        error.toHtmlEscaped()
                    )
                );

            chatPage
                ->sendButton()
                ->setEnabled(true);

            chatPage
                ->sendButton()
                ->setText("Send");

            statusBar()->showMessage(
                "KoboldCpp: Error"
            );

            updateRetryButtonState();
        }
    );
}

void MainWindow::createDefaultCharacter()
{
    defaultCharacter =
        Character();

    defaultCharacter.name =
        "Assistant";

    defaultCharacter.description =
        "A helpful conversational character.";

    defaultCharacter.personality =
        "Friendly, helpful, and conversational.";

    defaultCharacter.firstMessage =
        "Hello! How can I help you today?";

    characters.append(
        defaultCharacter
    );
}

void MainWindow::loadCharacters()
{
    QString charactersPath =
        QDir::current().filePath(
            "characters"
        );

    QDir charactersDirectory(
        charactersPath
    );

    if (!charactersDirectory.exists())
    {
        const QString executableDirectory =
            QCoreApplication::applicationDirPath();

        charactersPath =
            QDir(
                executableDirectory
            ).filePath(
                "../characters"
            );

        charactersDirectory.setPath(
            charactersPath
        );
    }

    characters =
        CharacterLoader::loadCharacters(
            charactersPath
        );

    if (characters.isEmpty())
    {
        createDefaultCharacter();
    }

    charactersPage->setCharacters(
        characters
    );
}

void MainWindow::selectCharacter(
    int index
)
{
    if (index < 0 ||
        index >= characters.size())
    {
        return;
    }

    if (chatPage->sendButton()->text() == "Stop")
    {
        QMessageBox::information(
            this,
            "Character",
            "Please stop the current generation before changing characters."
        );

        return;
    }

    selectedCharacter =
        &characters[index];
        
    chatPage->setCharacterImage(
        selectedCharacter->imagePath
    );

    auto *greetingSelector =
        chatPage->greetingSelector();

    {
        QSignalBlocker blocker(
            greetingSelector
        );

        greetingSelector->clear();

        greetingSelector->addItem(
            "Default Greeting"
        );

        for (int i = 0;
             i <
                 selectedCharacter
                     ->alternateGreetings
                     .size();
             ++i)
        {
            greetingSelector->addItem(
                "Alternate Greeting " +
                QString::number(
                    i + 1
                )
            );
        }

        greetingSelector->setCurrentIndex(
            0
        );

        greetingSelector->setEnabled(
            !selectedCharacter
                ->alternateGreetings
                .isEmpty()
        );

        greetingSelector->setProperty(
            "previousIndex",
            0
        );
    }

    conversation =
        QJsonArray();

    buildCharacterPrompt();

    if (!selectedCharacter->firstMessage.isEmpty())
    {
        QJsonObject greetingMessage;

        greetingMessage["role"] =
            "assistant";

        greetingMessage["content"] =
            replacePlaceholders(
              selectedCharacter->firstMessage
            );

        conversation.append(
            greetingMessage
        );
    }

    displayConversation();

    statusBar()->showMessage(
        "Selected character: " +
        selectedCharacter->name
    );

    chatPage
        ->messageInput()
        ->clear();

    chatPage
        ->messageInput()
        ->setFocus();

    updateRetryButtonState();
}

QString MainWindow::replacePlaceholders(
    const QString &text
) const
{
    QString result = text;

    result.replace(
        "{{user}}",
        settingsPage->userName()
    );

    if (selectedCharacter)
    {
        result.replace(
            "{{char}}",
            selectedCharacter->name
        );
    }

    return result;
}

void MainWindow::buildCharacterPrompt()
{
    if (!selectedCharacter)
        return;

    QString prompt;

    prompt +=
        "You are " +
        selectedCharacter->name +
        ".\n\n";

    prompt +=
        "The user's name is " +
        settingsPage->userName() +
        ".\n\n";

    if (!selectedCharacter->description.isEmpty())
    {
        prompt +=
            "Description:\n" +
            replacePlaceholders(
                selectedCharacter->description
            ) +
            "\n\n";
    }

    if (!selectedCharacter->personality.isEmpty())
    {
        prompt +=
            "Personality:\n" +
            replacePlaceholders(
                selectedCharacter->personality
            ) +
            "\n\n";
    }

    if (!selectedCharacter->scenario.isEmpty())
    {
        prompt +=
            "Scenario:\n" +
            replacePlaceholders(
                selectedCharacter->scenario
            ) +
            "\n\n";
    }

    if (!selectedCharacter->characterSystemPrompt.isEmpty())
    {
        prompt +=
            "Character System Instructions:\n" +
            replacePlaceholders(
                selectedCharacter->characterSystemPrompt
            ) +
            "\n\n";
    }

    prompt +=
        "Stay in character and respond as " +
        selectedCharacter->name +
        ". Do not describe yourself as an AI or "
        "assistant unless the character information "
        "specifically requires it.";

    QJsonObject systemMessage;
    systemMessage["role"] =
        "system";

    systemMessage["content"] =
        prompt;

    conversation.append(
        systemMessage
    );
}

void MainWindow::saveConversation()
{
    if (!selectedCharacter)
    {
        QMessageBox::information(
            this,
            "Save Conversation",
            "Please select a character first."
        );

        return;
    }

    if (conversation.isEmpty())
    {
        QMessageBox::information(
            this,
            "Save Conversation",
            "There is no conversation to save."
        );

        return;
    }

    QString conversationsPath =
        QDir::current().filePath(
            "conversations"
        );

    QDir conversationsDirectory(
        conversationsPath
    );

    if (!conversationsDirectory.exists())
    {
        if (!conversationsDirectory.mkpath("."))
        {
            QMessageBox::warning(
                this,
                "Save Conversation",
                "Could not create the conversations directory."
            );

            return;
        }
    }

    const QString characterDirectoryPath =
        conversationsDirectory.filePath(
            selectedCharacter->name
        );

    QDir characterDirectory(
        characterDirectoryPath
    );

    if (!characterDirectory.exists())
    {
        if (!conversationsDirectory.mkpath(
                selectedCharacter->name
            ))
        {
            QMessageBox::warning(
                this,
                "Save Conversation",
                "Could not create the character conversation directory."
            );

            return;
        }
    }

    const QString suggestedFileName =
        selectedCharacter->name +
        "_conversation.json";

    const QString filePath =
        QFileDialog::getSaveFileName(
            this,
            "Save Conversation",
            characterDirectory.filePath(
                suggestedFileName
            ),
            "JSON Files (*.json)"
        );

    if (filePath.isEmpty())
        return;

    QJsonObject root;

    root["character"] =
        selectedCharacter->name;

    root["conversation"] =
        conversation;

    const QJsonDocument document(
        root
    );

    QFile file(
        filePath
    );

    if (!file.open(
            QIODevice::WriteOnly |
            QIODevice::Truncate
        ))
    {
        QMessageBox::warning(
            this,
            "Save Conversation",
            "Could not save the conversation."
        );

        return;
    }

    file.write(
        document.toJson(
            QJsonDocument::Indented
        )
    );

    file.close();

    statusBar()->showMessage(
        "Conversation saved."
    );
}

void MainWindow::loadConversation()
{
    if (!selectedCharacter)
    {
        QMessageBox::information(
            this,
            "Load Conversation",
            "Please select a character first."
        );

        return;
    }

    QString conversationsPath =
        QDir::current().filePath(
            "conversations"
        );

    QDir conversationsDirectory(
        conversationsPath
    );

    const QString characterDirectoryPath =
        conversationsDirectory.filePath(
            selectedCharacter->name
        );

    QDir characterDirectory(
        characterDirectoryPath
    );

    const QString startingDirectory =
        characterDirectory.exists()
            ? characterDirectoryPath
            : conversationsPath;

    const QString filePath =
        QFileDialog::getOpenFileName(
            this,
            "Load Conversation",
            startingDirectory,
            "JSON Files (*.json)"
        );

    if (filePath.isEmpty())
        return;

    QFile file(
        filePath
    );

    if (!file.open(
            QIODevice::ReadOnly
        ))
    {
        QMessageBox::warning(
            this,
            "Load Conversation",
            "Could not open the conversation file."
        );

        return;
    }

    QJsonParseError parseError;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            file.readAll(),
            &parseError
        );

    file.close();

    if (parseError.error !=
        QJsonParseError::NoError)
    {
        QMessageBox::warning(
            this,
            "Load Conversation",
            "The conversation file contains invalid JSON."
        );

        return;
    }

    if (!document.isObject())
    {
        QMessageBox::warning(
            this,
            "Load Conversation",
            "The conversation file has an invalid format."
        );

        return;
    }

    const QJsonObject root =
        document.object();

    const QString savedCharacter =
        root.value(
            "character"
        ).toString();

    if (savedCharacter !=
        selectedCharacter->name)
    {
        const QMessageBox::StandardButton result =
            QMessageBox::question(
                this,
                "Different Character",
                "This conversation belongs to \"" +
                savedCharacter +
                "\" rather than \"" +
                selectedCharacter->name +
                "\".\n\n"
                "Do you want to load it anyway?",
                QMessageBox::Yes |
                QMessageBox::No
            );

        if (result != QMessageBox::Yes)
            return;
    }

    const QJsonValue conversationValue =
        root.value(
            "conversation"
        );

    if (!conversationValue.isArray())
    {
        QMessageBox::warning(
            this,
            "Load Conversation",
            "The conversation file does not contain a valid conversation."
        );

        return;
    }

    conversation =
        conversationValue.toArray();

    displayConversation();

    statusBar()->showMessage(
        "Conversation loaded."
    );

    chatPage
        ->messageInput()
        ->clear();

    chatPage
        ->messageInput()
        ->setFocus();
}

void MainWindow::displayConversation()
{
    if (!chatPage)
        return;

    chatPage->displayConversation(
        conversation,
        selectedCharacter
            ? selectedCharacter->name
            : QString(),
        settingsPage->userName()
    );

    updateRetryButtonState();
}

void MainWindow::loadGenerationSettings()
{
    if (generationSettings)
    {
        generationSettings->load();
    }
}

void MainWindow::updateGenerationSettingsFromUi()
{
    /*
     * SettingsPage owns the generation controls and
     * continuously saves their values into the shared
     * GenerationSettings instance.
     *
     * Nothing needs to be copied here.
     */
}

void MainWindow::saveGenerationSettings()
{
    if (generationSettings)
    {
        generationSettings->save();
    }
}

void MainWindow::connectToKobold()
{
    /*
     * SettingsPage owns the connection controls and
     * calls KoboldClient::checkConnection().
     *
     * The saved URL is restored by SettingsPage.
     */
    Q_UNUSED(kobold);
}

void MainWindow::sendMessage()
{
    const QString message =
        chatPage
            ->messageInput()
            ->text()
            .trimmed();

    if (message.isEmpty())
        return;

    generationSettings->save();

    QJsonObject userMessage;

    userMessage["role"] =
        "user";

    userMessage["content"] =
        message;

    conversation.append(
        userMessage
    );

    auto *view =
        chatPage->chatView();

    view->moveCursor(
        QTextCursor::End
    );

    chatPage->beginUserMessage(
        settingsPage->userName()
    );

    view->insertPlainText(
        message +
        "\n\n"
    );

    chatPage
        ->messageInput()
        ->clear();

    chatPage
        ->retryButton()
        ->setEnabled(false);

    kobold->generate(
        conversation,
        *generationSettings
    );
}

void MainWindow::updateRetryButtonState()
{
    if (!chatPage ||
        !chatPage->retryButton())
    {
        return;
    }

    if (!selectedCharacter)
    {
        chatPage
            ->retryButton()
            ->setEnabled(false);

        return;
    }

    if (chatPage->sendButton()->text() == "Stop")
    {
        chatPage
            ->retryButton()
            ->setEnabled(false);

        return;
    }

    for (int i = conversation.size() - 1;
         i >= 0;
         --i)
    {
        if (!conversation.at(i).isObject())
            continue;

        const QJsonObject message =
            conversation.at(i).toObject();

        if (message.value("role").toString() ==
            "assistant")
        {
            chatPage
                ->retryButton()
                ->setEnabled(true);

            return;
        }
    }

    chatPage
        ->retryButton()
        ->setEnabled(false);
}

void MainWindow::retryLastResponse()
{
    if (!selectedCharacter)
        return;

    if (chatPage->sendButton()->text() == "Stop")
        return;

    int lastAssistantIndex =
        -1;

    for (int i = conversation.size() - 1;
         i >= 0;
         --i)
    {
        if (!conversation.at(i).isObject())
            continue;

        const QJsonObject message =
            conversation.at(i).toObject();

        if (message.value("role").toString() ==
            "assistant")
        {
            lastAssistantIndex =
                i;

            break;
        }
    }

    if (lastAssistantIndex < 0)
    {
        updateRetryButtonState();
        return;
    }

    while (conversation.size() >
           lastAssistantIndex)
    {
        conversation.removeLast();
    }

    displayConversation();

    generationSettings->save();

    chatPage
        ->retryButton()
        ->setEnabled(false);

    kobold->generate(
        conversation,
        *generationSettings
    );
}
