#include "KoboldClient.h"
#include "GenerationSettings.h"

#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QUrl>

namespace
{
    bool isClosingCharacter(
        const QChar character
    )
    {
        return character == '"' ||
               character == '\'' ||
               character == ')' ||
               character == ']' ||
               character == '}' ||
               character == '*';
    }

    bool isCommonAbbreviation(
        const QString &word
    )
    {
        static const QStringList abbreviations =
        {
            "Mr.",
            "Mrs.",
            "Ms.",
            "Dr.",
            "Prof.",
            "Sr.",
            "Jr.",
            "St.",
            "vs.",
            "etc.",
            "e.g.",
            "i.e.",
            "a.m.",
            "p.m."
        };

        return abbreviations.contains(
            word,
            Qt::CaseInsensitive
        );
    }
}

int KoboldClient::findLastSentenceEnd(
    const QString &text
)
{
    int lastSentenceEnd = -1;

    for (int i = 0;
         i < text.length();
         ++i)
    {
        const QChar character =
            text.at(i);

        if (character != '.' &&
            character != '!' &&
            character != '?')
        {
            continue;
        }

        /*
         * A period between two digits is most likely
         * a decimal number, e.g. 3.14.
         */
        if (character == '.' &&
            i > 0 &&
            i + 1 < text.length() &&
            text.at(i - 1).isDigit() &&
            text.at(i + 1).isDigit())
        {
            continue;
        }

        /*
         * Check the word immediately preceding the period
         * for common abbreviations such as "Mr." or "Dr.".
         */
        if (character == '.')
        {
            int wordStart = i - 1;

            while (wordStart >= 0 &&
                   !text.at(wordStart).isSpace())
            {
                --wordStart;
            }

            const QString word =
                text.mid(
                    wordStart + 1,
                    i - wordStart
                );

            if (isCommonAbbreviation(word))
            {
                continue;
            }
        }

        /*
         * This is a sentence-ending punctuation mark.
         *
         * Consume whitespace and closing/formatting
         * characters which belong to the sentence.
         */
        int end = i + 1;

        while (end < text.length() &&
               isClosingCharacter(text.at(end)))
        {
            ++end;
        }

        lastSentenceEnd = end;
    }

    return lastSentenceEnd;
}

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

    QJsonObject modifiedSettings = settingsJson;

    for (auto it = modifiedSettings.begin();
         it != modifiedSettings.end();
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

            QString completedText =
                streamedText.trimmed();

            const int lastSentenceEnd =
                findLastSentenceEnd(
                    completedText
                );

            if (lastSentenceEnd > 0 &&
                lastSentenceEnd < completedText.length())
            {
                completedText =
                    completedText.left(
                        lastSentenceEnd
                    ).trimmed();
            }

            streamedText =
                completedText;

            emit generationFinished(
                completedText
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
    
    /*
     * Tell KoboldCpp to abort the generation on
     * the backend before closing our stream.
     */
    QUrl abortUrl(
        m_serverUrl +
        "/api/extra/abort"
    );

    QNetworkRequest abortRequest(
        abortUrl
    );

    abortRequest.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json"
    );

    networkManager.post(
        abortRequest,
        QByteArray()
    );

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

void KoboldClient::clearCache()
{
    QUrl clearCacheUrl(
        m_serverUrl +
        "/api/admin/clear_state"
    );

    QNetworkRequest clearCacheRequest(
        clearCacheUrl
    );

    clearCacheRequest.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json"
    );

    QNetworkReply *reply = networkManager.post(
        clearCacheRequest,
        QByteArray()
    );

    connect(
        reply,
        &QNetworkReply::finished,
        this,
        [this, reply]()
        {
            int statusCode = reply->attribute(
                QNetworkRequest::HttpStatusCodeAttribute
            ).toInt();

            QString response = QString::fromUtf8(
                reply->readAll()
            );

            bool success =
                reply->error() == QNetworkReply::NoError &&
                statusCode >= 200 &&
                statusCode < 300;

            QString message = QString(
                "HTTP %1: %2"
            ).arg(
                statusCode
            ).arg(
                response.isEmpty()
                    ? reply->errorString()
                    : response
            );

            emit cacheClearResult(
                success,
                message
            );

            reply->deleteLater();
        }
    );
}

