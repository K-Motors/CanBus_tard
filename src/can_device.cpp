#include "can_device.h"

#include <QCanDbcFileParser>
#include <QCanUniqueIdDescription>
#include <QCanBus>
#include <QFileInfo>
#include <QMessageBox>
#include <QCanMessageDescription>
#include "dbc_parser.h"

CanDevice::CanDevice() : QThread{nullptr}, canDevice{nullptr}, frameProcessor{}
{

    moveToThread(this);
    frameProcessor.setUniqueIdDescription(QCanDbcFileParser::uniqueIdDescription());
    start();
}

CanDevice::~CanDevice()
{
    terminate();
    wait();
    disconnect();
}

bool CanDevice::connect(QString& plugin, QString& name, unsigned baudrate, QString* error)
{
    error->clear();

    canDevice = QCanBus::instance()->createDevice(plugin, name, error);

    if (canDevice == nullptr || error->length() != 0)
    {
        return false;
    }

    // canDevice->setConfigurationParameter(QCanBusDevice::BitRateKey, baudrate);

    // TODO : Handle can bus signals (error, states, ...)
    // QObject::connect(canDevice, &QCanBusDevice::framesReceived, this, &CanDevice::onFrameReceived);
    return canDevice->connectDevice();
}

void CanDevice::disconnect()
{
    if (canDevice == nullptr)
    {
        return;
    }

    canDevice->disconnectDevice();
}

QList<QCanBusDeviceInfo> CanDevice::listAvailableDevices(QString* errorString)
{
    return QCanBus::instance()->availableDevices(errorString);
}

void CanDevice::loadDbcFiles(QStringList& dbcFiles)
{
    QList<QString>                failed_file;
    DbcParser                     parser;
    QList<QCanMessageDescription> messages_descr;

    frameProcessor.clearMessageDescriptions();
    mapMsgDescription.clear();

    qInfo() << "Reload DBC files";
    for (QString& path : dbcFiles)
    {
        QFileInfo file(path);

        if (file.isFile() == false)
        {
            qWarning() << "\tUnable to use the file:" << path << ". Path is not file or not exists.";
            failed_file.push_back(path);
            continue;
        }
        else if (file.completeSuffix().toUpper() != "DBC")
        {
            qWarning() << "\tUnable to use the file:" << path << ". Wrong file extension.";
            failed_file.push_back(path);
            continue;
        }

        if (!parser.parse(path))
        {
            qWarning() << "\tLoad DBC " << path.toStdString() << ": ERROR";
            qWarning() << "\t\tError description:" << parser.errorString();
            failed_file.push_back(path);
        }
        else
        {   
            qInfo() << "\t" << path.toStdString() << "DBC loaded succesfuly";

            for (auto& warn : parser.warnings())
            {
                qInfo() << "\t\tWarning: " << warn;
            }

            for (const auto& newMsg : parser.messageDescriptions())
            {
                auto it =
                    std::find_if(messages_descr.begin(), messages_descr.end(), [&newMsg](const auto& msg) { return msg.uniqueId() == newMsg.uniqueId(); });

                if (it != messages_descr.end())
                {
                    qInfo().nospace() << "\t\t" << newMsg.name() << " overwrites existing message (" << it->name() << ") due to same ID (" << newMsg.uniqueId()
                                      << ")";
                    size_t pos = std::distance(messages_descr.begin(), it);
                    messages_descr.remove(pos);
                }

                messages_descr.push_back(newMsg);
                mapMsgDescription.insert((unsigned)newMsg.uniqueId(), newMsg);
            }
        }
    }

    frameProcessor.setMessageDescriptions(messages_descr);

    if (failed_file.size() > 0)
    {
        QMessageBox(QMessageBox::Warning,
                    "DBC loading",
                    "Some files can't be loaded (invalid or file not found). See the console for more information.\n\n" + failed_file.join("\n"))
            .exec();
    }
}

QCanMessageDescription CanDevice::getMessageDescription(unsigned id)
{
    if (mapMsgDescription.contains(id))
        return mapMsgDescription[id];
    else
        return QCanMessageDescription();
}

bool CanDevice::sendFrame(quint32 id, const QVariantMap& signalsValues, QString* error)
{
    error->clear();
    if (canDevice == nullptr)
    {
        *error = "No device connected";
        return false;
    }

    QCanBusFrame frame = frameProcessor.prepareFrame((QtCanBus::UniqueId)id, signalsValues);

    if (frame.error() != QCanBusFrame::NoError)
    {
        *error = QString("Frame error. Reason: %1 -- %2").arg((unsigned)frame.error()).arg(frameProcessor.errorString());
        return false;
    }

    if (id & 0x80000000) frame.setExtendedFrameFormat(true);

    if (canDevice->writeFrame(frame) == false)
    {
        *error = QString("Failed to send CAN frame. Reason: %1 -- %2").arg((unsigned)canDevice->error()).arg(canDevice->errorString());
        return false;
    }

    return true;
}

void CanDevice::run()
{
    forever
    {
        while (canDevice != nullptr && canDevice->framesAvailable() > 0)
        {
            QCanBusFrame                    frame  = canDevice->readFrame();
            QCanFrameProcessor::ParseResult result = frameProcessor.parseFrame(frame);

            if (result.signalValues.empty())
                emit canUnknownFrameReceived(frame);
            else
                emit canFrameReceived(frame, result.signalValues);
        }

        QThread::msleep(1);
    }
}
