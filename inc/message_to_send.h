#pragma once

#include <QString>
#include <QMap>
#include <QCanMessageDescription>
#include <QCanSignalDescription>

struct MessageToSendValue
{
    QString  unit;
    QVariant value;
};

class MessageToSend
{
  public:
    MessageToSend(QCanMessageDescription& msgDescr) : messageName{""}, messageId{0}, values{}, repeatEvery{0}, key('\0')
    {
        messageName = msgDescr.name();
        messageId   = (unsigned)msgDescr.uniqueId();
        for (auto& sig : msgDescr.signalDescriptions())
        {
            values.insert(sig.name(), {.unit = sig.physicalUnit(), .value = sig.minimum()});
        }
    }

    QString print()
    {
        return QString("%1\n0x%2 - %3 (%4)")
            .arg(messageName)
            .arg(QString::number(messageId, 16).toUpper())
            .arg(repeatEvery > 0 ? QString::number(repeatEvery) + " ms" : "No repeat")
            .arg(key == '\0' ? "No bind" : "'" + QString(key) + "'");
    }

    QString                           messageName;
    unsigned                          messageId;
    QString                           unit;
    QMap<QString, MessageToSendValue> values;
    unsigned                          repeatEvery;
    QChar                             key;
};
