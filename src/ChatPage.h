#pragma once

#include <QJsonArray>
#include <QWidget>

class QLineEdit;
class QPushButton;
class QTextEdit;
class QLabel;
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

    QComboBox *greetingSelector() const;

    void displayConversation(
        const QJsonArray &conversation,
        const QString &characterName
    );

private:
    QTextEdit *m_chatView;

    QLineEdit *m_messageInput;

    QPushButton *m_sendButton;

    QPushButton *m_retryButton;

    QComboBox *m_greetingSelector;
};

