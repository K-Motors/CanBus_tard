#pragma once

#include <QMainWindow>
#include <QCloseEvent>

#include "can_device.h"
#include "dbc_manager.h"
#include "dock_send_message.h"
#include "dock_signal_watcher.h"
#include "filter_manager.h"
#include "process_stat.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow(QSettings* settings, QWidget* parent = nullptr);
    ~MainWindow();

  public slots:
    void canUnknownFrameReceived(QCanBusFrame frame);
    void canFrameReceived(QCanBusFrame frame, QVariantMap signalsValues);

  protected:
    void closeEvent(QCloseEvent* event) override;

  private:
    Ui::MainWindow*    ui;
    QSettings*         settings;
    ProcessStat        processStat;
    CanDevice          canDevice;
    QMap<quint32, int> mapIdLine;

    DbcManager    dbcManager;
    FilterManager     filterManager;
    DockSignalWatcher dockSignalWatcher;
    DockSendMessage   dockSendMessage;

    bool isConnected;
    int  selectedId;

    void initMessageTable();
    void clearMessageTable();
    void addMessageLine(quint32 id, QTableWidgetItem* items[], bool isItalic);
    void onCellClicked(int row, int);

    void refreshDeviceList();
    void connectDisconnectDevice();

    void loadDbcFiles();

    void saveFilters();
    void openFilters();

    void refreshTable();

    int computeTextSize(const QString& text);
};
