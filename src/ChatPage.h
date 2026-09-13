#pragma once

#include <QJsonArray>
#include <QWidget>

class QLineEdit;
class QPushButton;
class QTextEdit;
class QComboBox;

class ChatPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPage(
        QWidget *parent = nullptr
    );

    QTextEdit *chatView() const;

    QLineEdit *messageInput() const;

    QPushButton *sendButton() const;

    QPushButton *retryButton() const;

    QPushButton *saveConversationButton() const;

    QPushButton *loadConversationButton() const;

    QPushButton *editConversationButton() const;

    QComboBox *greetingSelector() const;

    void displayConversation(
        const QJsonArray &conversation,
        const QString &characterName
    );

signals:
    void saveConversationRequested();

    void loadConversationRequested();

    void editConversationRequested();

private:
    QTextEdit *m_chatView;

    QLineEdit *m_messageInput;

    QPushButton *m_sendButton;

    QPushButton *m_retryButton;

    QPushButton *m_saveConversationButton;

    QPushButton *m_loadConversationButton;

    QPushButton *m_editConversationButton;

    QComboBox *m_greetingSelector;
};
