#pragma once

#include <QCanDbcFileParser>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QList>
#include <QCanMessageDescription>

class DbcParser
{
  public:
    DbcParser() {}

    bool parse(const QString& file)
    {
        QCanDbcFileParser parser;

        internalErrorString.clear();
        messagesDescription.clear();

        QMap<QString, quint32> mapId = parseMessageNameAndId(file);

        if (isError) return false;

        if (!parser.parse(file))
        {
            internalErrorString = QString("Failed to load DBC (%1). %2").arg(file, parser.errorString());
            return false;
        }

        for (auto& message : parser.messageDescriptions())
        {
            QCanMessageDescription newMsg;

            newMsg.setName(message.name());
            newMsg.setComment(message.comment());
            newMsg.setSignalDescriptions(message.signalDescriptions());
            newMsg.setSize(message.size());
            newMsg.setTransmitter(message.transmitter());

            if (mapId.contains(message.name()))
                newMsg.setUniqueId((QtCanBus::UniqueId)mapId[message.name()]);
            else
                newMsg.setUniqueId(message.uniqueId());

            messagesDescription.append(newMsg);
        }

        parserWarnings = parser.warnings();

        return true;
    }

    QString                       errorString() const { return internalErrorString; }
    QList<QCanMessageDescription> messageDescriptions() const { return messagesDescription; }
    QStringList                   warnings() const { return parserWarnings; }

  private:
    QList<QCanMessageDescription> messagesDescription;
    bool                          isError;
    QString                       internalErrorString;
    QStringList                   parserWarnings;

    QMap<QString, quint32> parseMessageNameAndId(const QString& filePath)
    {
        static const QRegularExpression regexMsg(R"(^BO_\s+(\d+)\s+(\w+)\s*:)");
        QMap<QString, quint32>          result;

        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            internalErrorString = "Unable to open DBC file (" + filePath + ")";
            isError             = true;
            return result;
        }

        QTextStream in(&file);

        while (!in.atEnd())
        {
            QString                 line  = in.readLine();
            QRegularExpressionMatch match = regexMsg.match(line);

            if (match.hasMatch())
            {
                quint32 id   = match.captured(1).toULongLong() & 0x1FFFFFFF;
                QString name = match.captured(2);
                result.insert(name, id);
            }
        }

        file.close();
        isError = false;
        return result;
    }
};
