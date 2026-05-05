#include "can_device.h"

#include <QApplication>
#include <QCanDbcFileParser>
#include <QCanUniqueIdDescription>
#include <QCanBus>
#include <QFileInfo>
#include <QMessageBox>
#include <QCanMessageDescription>
#include <QElapsedTimer>

#include "dbc_parser.h"

CanDevice::CanDevice() : QThread{nullptr}, canDevice{nullptr}, frameProcessor{}, nextUUID{1}
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
    bool result = false;

    QMetaObject::invokeMethod(
        this,
        [this, &plugin, &name, error, &result]()
        {
            canDevice = QCanBus::instance()->createDevice(plugin, name, error);
            if (canDevice == nullptr || !error->isEmpty())
            {
                result = false;
                return;
            }
            result = canDevice->connectDevice();
        },
        Qt::BlockingQueuedConnection);

    return result;
}

void CanDevice::disconnect()
{
    if (canDevice == nullptr)
    {
        return;
    }

    canDevice->disconnectDevice();
    canDevice = nullptr;
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

    QMutexLocker locker(&oneShotFrameMutex);
    oneShotFrameToSend.enqueue(FrameData{.id = id, .values = signalsValues});
    return true;
}

quint32 CanDevice::addPeriodicFrame(quint32 id, const QVariantMap& signalsValues, quint32 everyMs)
{
    if (nextUUID == 0)
    {
        qWarning() << "UUID for periodic frame had overflow !";
        return 0;
    }

    QMutexLocker locker(&periodicFrameMutex);
    periodicFrameMap.insert(nextUUID, PeriodicFrame{.frameData = FrameData{.id = id, .values = signalsValues}, .everyMs = everyMs, .lastMs = 0});

    return nextUUID++;
}

void CanDevice::removePeriodicFrame(quint32 uuid)
{
    QMutexLocker locker(&periodicFrameMutex);
    periodicFrameMap.remove(uuid);
}

QCanBusFrame CanDevice::createFrame(quint32 id, const QVariantMap& signalsValues, QString* error)
{
    QCanBusFrame frame = frameProcessor.prepareFrame((QtCanBus::UniqueId)id, signalsValues);

    error->clear();

    if (frame.error() != QCanBusFrame::NoError)
    {
        *error = QString("Frame error. Reason: %1 -- %2").arg((unsigned)frame.error()).arg(frameProcessor.errorString());
    }

    if (id & 0x80000000) frame.setExtendedFrameFormat(true);

    return frame;
}

void CanDevice::run()
{
    QString       errorCreateFrame;
    QElapsedTimer elapse;
    elapse.start();

    forever
    {
        QCoreApplication::processEvents();

        if (canDevice != nullptr && canDevice->state() == QCanBusDevice::ConnectedState)
        {
            oneShotFrameMutex.lock();
            while (false == oneShotFrameToSend.isEmpty())
            {
                qInfo() << "OneShotFrameToSend size:" << oneShotFrameToSend.size();
                FrameData    frameData = oneShotFrameToSend.dequeue();
                QCanBusFrame frame     = createFrame(frameData.id, frameData.values, &errorCreateFrame);

                qDebug() << "Creating frame for ID:" << Qt::hex << frameData.id << "values:" << frameData.values;
                qDebug() << "Frame created, error:" << errorCreateFrame << "frame valid:" << frame.isValid() << "frame error:" << frame.error();
                if (errorCreateFrame.isEmpty())
                {
                    if (false == canDevice->writeFrame(frame))
                    {
                        qWarning() << "Failed to send CAN frame. Reason:" << (unsigned)canDevice->error() << "--" << canDevice->errorString();
                    }
                    qDebug() << "writeFrame -- " << "device error:" << canDevice->errorString();
                }
                else
                {
                    qWarning() << "Failed to create CAN frame:" << errorCreateFrame;
                }
            }
            oneShotFrameMutex.unlock();

            periodicFrameMutex.lock();
            for (auto& pFrame : periodicFrameMap)
            {
                if (elapse.elapsed() - pFrame.lastMs >= pFrame.everyMs)
                {
                    QCanBusFrame frame = createFrame(pFrame.frameData.id, pFrame.frameData.values, &errorCreateFrame);
                    if (errorCreateFrame.isEmpty())
                    {
                        if (false == canDevice->writeFrame(frame))
                        {
                            qWarning() << "Failed to send (periodic) CAN frame. Reason:" << (unsigned)canDevice->error() << "--" << canDevice->errorString();
                        }
                    }
                    else
                    {
                        qWarning() << "Failed to create CAN frame:" << errorCreateFrame;
                    }
                    pFrame.lastMs = elapse.elapsed();
                }
            }
            periodicFrameMutex.unlock();

            while (canDevice->framesAvailable() > 0)
            {
                QCanBusFrame                    frame  = canDevice->readFrame();
                QCanFrameProcessor::ParseResult result = frameProcessor.parseFrame(frame);

                if (result.signalValues.empty())
                    emit canUnknownFrameReceived(frame);
                else
                    emit canFrameReceived(frame, result.signalValues);
            }
        }

        QThread::usleep(250);
    }
}
