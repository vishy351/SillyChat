#include "CharactersPage.h"

#include <QFont>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QSizePolicy>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

CharactersPage::CharactersPage(
    QWidget *parent
)
    : QWidget(parent),
      m_scrollArea(nullptr),
      m_characterContainer(nullptr)
{
    auto *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        16,
        16,
        16,
        16
    );

    mainLayout->setSpacing(
        12
    );

    /*
     * --------------------------------------------------
     * Header
     * --------------------------------------------------
     */

    auto *headerLayout =
        new QHBoxLayout();

    auto *titleLabel =
        new QLabel(
            "Characters"
        );

    QFont titleFont =
        titleLabel->font();

    titleFont.setPointSize(
        titleFont.pointSize() + 3
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

    auto *importButton =
        new QPushButton(
            "Import Character"
        );

    headerLayout->addWidget(
        importButton
    );

    mainLayout->addLayout(
        headerLayout
    );

    connect(
        importButton,
        &QPushButton::clicked,
        this,
        &CharactersPage::importRequested
    );

    /*
     * --------------------------------------------------
     * Character grid
     * --------------------------------------------------
     */

    m_scrollArea =
        new QScrollArea();

    m_scrollArea->setWidgetResizable(
        true
    );

    m_scrollArea->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
    );

    m_characterContainer =
        new QWidget();

    m_scrollArea->setWidget(
        m_characterContainer
    );

    mainLayout->addWidget(
        m_scrollArea,
        1
    );
}

void CharactersPage::setCharacters(
    const QList<Character> &characters
)
{
    m_characters =
        characters;

    rebuildCharacterGrid();
}

void CharactersPage::rebuildCharacterGrid()
{
    if (!m_characterContainer)
        return;

    auto *oldLayout =
        m_characterContainer->layout();

    if (oldLayout)
    {
        QLayoutItem *item;

        while (
            (item = oldLayout->takeAt(0))
            != nullptr
        )
        {
            if (item->widget())
            {
                item->widget()->deleteLater();
            }

            delete item;
        }

        delete oldLayout;
    }

    auto *layout =
        new QGridLayout(
            m_characterContainer
        );

    layout->setAlignment(
        Qt::AlignTop | Qt::AlignLeft
    );

    layout->setHorizontalSpacing(
        18
    );

    layout->setVerticalSpacing(
        18
    );

    layout->setContentsMargins(
        8,
        8,
        8,
        8
    );

    if (m_characters.isEmpty())
    {
        auto *emptyLabel =
            new QLabel(
                "No character cards found.\n\n"
                "Import a character card to get started."
            );

        emptyLabel->setAlignment(
            Qt::AlignCenter
        );

        emptyLabel->setMinimumHeight(
            180
        );

        layout->addWidget(
            emptyLabel,
            0,
            0
        );

        return;
    }

    const int columns = 5;

    for (int i = 0;
         i < m_characters.size();
         ++i)
    {
        const Character &character =
            m_characters.at(i);

        auto *button =
            new QToolButton(
                m_characterContainer
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

        button->setCursor(
            Qt::PointingHandCursor
        );

        if (!character.imagePath.isEmpty())
        {
            QPixmap image(
                character.imagePath
            );

            if (!image.isNull())
            {
                button->setIcon(
                    QIcon(image)
                );
            }
        }

        connect(
            button,
            &QToolButton::clicked,
            this,
            [this, i]()
            {
                emit characterSelected(
                    i
                );
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
}

