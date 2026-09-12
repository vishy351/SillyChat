#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QJsonArray>

class GenerationSettings;

class KoboldClient : public QObject
{
    Q_OBJECT

public:
    explicit KoboldClient(QObject *parent = nullptr);

    void setServerUrl(const QString &url);
    QString serverUrl() const;

    void checkConnection();

    void generate(
        const QJsonArray &messages,
        const GenerationSettings &settings
    );

    void abortGeneration();

signals:
    void connectionChanged(
        bool connected,
        const QString &version
    );

    void contextSizeChanged(
        int contextSize
    );

    void generationStarted();

    void generationToken(
        const QString &text
    );

    void generationFinished(
        const QString &text
    );

    void generationError(
        const QString &error
    );

private:
    QNetworkAccessManager networkManager;

    QString m_serverUrl;

    QNetworkReply *generationReply = nullptr;

    QString streamedText;
    QByteArray streamBuffer;

    bool generationActive = false;

    void processStreamData(
        const QByteArray &data
    );

    void processStreamLine(
        const QByteArray &line
    );
};

