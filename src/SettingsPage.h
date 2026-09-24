#pragma once

#include <QWidget>

class QLineEdit;
class QLabel;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;

class KoboldClient;
class GenerationSettings;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(
        KoboldClient *kobold,
        GenerationSettings *generationSettings,
        QWidget *parent = nullptr
    );
    
    int chatFontSize() const;

    QString userName() const;

signals:
    void connectionStatusChanged(
        bool connected,
        const QString &version
    );

    void chatFontSizeChanged(
        int fontSize
    );

private:
    void loadSettings();
    void saveSettings();

    void connectToKobold();

    void updateGenerationSettingsFromUi();

    KoboldClient *kobold;
    GenerationSettings *generationSettings;

    QLineEdit *serverUrlInput;
    QLineEdit *userNameInput;

    QLabel *connectionStatusLabel;
    QLabel *versionLabel;
    QLabel *chatFontPreview;

    QSpinBox *contextSizeInput;
    QSpinBox *maxResponseInput;
    QSpinBox *chatFontSizeInput;

    QDoubleSpinBox *temperatureInput;

    QSpinBox *topKInput;

    QDoubleSpinBox *topPInput;
    QDoubleSpinBox *minPInput;
    QDoubleSpinBox *typicalPInput;
    QDoubleSpinBox *tfsInput;

    QDoubleSpinBox *repeatPenaltyInput;
    QSpinBox *repeatPenaltyRangeInput;
    QDoubleSpinBox *repeatPenaltySlopeInput;

    QSpinBox *seedInput;

    QPushButton *connectButton;
};

