#pragma once

#include <QStringList>
#include <QThread>

#include <QCanBusDevice>
#include <QCanFrameProcessor>
#include <QCanMessageDescription>
#include <QCanSignalDescription>
#include <QMap>

class CanDevice : public QThread
{

    Q_OBJECT

  public:
    CanDevice();
    ~CanDevice();

    bool                     connect(QString& plugin, QString& name, unsigned baudrate, QString* error);
    void                     disconnect();
    QList<QCanBusDeviceInfo> listAvailableDevices(QString* errorString = nullptr);

    void loadDbcFiles(QStringList& dbcFiles);
    QCanMessageDescription getMessageDescription(unsigned id);
    QList<QCanMessageDescription> getMessageDescriptions() { return frameProcessor.messageDescriptions(); }

    bool sendFrame(quint32 id, const QVariantMap& signalsValues, QString* error);

  signals:
    void canUnknownFrameReceived(QCanBusFrame frame);
    void canFrameReceived(QCanBusFrame frame, QVariantMap signalValues);

  protected:
    void run() override;

  private:
    QCanBusDevice*     canDevice;
    QCanFrameProcessor frameProcessor;
    QMap<unsigned, QCanMessageDescription> mapMsgDescription;
};
