#include "ChatPage.h"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextImageFormat>
#include <QTextTable>
#include <QTextTableCellFormat>
#include <QTextTableFormat>
#include <QTextFormat>
#include <QTextLength>
#include <QVBoxLayout>
#include <QUrl>

class ChatTextEdit : public QTextEdit
{
public:
    explicit ChatTextEdit(
        QWidget *parent = nullptr
    )
        : QTextEdit(parent)
    {
    }

protected:
    void mousePressEvent(
        QMouseEvent *event
    ) override
    {
        QTextCursor cursor =
            cursorForPosition(
                event->pos()
            );

        QTextCharFormat format =
            cursor.charFormat();

        if (format.objectType() ==
            QTextFormat::ImageObject)
        {
            const QTextImageFormat imageFormat =
                format.toImageFormat();

            if (imageFormat.isValid())
            {
                const QString imageName =
                    imageFormat.name();

                QUrl imageUrl(
                    imageName
                );

                if (imageUrl.isValid() &&
                    imageUrl.scheme() ==
                        "sillychat")
                {
                    /*
                     * The image displayed in the
                     * chat is the styled thumbnail.
                     *
                     * For the popup, retrieve the
                     * original full-resolution image.
                     */
                    const QUrl popupUrl(
                        "sillychat://character-image"
                    );

                    QVariant imageResource =
                        document()->resource(
                            QTextDocument::ImageResource,
                            popupUrl
                        );

                    if (imageResource.isValid())
                    {
                        QImage image =
                            qvariant_cast<QImage>(
                                imageResource
                            );

                        if (!image.isNull())
                        {
                            showImagePopup(
                                image
                            );

                            return;
                        }
                    }
                }
            }
        }

        QTextEdit::mousePressEvent(
            event
        );
    }

private:
    void showImagePopup(
        const QImage &image
    )
    {
        QDialog dialog(
            this
        );

        dialog.setWindowTitle(
            "Character"
        );

        dialog.setModal(
            true
        );

        dialog.setMinimumSize(
            400,
            400
        );

        auto *layout =
            new QVBoxLayout(
                &dialog
            );

        layout->setContentsMargins(
            12,
            12,
            12,
            12
        );

        auto *imageLabel =
            new QLabel(
                &dialog
            );

        imageLabel->setAlignment(
            Qt::AlignCenter
        );

        imageLabel->setSizePolicy(
            QSizePolicy::Expanding,
            QSizePolicy::Expanding
        );

        const QPixmap pixmap =
            QPixmap::fromImage(
                image
            );

        imageLabel->setPixmap(
            pixmap.scaled(
                700,
                700,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            )
        );

        layout->addWidget(
            imageLabel,
            1
        );

        auto *closeButton =
            new QPushButton(
                "Close",
                &dialog
            );

        closeButton->setFixedWidth(
            90
        );

        layout->addWidget(
            closeButton,
            0,
            Qt::AlignCenter
        );

        QObject::connect(
            closeButton,
            &QPushButton::clicked,
            &dialog,
            &QDialog::accept
        );

        dialog.exec();
    }
};

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
        new ChatTextEdit();

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

    connect(
        m_messageInput,
        &QLineEdit::returnPressed,
        m_sendButton,
        &QPushButton::click
    );

    m_messageInput->setFocus();
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

void ChatPage::setCharacterImage(
    const QString &imagePath
)
{
    m_characterImagePath =
        imagePath;
}

void ChatPage::beginUserMessage(const QString &userName)
{
    m_chatView->moveCursor(
        QTextCursor::End
    );

    if (m_chatView->document()->characterCount() > 1)
    {
        m_chatView->insertPlainText(
            "\n"
        );
    }

    QTextTableFormat tableFormat;

    tableFormat.setBorder(
        0
    );

    tableFormat.setCellPadding(
        0
    );

    tableFormat.setCellSpacing(
        0
    );

    QVector<QTextLength> columnWidths;

    columnWidths.append(
        QTextLength(
            QTextLength::FixedLength,
            175
        )
    );

    columnWidths.append(
        QTextLength(
            QTextLength::PercentageLength,
            100
        )
    );

    tableFormat.setColumnWidthConstraints(
        columnWidths
    );

    QTextTable *table =
        m_chatView->textCursor()
            .insertTable(
                1,
                2,
                tableFormat
            );

    /*
     * User profile cell.
     */
    QTextTableCell profileCell =
        table->cellAt(
            0,
            0
        );

    QTextTableCellFormat profileFormat;

    profileFormat.setVerticalAlignment(
        QTextCharFormat::AlignMiddle
    );

    profileCell.setFormat(
        profileFormat
    );

    QTextCursor profileCursor =
        profileCell.firstCursorPosition();

    QTextBlockFormat centerFormat;

    centerFormat.setAlignment(
        Qt::AlignHCenter
    );

    profileCursor.setBlockFormat(
        centerFormat
    );

    /*
     * Simple built-in user avatar.
     */
    QTextCharFormat avatarFormat;

    avatarFormat.setFontPointSize(
        42
    );

    profileCursor.setCharFormat(
        avatarFormat
    );

    profileCursor.insertText(
        "●"
    );

    profileCursor.insertText(
        "\n"
    );

    /*
     * User name.
     */
    QTextCharFormat nameFormat;

    nameFormat.setFontWeight(
        QFont::Bold
    );

    nameFormat.setFontPointSize(
        10
    );

    profileCursor.setCharFormat(
        nameFormat
    );

    profileCursor.insertText(
        userName
    );

    /*
     * User message cell.
     */
    QTextTableCell responseCell =
        table->cellAt(
            0,
            1
        );

    QTextTableCellFormat responseFormat;

    responseFormat.setVerticalAlignment(
        QTextCharFormat::AlignMiddle
    );

    responseFormat.setLeftPadding(
        16
    );

    responseFormat.setRightPadding(
        8
    );

    responseCell.setFormat(
        responseFormat
    );

    QTextCursor responseCursor =
        responseCell.firstCursorPosition();

    QTextBlockFormat responseBlockFormat;

    responseBlockFormat.setAlignment(
        Qt::AlignLeft
    );

    responseCursor.setBlockFormat(
        responseBlockFormat
    );

    m_chatView->setTextCursor(
        responseCursor
    );

    m_chatView->ensureCursorVisible();
}

void ChatPage::beginAssistantMessage(
    const QString &characterName
)
{
    m_chatView->moveCursor(
        QTextCursor::End
    );

    if (m_chatView->document()->characterCount() > 1)
    {
        m_chatView->insertPlainText(
            "\n"
        );
    }

    if (!m_characterImagePath.isEmpty())
    {
        QImage image;

        if (image.load(
                m_characterImagePath))
        {
            /*
             * Two separate resources are used:
             *
             * 1. thumbnailUrl
             *    Styled portrait shown in chat.
             *
             * 2. popupUrl
             *    Original full-resolution image
             *    used by the popup.
             */
            const QUrl thumbnailUrl(
                "sillychat://character-thumbnail"
            );

            const QUrl popupUrl(
                "sillychat://character-image"
            );

            /*
             * Maximum portrait size.
             *
             * The original aspect ratio is
             * always preserved.
             */
            const int portraitMaximum =
                150;

            const double scale =
                qMin(
                    static_cast<double>(
                        portraitMaximum
                    ) /
                    image.width(),

                    static_cast<double>(
                        portraitMaximum
                    ) /
                    image.height()
                );

            const int portraitWidth =
                qMax(
                    1,
                    qRound(
                        image.width() *
                        scale
                    )
                );

            const int portraitHeight =
                qMax(
                    1,
                    qRound(
                        image.height() *
                        scale
                    )
                );

            /*
             * Extra canvas space for the
             * rounded corners and shadow.
             */
            const int radius =
                10;

            const int shadowOffset =
                3;

            const int padding =
                4;

            const int styledWidth =
                portraitWidth +
                padding * 2 +
                shadowOffset;

            const int styledHeight =
                portraitHeight +
                padding * 2 +
                shadowOffset;

            /*
             * Scale the original image while
             * preserving its aspect ratio.
             */
            QImage scaledImage =
                image.scaled(
                    portraitWidth,
                    portraitHeight,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                );

            /*
             * Transparent canvas containing
             * the rounded portrait and shadow.
             */
            QImage styledImage(
                styledWidth,
                styledHeight,
                QImage::Format_ARGB32_Premultiplied
            );

            styledImage.fill(
                Qt::transparent
            );

            QPainter painter(
                &styledImage
            );

            painter.setRenderHint(
                QPainter::Antialiasing,
                true
            );

            /*
             * Rounded rectangle path for
             * the actual portrait.
             */
            QPainterPath portraitPath;

            portraitPath.addRoundedRect(
                QRectF(
                    padding,
                    padding,
                    scaledImage.width(),
                    scaledImage.height()
                ),
                radius,
                radius
            );

            /*
             * Soft shadow.
             */
            painter.save();

            painter.setOpacity(
                0.28
            );

            painter.setBrush(
                QColor(
                    0,
                    0,
                    0,
                    170
                )
            );

            painter.setPen(
                Qt::NoPen
            );

            painter.drawRoundedRect(
                QRectF(
                    padding + shadowOffset,
                    padding + shadowOffset,
                    scaledImage.width(),
                    scaledImage.height()
                ),
                radius,
                radius
            );

            painter.restore();

            /*
             * Clip the portrait to rounded
             * corners.
             */
            painter.save();

            painter.setClipPath(
                portraitPath
            );

            painter.drawImage(
                padding,
                padding,
                scaledImage
            );

            painter.restore();

            painter.end();

            /*
             * Store the styled thumbnail.
             */
            m_chatView->document()->addResource(
                QTextDocument::ImageResource,
                thumbnailUrl,
                styledImage
            );

            /*
             * Store the original image separately.
             *
             * This is what the popup will use,
             * preventing the enlarged image from
             * becoming blurry.
             */
            m_chatView->document()->addResource(
                QTextDocument::ImageResource,
                popupUrl,
                image
            );

            /*
             * Message row.
             *
             * First column:
             * character profile.
             *
             * Second column:
             * assistant response.
             */
            QTextTableFormat tableFormat;

            tableFormat.setBorder(
                0
            );

            tableFormat.setCellPadding(
                0
            );

            tableFormat.setCellSpacing(
                0
            );

            /*
             * Give the profile enough room
             * for the portrait and name.
             */
            QVector<QTextLength> columnWidths;

            columnWidths.append(
                QTextLength(
                    QTextLength::FixedLength,
                    175
                )
            );

            columnWidths.append(
                QTextLength(
                    QTextLength::PercentageLength,
                    100
                )
            );

            tableFormat.setColumnWidthConstraints(
                columnWidths
            );

            QTextTable *table =
                m_chatView->textCursor()
                    .insertTable(
                        1,
                        2,
                        tableFormat
                    );

            /*
             * Profile cell.
             */
            QTextTableCell profileCell =
                table->cellAt(
                    0,
                    0
                );

            QTextTableCellFormat profileFormat;

            profileFormat.setVerticalAlignment(
                QTextCharFormat::AlignMiddle
            );

            profileCell.setFormat(
                profileFormat
            );

            QTextCursor profileCursor =
                profileCell.firstCursorPosition();

            /*
             * Center the portrait and name
             * horizontally.
             */
            QTextBlockFormat centerFormat;

            centerFormat.setAlignment(
                Qt::AlignHCenter
            );

            profileCursor.setBlockFormat(
                centerFormat
            );

            /*
             * Insert styled portrait.
             */
            QTextImageFormat imageFormat;

            imageFormat.setName(
                thumbnailUrl.toString()
            );

            imageFormat.setWidth(
                styledImage.width()
            );

            imageFormat.setHeight(
                styledImage.height()
            );

            profileCursor.insertImage(
                imageFormat
            );

            /*
             * Small gap between portrait
             * and character name.
             */
            profileCursor.insertText(
                "\n"
            );

            /*
             * Character name.
             */
            QTextCharFormat nameFormat;

            nameFormat.setFontWeight(
                QFont::Bold
            );

            nameFormat.setFontPointSize(
                10
            );

            profileCursor.setCharFormat(
                nameFormat
            );

            profileCursor.insertText(
                characterName
            );

            /*
             * Response cell.
             */
            QTextTableCell responseCell =
                table->cellAt(
                    0,
                    1
                );

            QTextTableCellFormat responseFormat;

            responseFormat.setVerticalAlignment(
                QTextCharFormat::AlignMiddle
            );

            responseFormat.setLeftPadding(
                16
            );

            responseFormat.setRightPadding(
                8
            );

            responseCell.setFormat(
                responseFormat
            );

            QTextCursor responseCursor =
                responseCell.firstCursorPosition();
                
            m_chatView->setTextCursor(responseCursor);

            /*
             * Keep assistant text left aligned.
             */
            QTextBlockFormat responseBlockFormat;

            responseBlockFormat.setAlignment(
                Qt::AlignLeft
            );

            responseCursor.setBlockFormat(
                responseBlockFormat
            );

            m_chatView->setTextCursor(
                responseCursor
            );
        }
        else
        {
            m_chatView->insertPlainText(
                characterName +
                ": "
            );
        }
    }
    else
    {
        m_chatView->insertPlainText(
            characterName +
            ": "
        );
    }

    m_chatView->ensureCursorVisible();
}

void ChatPage::displayConversation(
    const QJsonArray &conversation,
    const QString &characterName,
    const QString &userName
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
            beginUserMessage(userName);

            m_chatView->insertPlainText(
                content
            );

            m_chatView->insertPlainText(
                "\n\n"
            );
        }
        else if (role == "assistant")
        {
            beginAssistantMessage(
                characterName
            );

            m_chatView->insertPlainText(
                content
            );

            m_chatView->insertPlainText(
                "\n\n"
            );
        }
    }

    m_chatView->moveCursor(
        QTextCursor::End
    );

    m_chatView->ensureCursorVisible();
}

