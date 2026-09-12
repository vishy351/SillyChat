#include "ChatPage.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonValue>


ChatPage::ChatPage(
    QWidget *parent
)
    : QWidget(parent),
      m_chatView(nullptr),
      m_messageInput(nullptr),
      m_sendButton(nullptr),
      m_retryButton(nullptr),
      m_greetingSelector(nullptr)
{
    /*
     * --------------------------------------------------
     * Main layout
     * --------------------------------------------------
     */

    auto *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        20,
        20,
        20,
        20
    );

    mainLayout->setSpacing(
        12
    );

    /*
     * --------------------------------------------------
     * Conversation header
     * --------------------------------------------------
     */

    auto *headerLayout =
        new QHBoxLayout();

    auto *titleLabel =
        new QLabel(
            "Chat",
            this
        );

    QFont titleFont =
        titleLabel->font();

    titleFont.setPointSize(
        16
    );

    titleFont.setBold(
        true
    );

    titleLabel->setFont(
        titleFont
    );

    headerLayout->addWidget(
        titleLabel
    );

    headerLayout->addStretch();

    auto *greetingLabel =
        new QLabel(
            "Greeting:",
            this
        );

    m_greetingSelector =
        new QComboBox(
            this
        );

    m_greetingSelector->setMinimumWidth(
        180
    );

    m_greetingSelector->addItem(
        "Default Greeting"
    );

    m_greetingSelector->setEnabled(
        false
    );

    headerLayout->addWidget(
        greetingLabel
    );

    headerLayout->addWidget(
        m_greetingSelector
    );

    mainLayout->addLayout(
        headerLayout
    );

    /*
     * --------------------------------------------------
     * Chat view
     * --------------------------------------------------
     */

    m_chatView =
        new QTextEdit(
            this
        );

    m_chatView->setReadOnly(
        true
    );

    m_chatView->setAcceptRichText(
        true
    );

    m_chatView->setPlaceholderText(
        "Your conversation will appear here..."
    );

    m_chatView->setMinimumHeight(
        300
    );

    mainLayout->addWidget(
        m_chatView,
        1
    );

    /*
     * --------------------------------------------------
     * Conversation controls
     * --------------------------------------------------
     */

    auto *conversationLayout =
        new QHBoxLayout();

    m_retryButton =
        new QPushButton(
            "Retry",
            this
        );

    m_retryButton->setEnabled(
        false
    );

    conversationLayout->addWidget(
        m_retryButton
    );

    conversationLayout->addStretch();

    mainLayout->addLayout(
        conversationLayout
    );

    /*
     * --------------------------------------------------
     * Message input
     * --------------------------------------------------
     */

    auto *inputFrame =
        new QFrame(
            this
        );

    inputFrame->setFrameShape(
        QFrame::StyledPanel
    );

    auto *inputLayout =
        new QHBoxLayout(
            inputFrame
        );

    inputLayout->setContentsMargins(
        8,
        8,
        8,
        8
    );

    m_messageInput =
        new QLineEdit(
            inputFrame
        );

    m_messageInput->setPlaceholderText(
        "Message..."
    );

    m_messageInput->setMinimumHeight(
        38
    );

    m_sendButton =
        new QPushButton(
            "Send",
            inputFrame
        );

    m_sendButton->setMinimumWidth(
        80
    );

    m_sendButton->setMinimumHeight(
        38
    );

    inputLayout->addWidget(
        m_messageInput,
        1
    );

    inputLayout->addWidget(
        m_sendButton
    );

    mainLayout->addWidget(
        inputFrame
    );

    /*
     * --------------------------------------------------
     * Enter sends the message.
     *
     * MainWindow remains responsible for deciding
     * what happens when the button is pressed.
     * --------------------------------------------------
     */

    connect(
        m_messageInput,
        &QLineEdit::returnPressed,
        m_sendButton,
        &QPushButton::click
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

        if (content.isEmpty())
            continue;

        /*
         * System instructions are never shown
         * in the normal chat view.
         */

        if (role == "system")
        {
            continue;
        }

        if (role == "user")
        {
            m_chatView->append(
                "You: " +
                content.toHtmlEscaped()
            );
        }
        else if (role == "assistant")
        {
            const QString displayName =
                characterName.isEmpty()
                    ? "Assistant"
                    : characterName;

            m_chatView->append(
                displayName +
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

