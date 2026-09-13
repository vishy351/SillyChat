#include "SettingsPage.h"

#include "KoboldClient.h"
#include "GenerationSettings.h"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>


SettingsPage::SettingsPage(
    KoboldClient *kobold,
    GenerationSettings *generationSettings,
    QWidget *parent
)
    : QWidget(parent),
      kobold(kobold),
      generationSettings(generationSettings),
      serverUrlInput(nullptr),
      connectionStatusLabel(nullptr),
      versionLabel(nullptr),
      contextSizeInput(nullptr),
      maxResponseInput(nullptr),
      temperatureInput(nullptr),
      topKInput(nullptr),
      topPInput(nullptr),
      minPInput(nullptr),
      typicalPInput(nullptr),
      tfsInput(nullptr),
      repeatPenaltyInput(nullptr),
      repeatPenaltyRangeInput(nullptr),
      repeatPenaltySlopeInput(nullptr),
      seedInput(nullptr),
      connectButton(nullptr)
{
    /*
     * --------------------------------------------------
     * Main layout
     * --------------------------------------------------
     */

    auto *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        24,
        24,
        24,
        24
    );

    mainLayout->setSpacing(
        18
    );

    /*
     * --------------------------------------------------
     * Backend connection
     * --------------------------------------------------
     */

    auto *connectionGroup =
        new QGroupBox(
            "KoboldCpp Connection",
            this
        );

    auto *connectionLayout =
        new QVBoxLayout(
            connectionGroup
        );

    connectionLayout->setSpacing(
        10
    );

    auto *serverLayout =
        new QHBoxLayout();

    auto *serverLabel =
        new QLabel(
            "Server URL:",
            connectionGroup
        );

    serverUrlInput =
        new QLineEdit(
            connectionGroup
        );

    serverUrlInput->setPlaceholderText(
        "http://127.0.0.1:5001"
    );

    connectButton =
        new QPushButton(
            "Connect",
            connectionGroup
        );

    serverLayout->addWidget(
        serverLabel
    );

    serverLayout->addWidget(
        serverUrlInput,
        1
    );

    serverLayout->addWidget(
        connectButton
    );

    connectionLayout->addLayout(
        serverLayout
    );

    auto *statusLayout =
        new QHBoxLayout();

    connectionStatusLabel =
        new QLabel(
            "Disconnected",
            connectionGroup
        );

    versionLabel =
        new QLabel(
            "Version: Unknown",
            connectionGroup
        );

    statusLayout->addWidget(
        connectionStatusLabel
    );

    statusLayout->addStretch();

    statusLayout->addWidget(
        versionLabel
    );

    connectionLayout->addLayout(
        statusLayout
    );

    mainLayout->addWidget(
        connectionGroup
    );

    /*
     * --------------------------------------------------
     * Generation settings
     * --------------------------------------------------
     */

    auto *generationGroup =
        new QGroupBox(
            "Generation",
            this
        );

    auto *settingsLayout =
        new QGridLayout(
            generationGroup
        );

    settingsLayout->setHorizontalSpacing(
        14
    );

    settingsLayout->setVerticalSpacing(
        10
    );

    /*
     * Context Size
     */

    settingsLayout->addWidget(
        new QLabel(
            "Context Size",
            generationGroup
        ),
        0,
        0
    );

    contextSizeInput =
        new QSpinBox(
            generationGroup
        );

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

    settingsLayout->addWidget(
        contextSizeInput,
        0,
        1
    );

    /*
     * Max Response
     */

    settingsLayout->addWidget(
        new QLabel(
            "Max Response",
            generationGroup
        ),
        0,
        2
    );

    maxResponseInput =
        new QSpinBox(
            generationGroup
        );

    maxResponseInput->setRange(
        1,
        1048576
    );

    maxResponseInput->setSingleStep(
        1
    );

    settingsLayout->addWidget(
        maxResponseInput,
        0,
        3
    );

    /*
     * Temperature
     */

    settingsLayout->addWidget(
        new QLabel(
            "Temperature",
            generationGroup
        ),
        1,
        0
    );

    temperatureInput =
        new QDoubleSpinBox(
            generationGroup
        );

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

    /*
     * Top K
     */

    settingsLayout->addWidget(
        new QLabel(
            "Top K",
            generationGroup
        ),
        1,
        2
    );

    topKInput =
        new QSpinBox(
            generationGroup
        );

    topKInput->setRange(
        0,
        100000
    );

    settingsLayout->addWidget(
        topKInput,
        1,
        3
    );

    /*
     * Top P
     */

    settingsLayout->addWidget(
        new QLabel(
            "Top P",
            generationGroup
        ),
        2,
        0
    );

    topPInput =
        new QDoubleSpinBox(
            generationGroup
        );

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

    /*
     * Min P
     */

    settingsLayout->addWidget(
        new QLabel(
            "Min P",
            generationGroup
        ),
        2,
        2
    );

    minPInput =
        new QDoubleSpinBox(
            generationGroup
        );

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

    /*
     * Typical P
     */

    settingsLayout->addWidget(
        new QLabel(
            "Typical P",
            generationGroup
        ),
        3,
        0
    );

    typicalPInput =
        new QDoubleSpinBox(
            generationGroup
        );

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

    /*
     * TFS
     */

    settingsLayout->addWidget(
        new QLabel(
            "TFS",
            generationGroup
        ),
        3,
        2
    );

    tfsInput =
        new QDoubleSpinBox(
            generationGroup
        );

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

    /*
     * Repeat Penalty
     */

    settingsLayout->addWidget(
        new QLabel(
            "Repeat Penalty",
            generationGroup
        ),
        4,
        0
    );

    repeatPenaltyInput =
        new QDoubleSpinBox(
            generationGroup
        );

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

    /*
     * Repeat Range
     */

    settingsLayout->addWidget(
        new QLabel(
            "Repeat Range",
            generationGroup
        ),
        4,
        2
    );

    repeatPenaltyRangeInput =
        new QSpinBox(
            generationGroup
        );

    repeatPenaltyRangeInput->setRange(
        0,
        1048576
    );

    settingsLayout->addWidget(
        repeatPenaltyRangeInput,
        4,
        3
    );

    /*
     * Penalty Slope
     */

    settingsLayout->addWidget(
        new QLabel(
            "Penalty Slope",
            generationGroup
        ),
        5,
        0
    );

    repeatPenaltySlopeInput =
        new QDoubleSpinBox(
            generationGroup
        );

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

    /*
     * Seed
     */

    settingsLayout->addWidget(
        new QLabel(
            "Seed",
            generationGroup
        ),
        5,
        2
    );

    seedInput =
        new QSpinBox(
            generationGroup
        );

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

    mainLayout->addStretch();

    /*
     * --------------------------------------------------
     * Load saved settings
     * --------------------------------------------------
     */

    loadSettings();

    /*
     * --------------------------------------------------
     * KoboldCpp connection status
     * --------------------------------------------------
     */

    if (kobold)
    {
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
                    connectionStatusLabel->setText(
                        "Connected"
                    );

                    versionLabel->setText(
                        "Version: " +
                        version
                    );

                    connectButton->setText(
                        "Reconnect"
                    );
                }
                else
                {
                    connectionStatusLabel->setText(
                        "Disconnected"
                    );

                    versionLabel->setText(
                        "Version: Unknown"
                    );

                    contextSizeInput->setValue(
                        0
                    );

                    connectButton->setText(
                        "Connect"
                    );
                }

                emit connectionStatusChanged(
                    connected,
                    version
                );
            }
        );

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
    }

    /*
     * --------------------------------------------------
     * Connect button
     * --------------------------------------------------
     */

    connect(
        connectButton,
        &QPushButton::clicked,
        this,
        &SettingsPage::connectToKobold
    );

    /*
     * --------------------------------------------------
     * Save generation settings when changed
     * --------------------------------------------------
     */

    connect(
        maxResponseInput,
        &QSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        temperatureInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        topKInput,
        &QSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        topPInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        minPInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        typicalPInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        tfsInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        repeatPenaltyInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        repeatPenaltyRangeInput,
        &QSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        repeatPenaltySlopeInput,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );

    connect(
        seedInput,
        &QSpinBox::valueChanged,
        this,
        [this]()
        {
            saveSettings();
        }
    );
}


void SettingsPage::loadSettings()
{
    if (generationSettings)
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

    if (kobold)
    {
        kobold->setServerUrl(
            savedUrl
        );
    }
}


void SettingsPage::saveSettings()
{
    if (!generationSettings)
        return;

    updateGenerationSettingsFromUi();

    generationSettings->save();
}


void SettingsPage::updateGenerationSettingsFromUi()
{
    if (!generationSettings)
        return;

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


void SettingsPage::connectToKobold()
{
    QString url =
        serverUrlInput->text().trimmed();

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

    settings.sync();

    if (!kobold)
        return;

    kobold->setServerUrl(
        url
    );

    connectionStatusLabel->setText(
        "Connecting..."
    );

    versionLabel->setText(
        "Version: Unknown"
    );

    contextSizeInput->setValue(
        0
    );

    kobold->checkConnection();
}

