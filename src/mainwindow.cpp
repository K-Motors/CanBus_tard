#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QLabel>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QFontDatabase>

#include <QMap>
#include <QQueue>
#include <QTimer>

#include "const.h"

constexpr unsigned NUMBER_TABLE_ROW = 6;
constexpr unsigned USERDATA_ROW     = 0;
constexpr unsigned ID_ROW           = 2;

constexpr unsigned ID_TIME_COL  = 0;
constexpr unsigned ID_ASCII_COL = 4;
constexpr unsigned ID_RAW_COL   = 5;

static QFont monoFont;
static QFont monoFontItalic;

struct PendingFrame
{
    QCanBusFrame frame;
    QVariantMap  signalsValues;
    bool         isUnknown;
    QTime        time;
};

static QMap<quint32, PendingFrame> pendingFramesMap;
static QQueue<PendingFrame>        pendingFramesQueue;

QString rawToAscii(QByteArray data)
{
    QString result;

    for (int i = 0; i < 8; ++i)
    {
        QChar c((quint8)data[i]);

        if (c.isPrint())
            result += c;
        else
            result += '.';
    }

    return result;
}

QString rawToString(QByteArray data)
{
    QString result;

    for (int i = 0; i < 8; ++i)
    {
        if ((quint8)data[i] < 16) result += "0";

        result += QString::number((quint8)data[i], 16).toUpper() + " ";
    }

    return result.removeLast();
}

MainWindow::MainWindow(QSettings* settings, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), settings{settings}, processStat{1}, canDevice{}, dbcManager{settings, this}, filterManager(settings, this),
      dockSignalWatcher{}, dockSendMessage{canDevice, settings, this}, isConnected{false}, selectedId{-1}
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/img/logo.png"));
    restoreGeometry(settings->value(SETTINGS_KEY_WIN_GEOMETRY).toByteArray());
    restoreState(settings->value(SETTINGS_KEY_WIN_STATE).toByteArray());

    monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);

    monoFontItalic = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFontItalic.setPointSize(10);
    monoFontItalic.setItalic(true);

    dockSignalWatcher.init(ui);
    dockSendMessage.init(ui);

    // === SETUP ACTIONS ===
    connect(ui->actionManage_DBC_files, &QAction::triggered, this, [this]() { dbcManager.exec(); });
    connect(ui->actionForce_reload_DBC, &QAction::triggered, this, [this]() { loadDbcFiles(); });
    connect(&dbcManager, &QDialog::accepted, this, &MainWindow::loadDbcFiles);
    connect(&dbcManager, &QDialog::rejected, this, &MainWindow::loadDbcFiles);

    // === FILTER ACTIONS ===
    connect(ui->action_filter, &QAction::triggered, this, [this]() { filterManager.exec(); });
    connect(ui->action_save_filters_config, &QAction::triggered, this, [this]() { saveFilters(); });
    connect(ui->action_open_filters_config, &QAction::triggered, this, [this]() { openFilters(); });

    // === VIEW ACTIONS ===
    connect(ui->action_overwrite_mode,
            &QAction::triggered,
            this,
            [this](bool checked)
            {
                clearMessageTable();
                if (checked)
                    pendingFramesQueue.clear();
                else
                    pendingFramesMap.clear();
            });
    connect(ui->action_view_clear, &QAction::triggered, this, [this]() { clearMessageTable(); });

    // === DEVICE CONNECTION ===
    connect(ui->button_refresh_device, &QPushButton::clicked, this, &MainWindow::refreshDeviceList);
    connect(ui->button_connect_device, &QPushButton::clicked, this, &MainWindow::connectDisconnectDevice);

    refreshDeviceList();
    initMessageTable();

    processStat.start();

    connect(&canDevice, &CanDevice::canFrameReceived, this, &MainWindow::canFrameReceived);
    connect(&canDevice, &CanDevice::canUnknownFrameReceived, this, &MainWindow::canUnknownFrameReceived);

    connect(ui->table_can_messages, &QTableWidget::cellClicked, this, &MainWindow::onCellClicked);

    loadDbcFiles();

    //      ===== Status Bar section =====
    QLabel* statusLabel = new QLabel("Loading status...");
    QLabel* loveLabel   = new QLabel(QString::fromUtf8("Made with \xe2\x99\xa5 by K-Motors"));
    ui->statusBar->addPermanentWidget(loveLabel);
    ui->statusBar->addWidget(statusLabel);
    connect(&processStat,
            &ProcessStat::updateStat,
            this,
            [statusLabel](unsigned long mem, float cpu) {
                statusLabel->setText(
                    QString("PID: %1    CPU: %2 %    MEM: %3 MB").arg(QCoreApplication::applicationPid()).arg(cpu, 0, 'f', 1).arg(mem / 1024 / 1024));
            });

    QTimer* uiRefreshTimer = new QTimer(this);
    connect(uiRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshTable);
    uiRefreshTimer->start(33); // ~30fps

    qApp->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::canUnknownFrameReceived(QCanBusFrame frame)
{

    if (false == ui->action_enable_filters->isChecked() || (ui->action_enable_filters->isChecked() && filterManager.isMessageAccept(frame.frameId())))
    {
        PendingFrame pending;

        pending.isUnknown = true;
        pending.frame     = frame;
        pending.time      = QTime::currentTime();

        if (ui->action_overwrite_mode->isChecked())
        {
            pendingFramesMap.insert((quint32)frame.frameId(), pending);
        }
        else
        {
            pendingFramesQueue.enqueue(pending);
        }
    }
}

void MainWindow::canFrameReceived(QCanBusFrame frame, QVariantMap signalsValues)
{
    if (false == ui->action_enable_filters->isChecked() || (ui->action_enable_filters->isChecked() && filterManager.isMessageAccept(frame.frameId())))
    {
        PendingFrame pending;

        pending.isUnknown     = false;
        pending.frame         = frame;
        pending.signalsValues = signalsValues;
        pending.time          = QTime::currentTime();

        if (ui->action_overwrite_mode->isChecked())
        {
            pendingFramesMap.insert((quint32)frame.frameId(), pending);
        }
        else
        {
            pendingFramesQueue.enqueue(pending);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    settings->setValue(SETTINGS_KEY_WIN_STATE, saveState());
    settings->setValue(SETTINGS_KEY_WIN_GEOMETRY, saveGeometry());
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() >= Qt::Key_A && keyEvent->key() <= Qt::Key_Z)
        {
            QWidget* focused = qApp->focusWidget();
            if (nullptr == qobject_cast<QLineEdit*>(focused) && nullptr == qobject_cast<QTextEdit*>(focused) &&
                nullptr == qobject_cast<QPlainTextEdit*>(focused))
            {
                QChar key = QChar(keyEvent->key()).toLower();
                qDebug().nospace() << "'" << key << "' pressed";
                dockSendMessage.sendMessageWithKey(key);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::initMessageTable()
{
    mapIdLine.clear();
    ui->table_can_messages->clear();
    ui->table_can_messages->clearContents();
    ui->table_can_messages->setRowCount(0);

    ui->table_can_messages->setColumnCount(NUMBER_TABLE_ROW);
    ui->table_can_messages->setHorizontalHeaderLabels({"Time", "Name", "ID", "Std/Ext", "ASCII", "Raw Data"});

    QHeaderView* hh = ui->table_can_messages->horizontalHeader();

    for (unsigned col = 1; col < NUMBER_TABLE_ROW; ++col) hh->setSectionResizeMode(col, QHeaderView::ResizeToContents);

    hh->setStretchLastSection(true);
    ui->table_can_messages->setColumnWidth(0, computeTextSize("99:99:99.999"));
}

void MainWindow::clearMessageTable()
{
    ui->table_can_messages->clear();
    ui->table_can_messages->clearContents();
    initMessageTable();
}

void MainWindow::addMessageLine(quint32 id, QTableWidgetItem* items[], bool isItalic)
{
    int row = ui->table_can_messages->rowCount();

    if (ui->action_overwrite_mode->isChecked())
    {
        if (mapIdLine.contains(id))
        {
            row = mapIdLine[id];
        }
        else
        {
            mapIdLine.insert(id, row);
            ui->table_can_messages->insertRow(row);
        }
    }
    else
    {
        ui->table_can_messages->insertRow(row);

        if (ui->action_autoscroll->isChecked()) ui->table_can_messages->scrollToBottom();
    }

    for (unsigned i = 0; i < NUMBER_TABLE_ROW; i++)
    {
        ui->table_can_messages->setItem(row, i, items[i]);
        ui->table_can_messages->item(row, i)->setFont(isItalic ? monoFontItalic : monoFont);
    }
}

void MainWindow::onCellClicked(int row, int)
{
    QVariant data = ui->table_can_messages->item(row, USERDATA_ROW)->data(Qt::UserRole);
    unsigned id   = ui->table_can_messages->item(row, ID_ROW)->data(Qt::UserRole).toUInt();

    selectedId = -1;

    if (data.isNull()) return;

    QVariantMap signalsData = data.toMap();

    selectedId = id;
    dockSignalWatcher.setMessageSignals(canDevice.getMessageDescription(id), signalsData);
}

void MainWindow::refreshDeviceList()
{
    QString                  error;
    QList<QCanBusDeviceInfo> availableDevices = canDevice.listAvailableDevices(&error);

    if (error.length() > 0)
    {
        qCritical() << "Failed to retrieve available devices. Reason:" << error;
    }
    else
    {
        ui->combo_can_device->clear();
        for (QCanBusDeviceInfo& dev : availableDevices)
        {
            QString     name     = QString("%1 (%2: %3) - Channel %5").arg(dev.name(), dev.plugin(), dev.description()).arg(dev.channel());
            QStringList userData = QStringList({dev.name(), dev.plugin()});
            ui->combo_can_device->addItem(name, userData);
        }
    }
}

void MainWindow::connectDisconnectDevice()
{
    if (isConnected)
    {
        canDevice.disconnect();
        ui->button_connect_device->setText("Connect");
        isConnected = false;

        ui->combo_can_device->setEnabled(true);
        ui->button_refresh_device->setEnabled(true);
        ui->spin_baudrate_device->setEnabled(true);
    }
    else
    {
        initMessageTable();
        int idx = ui->combo_can_device->currentIndex();

        if (idx == -1 || idx >= ui->combo_can_device->count()) return;

        QString     error;
        QStringList devData = ui->combo_can_device->itemData(idx).toStringList();

        if (canDevice.connect(devData[1], devData[0], ui->spin_baudrate_device->value(), &error))
        {
            ui->button_connect_device->setText("Disconnect");
            isConnected = true;

            ui->combo_can_device->setDisabled(true);
            ui->button_refresh_device->setDisabled(true);
            ui->spin_baudrate_device->setDisabled(true);
        }
        else
        {
            QMessageBox::critical(this, "Connection", QString("Failed to connect to CAN %1.\nReason: %2").arg(devData[0], error));
        }
    }
}

void MainWindow::loadDbcFiles()
{
    QStringList files = dbcManager.getDbcFiles();
    canDevice.loadDbcFiles(files);
}

void MainWindow::saveFilters()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save filters", "filters.kflt", "Filters file (*.kflt *.KFLT);;All files (*.*)");

    if (filePath.isEmpty()) return;

    QFile saveFile(filePath);

    if (!saveFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text))
    {
        QMessageBox::critical(this, "Failed open file", "Unable to open the file (" + filePath + ")");
        return;
    }

    QTextStream out(&saveFile);
    for (auto& line : filterManager.toString()) out << line << "\n";
}

void MainWindow::openFilters()
{

    QString filePath = QFileDialog::getOpenFileName(this, "Open filters", "", "Filters file (*.kflt *.KFLT);;All files (*.*)");

    if (filePath.isEmpty()) return;

    QFile openFile(filePath);

    if (!openFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
    {
        QMessageBox::critical(this, "Failed open file", "Unable to open the file (" + filePath + ")");
        return;
    }

    QTextStream in(&openFile);
    QStringList lines;

    while (!in.atEnd()) lines << in.readLine();
    filterManager.loadFromStrings(lines);
}

void MainWindow::refreshTable()
{
    while (true)
    {
        PendingFrame pending;

        if (ui->action_overwrite_mode->isChecked())
        {
            if (pendingFramesMap.isEmpty()) return;

            pending = pendingFramesMap.take(pendingFramesMap.firstKey());
        }
        else
        {
            if (pendingFramesQueue.empty()) return;
            pending = pendingFramesQueue.dequeue();
        }

        quint32    id      = (quint32)pending.frame.frameId();
        QByteArray rawData = pending.frame.payload();

        if (ui->action_overwrite_mode->isChecked() && mapIdLine.contains(id))
        {
            int row = mapIdLine[id];

            ui->table_can_messages->item(row, ID_TIME_COL)->setText(pending.time.toString("HH:mm:ss.zzz"));
            ui->table_can_messages->item(row, ID_ASCII_COL)->setText(rawToAscii(rawData));
            ui->table_can_messages->item(row, ID_RAW_COL)->setText(rawToString(rawData));

            if (!pending.isUnknown)
            {
                ui->table_can_messages->item(row, USERDATA_ROW)->setData(Qt::UserRole, pending.signalsValues);
                if ((int)pending.frame.frameId() == selectedId)
                {
                    dockSignalWatcher.setMessageSignals(canDevice.getMessageDescription(id), pending.signalsValues);
                }
            }
        }
        else
        {
            QTableWidgetItem* items[NUMBER_TABLE_ROW] = {new QTableWidgetItem(pending.time.toString("HH:mm:ss.zzz")),
                                                         new QTableWidgetItem(pending.isUnknown ? "---" : canDevice.getMessageDescription(id).name()),
                                                         new QTableWidgetItem(QString::number(id, 16)),
                                                         new QTableWidgetItem(pending.frame.hasExtendedFrameFormat() ? "EXT" : "STD"),
                                                         new QTableWidgetItem(rawToAscii(rawData)),
                                                         new QTableWidgetItem(rawToString(rawData))};
            items[ID_ROW]->setData(Qt::UserRole, id);
            if (!pending.isUnknown) items[USERDATA_ROW]->setData(Qt::UserRole, pending.signalsValues);
            addMessageLine(id, items, pending.isUnknown);
        }
    }
}

int MainWindow::computeTextSize(const QString& text)
{
    QFontMetrics fm(ui->table_can_messages->font());
    return fm.horizontalAdvance(text) + 10; // +10 for padding
}
