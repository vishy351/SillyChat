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

signals:
    void connectionStatusChanged(
        bool connected,
        const QString &version
    );

private:
    void loadSettings();
    void saveSettings();

    void connectToKobold();

    void updateGenerationSettingsFromUi();

    KoboldClient *kobold;
    GenerationSettings *generationSettings;

    QLineEdit *serverUrlInput;

    QLabel *connectionStatusLabel;
    QLabel *versionLabel;

    QSpinBox *contextSizeInput;
    QSpinBox *maxResponseInput;

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

