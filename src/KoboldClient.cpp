#include "KoboldClient.h"
#include "GenerationSettings.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QUrl>

KoboldClient::KoboldClient(QObject *parent)
    : QObject(parent),
      m_serverUrl("http://127.0.0.1:5001")
{
}

void KoboldClient::setServerUrl(
    const QString &url
)
{
    m_serverUrl = url.trimmed();

    while (m_serverUrl.endsWith('/'))
    {
        m_serverUrl.chop(1);
    }
}

QString KoboldClient::serverUrl() const
{
    return m_serverUrl;
}

void KoboldClient::checkConnection()
{
    /*
     * First check the KoboldCpp version.
     */

    QUrl versionUrl(
        m_serverUrl +
        "/api/extra/version"
    );

    QNetworkRequest versionRequest(
        versionUrl
    );

    QNetworkReply *versionReply =
        networkManager.get(
            versionRequest
        );

    connect(
        versionReply,
        &QNetworkReply::finished,
        this,
        [this, versionReply]()
        {
            if (versionReply->error() !=
                QNetworkReply::NoError)
            {
                emit connectionChanged(
                    false,
                    QString()
                );

                versionReply->deleteLater();

                return;
            }

            const QByteArray data =
                versionReply->readAll();

            QJsonParseError error;

            const QJsonDocument document =
                QJsonDocument::fromJson(
                    data,
                    &error
                );

            if (error.error !=
                    QJsonParseError::NoError ||
                !document.isObject())
            {
                emit connectionChanged(
                    false,
                    QString()
                );

                versionReply->deleteLater();

                return;
            }

            const QJsonObject object =
                document.object();

            const QString version =
                object.value(
                    "version"
                ).toString();

            emit connectionChanged(
                true,
                version
            );

            versionReply->deleteLater();

            /*
             * Now retrieve the actual context size
             * selected when KoboldCpp was launched.
             */

            QUrl contextUrl(
                m_serverUrl +
                "/api/extra/true_max_context_length"
            );

            QNetworkRequest contextRequest(
                contextUrl
            );

            QNetworkReply *contextReply =
                networkManager.get(
                    contextRequest
                );

            connect(
                contextReply,
                &QNetworkReply::finished,
                this,
                [this, contextReply]()
                {
                    if (contextReply->error() !=
                        QNetworkReply::NoError)
                    {
                        contextReply->deleteLater();

                        return;
                    }

                    const QByteArray data =
                        contextReply->readAll();

                    QJsonParseError error;

                    const QJsonDocument document =
                        QJsonDocument::fromJson(
                            data,
                            &error
                        );

                    if (error.error !=
                            QJsonParseError::NoError ||
                        !document.isObject())
                    {
                        contextReply->deleteLater();

                        return;
                    }

                    const QJsonObject object =
                        document.object();

                    const int contextSize =
                        object.value(
                            "value"
                        ).toInt();

                    if (contextSize > 0)
                    {
                        emit contextSizeChanged(
                            contextSize
                        );
                    }

                    contextReply->deleteLater();
                }
            );
        }
    );
}

void KoboldClient::generate(
    const QJsonArray &messages,
    const GenerationSettings &settings
)
{
    if (generationActive)
        return;

    streamedText.clear();
    streamBuffer.clear();

    generationActive = true;

    emit generationStarted();

    QUrl url(
        m_serverUrl +
        "/v1/chat/completions"
    );

    QNetworkRequest request(url);

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json"
    );

    request.setRawHeader(
        "Accept",
        "text/event-stream"
    );

    QJsonObject body;

    /*
     * KoboldCpp accepts the OpenAI-compatible
     * chat-completions format.
     */

    body["model"] =
        "koboldcpp";

    body["messages"] =
        messages;

    body["stream"] =
        true;

    /*
     * Apply generation settings.
     */

    const QJsonObject settingsJson =
        settings.toJson();

    for (auto it = settingsJson.begin();
         it != settingsJson.end();
         ++it)
    {
        body[it.key()] =
            it.value();
    }

    const QByteArray payload =
        QJsonDocument(body).toJson(
            QJsonDocument::Compact
        );

    generationReply =
        networkManager.post(
            request,
            payload
        );

    connect(
        generationReply,
        &QNetworkReply::readyRead,
        this,
        [this]()
        {
            if (!generationReply)
                return;

            const QByteArray data =
                generationReply->readAll();

            processStreamData(
                data
            );
        }
    );

    connect(
        generationReply,
        &QNetworkReply::finished,
        this,
        [this]()
        {
            if (!generationReply)
                return;

            /*
             * If abortGeneration() already handled
             * this request, generationReply will have
             * been cleared.
             *
             * This guard prevents the aborted request
             * from producing a second finished signal.
             */

            const QByteArray remaining =
                generationReply->readAll();

            if (!remaining.isEmpty())
            {
                processStreamData(
                    remaining
                );
            }

            if (generationReply->error() !=
                QNetworkReply::NoError)
            {
                const QString error =
                    generationReply->errorString();

                generationReply->deleteLater();

                generationReply = nullptr;

                if (!generationActive)
                    return;

                generationActive = false;

                emit generationError(
                    error
                );

                return;
            }

            generationReply->deleteLater();

            generationReply = nullptr;

            generationActive = false;

            emit generationFinished(
                streamedText
            );
        }
    );
}

void KoboldClient::processStreamData(
    const QByteArray &data
)
{
    streamBuffer.append(data);

    while (true)
    {
        const int newlineIndex =
            streamBuffer.indexOf('\n');

        if (newlineIndex < 0)
            break;

        QByteArray line =
            streamBuffer.left(
                newlineIndex
            );

        streamBuffer.remove(
            0,
            newlineIndex + 1
        );

        if (line.endsWith('\r'))
        {
            line.chop(1);
        }

        processStreamLine(
            line
        );
    }
}

void KoboldClient::processStreamLine(
    const QByteArray &line
)
{
    if (line.isEmpty())
        return;

    if (line.startsWith(':'))
        return;

    if (!line.startsWith("data:"))
        return;

    QByteArray jsonData =
        line.mid(5).trimmed();

    if (jsonData.isEmpty())
        return;

    if (jsonData == "[DONE]")
        return;

    QJsonParseError error;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            jsonData,
            &error
        );

    if (error.error !=
            QJsonParseError::NoError ||
        !document.isObject())
    {
        return;
    }

    const QJsonObject root =
        document.object();

    const QJsonArray choices =
        root.value(
            "choices"
        ).toArray();

    if (choices.isEmpty())
        return;

    const QJsonObject choice =
        choices.first().toObject();

    const QJsonObject delta =
        choice.value(
            "delta"
        ).toObject();

    const QString text =
        delta.value(
            "content"
        ).toString();

    if (text.isEmpty())
        return;

    streamedText += text;

    emit generationToken(
        text
    );
}

void KoboldClient::abortGeneration()
{
    if (!generationActive)
        return;

    /*
     * Stop receiving data immediately.
     */

    QNetworkReply *reply =
        generationReply;

    generationReply = nullptr;

    if (reply)
    {
        reply->abort();

        reply->deleteLater();
    }

    generationActive = false;

    streamBuffer.clear();

    /*
     * Keep only text ending at the last complete
     * sentence.
     */

    QString completedText =
        streamedText.trimmed();

    int lastSentenceEnd = -1;

    for (int i = 0;
         i < completedText.length();
         ++i)
    {
        const QChar character =
            completedText.at(i);

        if (character == '.' ||
            character == '!' ||
            character == '?')
        {
            lastSentenceEnd =
                i + 1;
        }
    }

    if (lastSentenceEnd > 0)
    {
        completedText =
            completedText.left(
                lastSentenceEnd
            ).trimmed();
    }
    else
    {
        /*
         * No complete sentence was generated.
         */

        completedText.clear();
    }

    streamedText =
        completedText;

    emit generationFinished(
        completedText
    );
}

