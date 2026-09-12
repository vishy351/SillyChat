#pragma once

#include "Character.h"

#include <QList>
#include <QWidget>

class QScrollArea;
class QWidget;

class CharactersPage : public QWidget
{
    Q_OBJECT

public:
    explicit CharactersPage(QWidget *parent = nullptr);

    void setCharacters(
        const QList<Character> &characters
    );

signals:
    void characterSelected(
        int index
    );

    void importRequested();

private:
    void rebuildCharacterGrid();

    QList<Character> m_characters;

    QScrollArea *m_scrollArea;
    QWidget *m_characterContainer;
};

