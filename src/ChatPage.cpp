#include "ChatPage.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent),
      m_chatView(nullptr),
      m_messageInput(nullptr),
      m_sendButton(nullptr),
      m_retryButton(nullptr),
      m_saveConversationButton(nullptr),
      m_loadConversationButton(nullptr),
      m_editConversationButton(nullptr),
      m_greetingSelector(nullptr)
{
    auto *mainLayout =
        new QVBoxLayout(this);

    /*
     * Greeting selector
     */

    auto *greetingLayout =
        new QHBoxLayout();

    auto *greetingLabel =
        new QLabel(
            "Greeting:"
        );

    m_greetingSelector =
        new QComboBox();

    m_greetingSelector->setObjectName(
        "greetingSelector"
    );

    m_greetingSelector->setEnabled(
        false
    );

    greetingLayout->addWidget(
        greetingLabel
    );

    greetingLayout->addWidget(
        m_greetingSelector
    );

    mainLayout->addLayout(
        greetingLayout
    );

    /*
     * Chat history controls
     *
     * Save and Load stay above the
     * conversation history.
     */

    auto *topButtonLayout =
        new QHBoxLayout();

    m_saveConversationButton =
        new QPushButton(
            "Save Chat"
        );

    m_loadConversationButton =
        new QPushButton(
            "Load Chat"
        );

    topButtonLayout->addWidget(
        m_saveConversationButton
    );

    topButtonLayout->addWidget(
        m_loadConversationButton
    );

    topButtonLayout->addStretch();

    mainLayout->addLayout(
        topButtonLayout
    );

    /*
     * Chat history
     */

    m_chatView =
        new QTextEdit();

    m_chatView->setReadOnly(
        true
    );

    m_chatView->setPlaceholderText(
        "Conversation will appear here..."
    );

    mainLayout->addWidget(
        m_chatView
    );

    /*
     * Conversation controls
     *
     * Edit is next to Retry, below
     * the conversation history.
     */

    auto *conversationButtonLayout =
        new QHBoxLayout();

    m_editConversationButton =
        new QPushButton(
            "Edit Chat"
        );

    m_retryButton =
        new QPushButton(
            "Retry"
        );

    conversationButtonLayout->addWidget(
        m_editConversationButton
    );

    conversationButtonLayout->addWidget(
        m_retryButton
    );

    conversationButtonLayout->addStretch();

    mainLayout->addLayout(
        conversationButtonLayout
    );

    /*
     * Message input
     */

    auto *inputLayout =
        new QHBoxLayout();

    m_messageInput =
        new QLineEdit();

    m_messageInput->setPlaceholderText(
        "Message..."
    );

    m_sendButton =
        new QPushButton(
            "Send"
        );

    inputLayout->addWidget(
        m_messageInput
    );

    inputLayout->addWidget(
        m_sendButton
    );

    mainLayout->addLayout(
        inputLayout
    );

    /*
     * Button signals
     */

    connect(
        m_saveConversationButton,
        &QPushButton::clicked,
        this,
        &ChatPage::saveConversationRequested
    );

    connect(
        m_loadConversationButton,
        &QPushButton::clicked,
        this,
        &ChatPage::loadConversationRequested
    );

    connect(
        m_editConversationButton,
        &QPushButton::clicked,
        this,
        &ChatPage::editConversationRequested
    );
}

QTextEdit *ChatPage::chatView() const
{
    return m_chatView;
}

QLineEdit *ChatPage::messageInput() const
{
    return m_messageInput;
}

QPushButton *ChatPage::sendButton() const
{
    return m_sendButton;
}

QPushButton *ChatPage::retryButton() const
{
    return m_retryButton;
}

QPushButton *ChatPage::saveConversationButton() const
{
    return m_saveConversationButton;
}

QPushButton *ChatPage::loadConversationButton() const
{
    return m_loadConversationButton;
}

QPushButton *ChatPage::editConversationButton() const
{
    return m_editConversationButton;
}

QComboBox *ChatPage::greetingSelector() const
{
    return m_greetingSelector;
}

void ChatPage::displayConversation(
    const QJsonArray &conversation,
    const QString &characterName
)
{
    m_chatView->clear();

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
            continue;

        if (role == "user")
        {
            m_chatView->append(
                "You: " +
                content.toHtmlEscaped()
            );
        }
        else if (role == "assistant")
        {
            m_chatView->append(
                characterName +
                ": " +
                content.toHtmlEscaped()
            );
        }
    }

    m_chatView->moveCursor(
        QTextCursor::End
    );

    m_chatView->ensureCursorVisible();
}
