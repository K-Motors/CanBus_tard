#pragma once

#include <QString>
#include <QMap>
#include <QCanMessageDescription>
#include <QCanSignalDescription>
#include <QJsonObject>

struct MessageToSendValue
{
    QString  unit;
    QVariant value;
};

class MessageToSend
{
  public:
    MessageToSend() : messageName{""}, messageId{0}, values{}, repeatEvery{0}, key('\0'), uuidCanDevice{0} {}
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

    QVariantMap valuestoVariantMap()
    {
        QVariantMap map;

        for (QString& key : values.keys())
        {
            map.insert(key, values[key].value);
        }

        return map;
    }

    QJsonObject serialize()
    {
        QJsonObject obj;
        obj["messageName"] = messageName;
        obj["messageId"]   = (int)messageId;
        obj["repeatEvery"] = (int)repeatEvery;
        obj["key"]         = QString(key);

        QJsonObject serValues;

        for (QString& key : values.keys())
        {
            QJsonObject data;
            data["unit"]   = values[key].unit;
            data["value"]  = QJsonValue::fromVariant(values[key].value);
            serValues[key] = data;
        }

        obj["values"] = serValues;

        return obj;
    }

    void loadFromJson(const QJsonObject& json)
    {
        messageName    = json.value("messageName").toString();
        messageId      = (unsigned)json.value("messageId").toInt();
        repeatEvery    = (unsigned)json.value("repeatEvery").toInt();
        QString rawKey = json.value("key").toString();
        key            = rawKey.isEmpty() ? '\0' : rawKey[0];

        QJsonObject serValues = json.value("values").toObject();
        for (auto it = serValues.constBegin(); it != serValues.constEnd(); ++it)
        {
            QJsonObject        data = it.value().toObject();
            MessageToSendValue value;
            value.value = data.value("value").toVariant();
            value.unit  = data.value("unit").toString();
            values.insert(it.key(), value);
        }
    }

    QString                           messageName;
    unsigned                          messageId;
    QMap<QString, MessageToSendValue> values;
    unsigned                          repeatEvery;
    QChar                             key;
    quint32                           uuidCanDevice;
};
