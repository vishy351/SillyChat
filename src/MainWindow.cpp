#include "MainWindow.h"
#include "KoboldClient.h"
#include "GenerationSettings.h"
#include "CharacterLoader.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QWidget>
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QToolButton>
#include <QPixmap>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSizePolicy>
#include <QCoreApplication>
#include <QIcon>
#include <QSize>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialog>
#include <QComboBox>
#include <QFrame>
#include <QVector>
#include <QSignalBlocker>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      generationSettings(
          new GenerationSettings()
      ),
      selectedCharacter(nullptr)
{
    setWindowTitle(
        "SillyChat"
    );

    resize(
        1000,
        800
    );

    auto *centralWidget =
        new QWidget(this);

    setCentralWidget(
        centralWidget
    );

    auto *mainLayout =
        new QVBoxLayout(
            centralWidget
        );

    /*
     * --------------------------------------------------
     * KoboldCpp connection
     * --------------------------------------------------
     */

    auto *serverLayout =
        new QHBoxLayout();

    auto *serverLabel =
        new QLabel(
            "KoboldCpp API:"
        );

    serverUrlInput =
        new QLineEdit();

    serverUrlInput->setPlaceholderText(
        "http://127.0.0.1:5001"
    );

    connectButton =
        new QPushButton(
            "Connect"
        );

    serverLayout->addWidget(
        serverLabel
    );

    serverLayout->addWidget(
        serverUrlInput
    );

    serverLayout->addWidget(
        connectButton
    );

    mainLayout->addLayout(
        serverLayout
    );

    /*
     * --------------------------------------------------
     * Status
     * --------------------------------------------------
     */

    auto *statusLayout =
        new QHBoxLayout();

    statusLabel =
        new QLabel(
            "KoboldCpp: Disconnected"
        );

    modelLabel =
        new QLabel(
            "Version: Unknown"
        );

    statusLayout->addWidget(
        statusLabel
    );

    statusLayout->addStretch();

    statusLayout->addWidget(
        modelLabel
    );

    mainLayout->addLayout(
        statusLayout
    );

    /*
     * --------------------------------------------------
     * Character selection
     * --------------------------------------------------
     */

    characterGroup =
        new QGroupBox(
            "Characters"
        );

    auto *characterGroupLayout =
        new QVBoxLayout(
            characterGroup
        );

    characterScrollArea =
        new QScrollArea();

    characterScrollArea->setWidgetResizable(
        true
    );

    characterScrollArea->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
    );

    characterContainer =
        new QWidget();

    characterScrollArea->setWidget(
        characterContainer
    );

    characterGroupLayout->addWidget(
        characterScrollArea
    );

    /*
     * Greeting selector.
     *
     * This is created as a child of characterGroup so
     * selectCharacter() can find it without requiring
     * another MainWindow member in MainWindow.h.
     *
     * V1 cards will have one entry: Default.
     * V2 cards can additionally contain alternate
     * greetings.
     */

    auto *greetingLayout =
        new QHBoxLayout();

    auto *greetingLabel =
        new QLabel(
            "Greeting:"
        );

    auto *greetingSelector =
        new QComboBox(
            characterGroup
        );

    greetingSelector->setObjectName(
        "greetingSelector"
    );

    greetingSelector->setEnabled(
        false
    );

    greetingSelector->setToolTip(
        "Choose the greeting used when starting a new conversation."
    );

    greetingLayout->addWidget(
        greetingLabel
    );

    greetingLayout->addWidget(
        greetingSelector
    );

    characterGroupLayout->addLayout(
        greetingLayout
    );

    mainLayout->addWidget(
        characterGroup
    );

    loadCharacters();

    /*
     * Changing the greeting starts a fresh conversation
     * for the currently selected character.
     *
     * Index 0 is always the normal first_mes greeting.
     * Additional entries are V2 alternate greetings.
     */

    connect(
        greetingSelector,
        QOverload<int>::of(
            &QComboBox::currentIndexChanged
        ),
        this,
        [this, greetingSelector](
            int index
        )
        {
            if (!selectedCharacter)
                return;

            if (index < 0)
                return;

            if (greetingSelector->property(
                    "updating"
                ).toBool())
            {
                return;
            }

            /*
             * Do not interrupt an active generation.
             */

            if (sendButton &&
                sendButton->text() == "Stop")
            {
                QMessageBox::information(
                    this,
                    "Greeting",
                    "Please stop the current generation before changing the greeting."
                );

                const int previousIndex =
                    greetingSelector->property(
                        "previousIndex"
                    ).toInt();

                QSignalBlocker blocker(
                    greetingSelector
                );

                greetingSelector->setCurrentIndex(
                    previousIndex
                );

                return;
            }

            /*
             * Ask before throwing away an existing
             * conversation.
             */

            if (!conversation.isEmpty())
            {
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
                            greetingSelector->property(
                                "previousIndex"
                            ).toInt();

                        QSignalBlocker blocker(
                            greetingSelector
                        );

                        greetingSelector->setCurrentIndex(
                            previousIndex
                        );

                        return;
                    }
                }
            }

            greetingSelector->setProperty(
                "previousIndex",
                index
            );

            conversation =
                QJsonArray();

            chatView->clear();

            /*
             * The system instructions are rebuilt first.
             */

            buildCharacterPrompt();

            /*
             * Retrieve the selected greeting.
             */

            QString greeting;

            if (index == 0)
            {
                greeting =
                    selectedCharacter->firstMessage;
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
                        selectedCharacter
                            ->alternateGreetings
                            .at(
                                alternateIndex
                            );
                }
            }

            /*
             * Add the selected greeting as the
             * initial assistant message.
             */

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

                chatView->moveCursor(
                    QTextCursor::End
                );

                chatView->insertPlainText(
                    selectedCharacter->name +
                    ": " +
                    greeting +
                    "\n"
                );
            }

            statusLabel->setText(
                "Selected greeting: " +
                selectedCharacter->name
            );

            messageInput->clear();

            messageInput->setFocus();

            updateRetryButtonState();
        }
    );

    /*
     * --------------------------------------------------
     * Generation settings
     * --------------------------------------------------
     */

    generationGroup =
        new QGroupBox(
            "Generation Settings"
        );

    auto *settingsLayout =
        new QGridLayout(
            generationGroup
        );

    settingsLayout->addWidget(
        new QLabel("Context Size"),
        0,
        0
    );

    contextSizeInput =
        new QSpinBox();

    contextSizeInput->setRange(
        0,
        1048576
    );

    contextSizeInput->setSpecialValueText(
        "Unknown"
    );

    contextSizeInput->setReadOnly(
        true
    );

    contextSizeInput->setButtonSymbols(
        QAbstractSpinBox::NoButtons
    );

    contextSizeInput->setValue(
        0
    );

    settingsLayout->addWidget(
        contextSizeInput,
        0,
        1
    );

    settingsLayout->addWidget(
        new QLabel("Max Response"),
        0,
        2
    );

    maxResponseInput =
        new QSpinBox();

    maxResponseInput->setRange(
        1,
        1048576
    );

    maxResponseInput->setSingleStep(
        64
    );

    settingsLayout->addWidget(
        maxResponseInput,
        0,
        3
    );

    settingsLayout->addWidget(
        new QLabel("Temperature"),
        1,
        0
    );

    temperatureInput =
        new QDoubleSpinBox();

    temperatureInput->setRange(
        0.0,
        5.0
    );

    temperatureInput->setSingleStep(
        0.05
    );

    temperatureInput->setDecimals(
        2
    );

    settingsLayout->addWidget(
        temperatureInput,
        1,
        1
    );

    settingsLayout->addWidget(
        new QLabel("Top K"),
        1,
        2
    );

    topKInput =
        new QSpinBox();

    topKInput->setRange(
        0,
        100000
    );

    settingsLayout->addWidget(
        topKInput,
        1,
        3
    );

    settingsLayout->addWidget(
        new QLabel("Top P"),
        2,
        0
    );

    topPInput =
        new QDoubleSpinBox();

    topPInput->setRange(
        0.0,
        1.0
    );

    topPInput->setSingleStep(
        0.01
    );

    topPInput->setDecimals(
        2
    );

    settingsLayout->addWidget(
        topPInput,
        2,
        1
    );

    settingsLayout->addWidget(
        new QLabel("Min P"),
        2,
        2
    );

    minPInput =
        new QDoubleSpinBox();

    minPInput->setRange(
        0.0,
        1.0
    );

    minPInput->setSingleStep(
        0.01
    );

    minPInput->setDecimals(
        2
    );

    settingsLayout->addWidget(
        minPInput,
        2,
        3
    );

    settingsLayout->addWidget(
        new QLabel("Typical P"),
        3,
        0
    );

    typicalPInput =
        new QDoubleSpinBox();

    typicalPInput->setRange(
        0.0,
        1.0
    );

    typicalPInput->setSingleStep(
        0.01
    );

    typicalPInput->setDecimals(
        2
    );

    settingsLayout->addWidget(
        typicalPInput,
        3,
        1
    );

    settingsLayout->addWidget(
        new QLabel("TFS"),
        3,
        2
    );

    tfsInput =
        new QDoubleSpinBox();

    tfsInput->setRange(
        0.0,
        1.0
    );

    tfsInput->setSingleStep(
        0.01
    );

    tfsInput->setDecimals(
        2
    );

    settingsLayout->addWidget(
        tfsInput,
        3,
        3
    );

    settingsLayout->addWidget(
        new QLabel("Repeat Penalty"),
        4,
        0
    );

    repeatPenaltyInput =
        new QDoubleSpinBox();

    repeatPenaltyInput->setRange(
        0.0,
        5.0
    );

    repeatPenaltyInput->setSingleStep(
        0.01
    );

    repeatPenaltyInput->setDecimals(
        2
    );

    settingsLayout->addWidget(
        repeatPenaltyInput,
        4,
        1
    );

    settingsLayout->addWidget(
        new QLabel("Repeat Range"),
        4,
        2
    );

    repeatPenaltyRangeInput =
        new QSpinBox();

    repeatPenaltyRangeInput->setRange(
        0,
        1048576
    );

    settingsLayout->addWidget(
        repeatPenaltyRangeInput,
        4,
        3
    );

    settingsLayout->addWidget(
        new QLabel("Penalty Slope"),
        5,
        0
    );

    repeatPenaltySlopeInput =
        new QDoubleSpinBox();

    repeatPenaltySlopeInput->setRange(
        0.0,
        5.0
    );

    repeatPenaltySlopeInput->setSingleStep(
        0.05
    );

    repeatPenaltySlopeInput->setDecimals(
        2
    );

    settingsLayout->addWidget(
        repeatPenaltySlopeInput,
        5,
        1
    );

    settingsLayout->addWidget(
        new QLabel("Seed"),
        5,
        2
    );

    seedInput =
        new QSpinBox();

    seedInput->setRange(
        -1,
        2147483647
    );

    settingsLayout->addWidget(
        seedInput,
        5,
        3
    );

    mainLayout->addWidget(
        generationGroup
    );

    loadGenerationSettings();

    /*
     * --------------------------------------------------
     * Chat
     * --------------------------------------------------
     */

    chatView =
        new QTextEdit();

    chatView->setReadOnly(
        true
    );

    chatView->setPlaceholderText(
        "Conversation will appear here..."
    );

    mainLayout->addWidget(
        chatView
    );

    /*
     * --------------------------------------------------
     * Conversation buttons
     * --------------------------------------------------
     */

    auto *conversationButtonLayout =
        new QHBoxLayout();

    saveConversationButton =
        new QPushButton(
            "Save Conversation"
        );

    loadConversationButton =
        new QPushButton(
            "Load Conversation"
        );

    auto *editConversationButton =
        new QPushButton(
            "Edit Conversation"
        );

    retryButton =
        new QPushButton(
            "Retry"
        );

    conversationButtonLayout->addWidget(
        saveConversationButton
    );

    conversationButtonLayout->addWidget(
        loadConversationButton
    );

    conversationButtonLayout->addWidget(
        editConversationButton
    );

    conversationButtonLayout->addWidget(
        retryButton
    );

    conversationButtonLayout->addStretch();

    mainLayout->addLayout(
        conversationButtonLayout
    );

    /*
     * --------------------------------------------------
     * Input
     * --------------------------------------------------
     */

    auto *inputLayout =
        new QHBoxLayout();

    messageInput =
        new QLineEdit();

    messageInput->setPlaceholderText(
        "Message..."
    );

    sendButton =
        new QPushButton(
            "Send"
        );

    inputLayout->addWidget(
        messageInput
    );

    inputLayout->addWidget(
        sendButton
    );

    mainLayout->addLayout(
        inputLayout
    );

    /*
     * --------------------------------------------------
     * Kobold client
     * --------------------------------------------------
     */

    kobold =
        new KoboldClient(
            this
        );

    /*
     * --------------------------------------------------
     * Saved API address
     * --------------------------------------------------
     */

    QSettings settings(
        "SillyChat",
        "SillyChat"
    );

    const QString savedUrl =
        settings.value(
            "koboldcpp/serverUrl",
            "http://127.0.0.1:5001"
        ).toString();

    serverUrlInput->setText(
        savedUrl
    );

    kobold->setServerUrl(
        savedUrl
    );

    /*
     * --------------------------------------------------
     * Initial Retry state
     * --------------------------------------------------
     */

    updateRetryButtonState();

    /*
     * --------------------------------------------------
     * Connection status
     * --------------------------------------------------
     */

    connect(
        kobold,
        &KoboldClient::connectionChanged,
        this,
        [this](
            bool connected,
            const QString &version
        )
        {
            if (connected)
            {
                statusLabel->setText(
                    "KoboldCpp: Connected"
                );

                modelLabel->setText(
                    "Version: " +
                    version
                );

                connectButton->setText(
                    "Reconnect"
                );
            }
            else
            {
                statusLabel->setText(
                    "KoboldCpp: Disconnected"
                );

                modelLabel->setText(
                    "Version: Unknown"
                );

                contextSizeInput->setValue(
                    0
                );

                connectButton->setText(
                    "Connect"
                );
            }
        }
    );

    /*
     * --------------------------------------------------
     * Context size
     * --------------------------------------------------
     */

    connect(
        kobold,
        &KoboldClient::contextSizeChanged,
        this,
        [this](
            int contextSize
        )
        {
            contextSizeInput->setValue(
                contextSize
            );
        }
    );

    /*
     * --------------------------------------------------
     * Generation started
     * --------------------------------------------------
     */

    connect(
        kobold,
        &KoboldClient::generationStarted,
        this,
        [this]()
        {
            sendButton->setEnabled(
                true
            );

            sendButton->setText(
                "Stop"
            );

            retryButton->setEnabled(
                false
            );

            statusLabel->setText(
                "KoboldCpp: Generating..."
            );

            chatView->moveCursor(
                QTextCursor::End
            );

            const QString characterName =
                selectedCharacter
                    ? selectedCharacter->name
                    : "Assistant";

            chatView->insertPlainText(
                "\n"
            );

            chatView->insertPlainText(
                characterName +
                ": "
            );
        }
    );

    /*
     * --------------------------------------------------
     * Streaming tokens
     * --------------------------------------------------
     */

    connect(
        kobold,
        &KoboldClient::generationToken,
        this,
        [this](
            const QString &text
        )
        {
            chatView->moveCursor(
                QTextCursor::End
            );

            chatView->insertPlainText(
                text
            );

            chatView->ensureCursorVisible();
        }
    );

    /*
     * --------------------------------------------------
     * Generation finished
     * --------------------------------------------------
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

            sendButton->setEnabled(
                true
            );

            sendButton->setText(
                "Send"
            );

            statusLabel->setText(
                "KoboldCpp: Connected"
            );

            displayConversation();
        }
    );

    /*
     * --------------------------------------------------
     * Generation error
     * --------------------------------------------------
     */

    connect(
        kobold,
        &KoboldClient::generationError,
        this,
        [this](
            const QString &error
        )
        {
            chatView->append(
                QString(
                    "<b>Error:</b> %1"
                ).arg(
                    error.toHtmlEscaped()
                )
            );

            sendButton->setEnabled(
                true
            );

            sendButton->setText(
                "Send"
            );

            statusLabel->setText(
                "KoboldCpp: Error"
            );

            updateRetryButtonState();
        }
    );

    /*
     * --------------------------------------------------
     * Connect button
     * --------------------------------------------------
     */

    connect(
        connectButton,
        &QPushButton::clicked,
        this,
        &MainWindow::connectToKobold
    );

    /*
     * --------------------------------------------------
     * Send / Stop button
     * --------------------------------------------------
     */

    connect(
        sendButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (sendButton->text() == "Stop")
            {
                kobold->abortGeneration();

                return;
            }

            sendMessage();
        }
    );

    /*
     * --------------------------------------------------
     * Enter to send / stop
     * --------------------------------------------------
     */

    connect(
        messageInput,
        &QLineEdit::returnPressed,
        sendButton,
        &QPushButton::click
    );

    /*
     * --------------------------------------------------
     * Save conversation
     * --------------------------------------------------
     */

    connect(
        saveConversationButton,
        &QPushButton::clicked,
        this,
        &MainWindow::saveConversation
    );

    /*
     * --------------------------------------------------
     * Load conversation
     * --------------------------------------------------
     */

    connect(
        loadConversationButton,
        &QPushButton::clicked,
        this,
        &MainWindow::loadConversation
    );

    /*
     * --------------------------------------------------
     * Retry last AI response
     * --------------------------------------------------
     */

    connect(
        retryButton,
        &QPushButton::clicked,
        this,
        &MainWindow::retryLastResponse
    );

    /*
     * --------------------------------------------------
     * Edit conversation
     *
     * The editor works on a temporary copy.
     * Nothing changes in the actual conversation
     * until Apply Changes is pressed.
     *
     * Deleted messages are hidden rather than being
     * destroyed immediately. This prevents dangling
     * widget pointers while the editor is open.
     * --------------------------------------------------
     */

    connect(
        editConversationButton,
        &QPushButton::clicked,
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

            if (sendButton->text() == "Stop")
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

            QDialog dialog(this);

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
                            "Role:"
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
                        roleLabel->setText(
                            "System:"
                        );

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

                        contentEdit->setPlaceholderText(
                            "Protected system instructions"
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

                            editorMessages[editorIndex].deleted =
                                true;

                            editorMessages[editorIndex].row->hide();
                        }
                    );
                };

            for (const QJsonValue &value : conversation)
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

                if (role == "system")
                {
                    addMessageEditor(
                        "system",
                        content,
                        true
                    );
                }
                else if (role == "user")
                {
                    addMessageEditor(
                        "user",
                        content,
                        false
                    );
                }
                else if (role == "assistant")
                {
                    addMessageEditor(
                        "assistant",
                        content,
                        false
                    );
                }
            }

            scrollArea->setWidget(
                messageContainer
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
                [
                    &addMessageEditor
                ]()
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
                {
                    continue;
                }

                if (!editorMessage.row)
                {
                    continue;
                }

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

            bool hasSystemMessage = false;

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

            statusLabel->setText(
                "Conversation edited."
            );

            messageInput->setFocus();
        }
    );

    /*
     * --------------------------------------------------
     * Save settings whenever they change.
     * --------------------------------------------------
     */

    connect(
        maxResponseInput,
        &QSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        temperatureInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        topKInput,
        &QSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        topPInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        minPInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        typicalPInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        tfsInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        repeatPenaltyInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        repeatPenaltyRangeInput,
        &QSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        repeatPenaltySlopeInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    connect(
        seedInput,
        &QSpinBox::valueChanged,
        this,
        [this]()
        {
            saveGenerationSettings();
        }
    );

    /*
     * Automatically connect.
     */

    connectToKobold();
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

    auto *layout =
        new QGridLayout(
            characterContainer
        );

    layout->setAlignment(
        Qt::AlignTop | Qt::AlignLeft
    );

    layout->setHorizontalSpacing(
        16
    );

    layout->setVerticalSpacing(
        16
    );

    const int columns = 5;

    for (int i = 0;
         i < characters.size();
         ++i)
    {
        const Character &character =
            characters.at(i);

        auto *button =
            new QToolButton(
                characterContainer
            );

        button->setText(
            character.name
        );

        button->setToolButtonStyle(
            Qt::ToolButtonTextUnderIcon
        );

        button->setIconSize(
            QSize(
                140,
                140
            )
        );

        button->setFixedSize(
            170,
            190
        );

        button->setSizePolicy(
            QSizePolicy::Fixed,
            QSizePolicy::Fixed
        );

        if (!character.imagePath.isEmpty())
        {
            QPixmap image(
                character.imagePath
            );

            if (!image.isNull())
            {
                button->setIcon(
                    QIcon(
                        image
                    )
                );
            }
        }

        connect(
            button,
            &QToolButton::clicked,
            this,
            [this, i]()
            {
                selectCharacter(i);
            }
        );

        const int row =
            i / columns;

        const int column =
            i % columns;

        layout->addWidget(
            button,
            row,
            column
        );
    }

    if (characters.isEmpty())
    {
        auto *label =
            new QLabel(
                "No characters found."
            );

        label->setAlignment(
            Qt::AlignCenter
        );

        layout->addWidget(
            label,
            0,
            0
        );
    }
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

    if (sendButton &&
        sendButton->text() == "Stop")
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

    /*
     * Update the greeting selector.
     */

    auto *greetingSelector =
        characterGroup
            ? characterGroup->findChild<QComboBox *>(
                "greetingSelector"
            )
            : nullptr;

    if (greetingSelector)
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

    /*
     * Start a completely new conversation.
     */

    conversation =
        QJsonArray();

    chatView->clear();

    /*
     * Add the character's instructions
     * as a system message.
     */

    buildCharacterPrompt();

    /*
     * Add the default character greeting.
     */

    if (!selectedCharacter->firstMessage.isEmpty())
    {
        QJsonObject greetingMessage;

        greetingMessage["role"] =
            "assistant";

        greetingMessage["content"] =
            selectedCharacter->firstMessage;

        conversation.append(
            greetingMessage
        );

        chatView->moveCursor(
            QTextCursor::End
        );

        chatView->insertPlainText(
            selectedCharacter->name +
            ": " +
            selectedCharacter->firstMessage +
            "\n"
        );
    }

    statusLabel->setText(
        "Selected character: " +
        selectedCharacter->name
    );

    messageInput->clear();

    messageInput->setFocus();

    updateRetryButtonState();
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

    if (!selectedCharacter->description.isEmpty())
    {
        prompt +=
            "Description:\n" +
            selectedCharacter->description +
            "\n\n";
    }

    if (!selectedCharacter->personality.isEmpty())
    {
        prompt +=
            "Personality:\n" +
            selectedCharacter->personality +
            "\n\n";
    }

    if (!selectedCharacter->scenario.isEmpty())
    {
        prompt +=
            "Scenario:\n" +
            selectedCharacter->scenario +
            "\n\n";
    }

    /*
     * V2 character cards may provide their own
     * system_prompt. Use it as additional character
     * instructions rather than replacing the basic
     * character information above.
     */

    if (!selectedCharacter->characterSystemPrompt.isEmpty())
    {
        prompt +=
            "Character System Instructions:\n" +
            selectedCharacter->characterSystemPrompt +
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

    statusLabel->setText(
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

    QString startingDirectory =
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

    statusLabel->setText(
        "Conversation loaded."
    );

    messageInput->clear();

    messageInput->setFocus();
}

void MainWindow::displayConversation()
{
    chatView->clear();

    if (!selectedCharacter)
    {
        updateRetryButtonState();

        return;
    }

    for (const QJsonValue &value : conversation)
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

        if (content.isEmpty())
            continue;

        if (role == "system")
        {
            continue;
        }

        if (role == "user")
        {
            chatView->append(
                "You: " +
                content.toHtmlEscaped()
            );
        }
        else if (role == "assistant")
        {
            chatView->append(
                selectedCharacter->name +
                ": " +
                content.toHtmlEscaped()
            );
        }
    }

    chatView->moveCursor(
        QTextCursor::End
    );

    chatView->ensureCursorVisible();

    updateRetryButtonState();
}

void MainWindow::loadGenerationSettings()
{
    generationSettings->load();

    maxResponseInput->setValue(
        generationSettings->maxResponse
    );

    temperatureInput->setValue(
        generationSettings->temperature
    );

    topKInput->setValue(
        generationSettings->topK
    );

    topPInput->setValue(
        generationSettings->topP
    );

    minPInput->setValue(
        generationSettings->minP
    );

    typicalPInput->setValue(
        generationSettings->typicalP
    );

    tfsInput->setValue(
        generationSettings->tfs
    );

    repeatPenaltyInput->setValue(
        generationSettings->repeatPenalty
    );

    repeatPenaltyRangeInput->setValue(
        generationSettings->repeatPenaltyRange
    );

    repeatPenaltySlopeInput->setValue(
        generationSettings->repeatPenaltySlope
    );

    seedInput->setValue(
        generationSettings->seed
    );
}

void MainWindow::updateGenerationSettingsFromUi()
{
    generationSettings->maxResponse =
        maxResponseInput->value();

    generationSettings->temperature =
        temperatureInput->value();

    generationSettings->topK =
        topKInput->value();

    generationSettings->topP =
        topPInput->value();

    generationSettings->minP =
        minPInput->value();

    generationSettings->typicalP =
        typicalPInput->value();

    generationSettings->tfs =
        tfsInput->value();

    generationSettings->repeatPenalty =
        repeatPenaltyInput->value();

    generationSettings->repeatPenaltyRange =
        repeatPenaltyRangeInput->value();

    generationSettings->repeatPenaltySlope =
        repeatPenaltySlopeInput->value();

    generationSettings->seed =
        seedInput->value();
}

void MainWindow::saveGenerationSettings()
{
    if (!generationSettings)
        return;

    updateGenerationSettingsFromUi();

    generationSettings->save();
}

void MainWindow::connectToKobold()
{
    QString url =
        serverUrlInput->text()
            .trimmed();

    if (url.isEmpty())
    {
        url =
            "http://127.0.0.1:5001";

        serverUrlInput->setText(
            url
        );
    }

    while (url.endsWith('/'))
    {
        url.chop(1);
    }

    serverUrlInput->setText(
        url
    );

    QSettings settings(
        "SillyChat",
        "SillyChat"
    );

    settings.setValue(
        "koboldcpp/serverUrl",
        url
    );

    kobold->setServerUrl(
        url
    );

    statusLabel->setText(
        "KoboldCpp: Connecting..."
    );

    modelLabel->setText(
        "Version: Unknown"
    );

    contextSizeInput->setValue(
        0
    );

    kobold->checkConnection();
}

void MainWindow::sendMessage()
{
    const QString message =
        messageInput->text()
            .trimmed();

    if (message.isEmpty())
        return;

    if (!selectedCharacter)
    {
        chatView->append(
            "<b>Please select a character first.</b>"
        );

        return;
    }

    updateGenerationSettingsFromUi();

    generationSettings->save();

    /*
     * Add the user's message.
     */

    QJsonObject userMessage;

    userMessage["role"] =
        "user";

    userMessage["content"] =
        message;

    conversation.append(
        userMessage
    );

    /*
     * Display the user's message.
     */

    chatView->moveCursor(
        QTextCursor::End
    );

    chatView->insertPlainText(
        "\nYou: " +
        message +
        "\n\n"
    );

    messageInput->clear();

    /*
     * Retry is unavailable while a new response
     * is being generated.
     */

    retryButton->setEnabled(
        false
    );

    /*
     * Send the complete conversation.
     */

    kobold->generate(
        conversation,
        *generationSettings
    );
}

void MainWindow::updateRetryButtonState()
{
    if (!retryButton)
        return;

    if (!selectedCharacter)
    {
        retryButton->setEnabled(
            false
        );

        return;
    }

    /*
     * Retry is unavailable while KoboldCpp is
     * generating a response.
     */

    if (sendButton &&
        sendButton->text() == "Stop")
    {
        retryButton->setEnabled(
            false
        );

        return;
    }

    /*
     * Find out whether there is currently at
     * least one assistant response.
     *
     * We search backwards because Retry always
     * targets the latest assistant response
     * that currently exists in the conversation.
     */

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
            retryButton->setEnabled(
                true
            );

            return;
        }
    }

    retryButton->setEnabled(
        false
    );
}

void MainWindow::retryLastResponse()
{
    if (!selectedCharacter)
        return;

    if (sendButton->text() == "Stop")
        return;

    /*
     * Find the latest assistant response currently
     * present in the conversation.
     */

    int lastAssistantIndex = -1;

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
            lastAssistantIndex = i;
            break;
        }
    }

    if (lastAssistantIndex < 0)
    {
        updateRetryButtonState();

        return;
    }

    /*
     * Remove the latest assistant response and
     * everything that follows it.
     *
     * This creates the exact conversation state
     * that existed immediately before that AI
     * response was generated.
     */

    while (conversation.size() >
           lastAssistantIndex)
    {
        conversation.removeLast();
    }

    displayConversation();

    /*
     * Use the current generation settings.
     */

    updateGenerationSettingsFromUi();

    generationSettings->save();

    /*
     * The conversation now ends immediately before
     * the old assistant response, so KoboldCpp
     * generates a completely new response.
     */

    retryButton->setEnabled(
        false
    );

    kobold->generate(
        conversation,
        *generationSettings
    );
}

