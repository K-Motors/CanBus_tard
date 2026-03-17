#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QLabel>

#include "const.h"

constexpr unsigned NUMBER_TABLE_ROW = 6;
constexpr unsigned USERDATA_ROW     = 0;
constexpr unsigned ID_ROW           = 2;

QString rawToAscii(quint64 raw)
{
    QString result;

    for (int i = 0; i < 8; ++i)
    {
        quint64 mask = 0xFF00000000000000 >> (i * 8);
        quint8  val  = (raw & mask) >> ((7 - i) * 8);
        QChar   c(val);

        if (c.isPrint())
            result += c;
        else
            result += '.';
    }

    return result;
}

QString rawToString(quint64 raw)
{
    QString result;

    for (int i = 0; i < 8; ++i)
    {
        quint64 mask = 0xFF00000000000000 >> (i * 8);
        quint8  val  = (raw & mask) >> ((7 - i) * 8);

        result += QString::number(val, 16).toUpper() + " ";
    }

    return result;
}

quint64 payloadTo64bits(QByteArray data)
{
    quint64 value = 0;

    for (int i = 0; i < 8; ++i)
    {
        value = data[i] << (i * 8);
    }

    return value;
}

MainWindow::MainWindow(QSettings* settings, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), settings{settings}, processStat{1}, canDevice{}, dbcManager{settings, this}, filterManager(settings, this),
      dockSignalWatcher{}, dockSendMessage{canDevice, this}, isConnected{false}, selectedId{-1}
{
    ui->setupUi(this);
    restoreGeometry(settings->value(SETTINGS_KEY_WIN_GEOMETRY).toByteArray());
    restoreState(settings->value(SETTINGS_KEY_WIN_STATE).toByteArray());

    dockSignalWatcher.init(ui);
    dockSendMessage.init(ui);

    // === FILE ACTIONS ===
    connect(ui->action_save_filters_config, &QAction::triggered, this, [this]() { saveFilters(); });
    connect(ui->action_open_filters_config, &QAction::triggered, this, [this]() { openFilters(); });

    // === DBC ACTIONS ===
    connect(ui->actionManage_DBC_files, &QAction::triggered, this, [this]() { dbcManager.exec(); });
    connect(ui->actionForce_reload_DBC, &QAction::triggered, this, [this]() { loadDbcFiles(); });
    connect(&dbcManager, &QDialog::accepted, this, &MainWindow::loadDbcFiles);
    connect(&dbcManager, &QDialog::rejected, this, &MainWindow::loadDbcFiles);

    // === MESSAGE ACTIONS ===
    connect(ui->action_filter, &QAction::triggered, this, [this]() { filterManager.exec(); });
    connect(ui->action_overwrite_mode, &QAction::triggered, this, [this]() { clearMessageTable(); });

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
    ui->statusBar->addWidget(statusLabel);
    connect(&processStat,
            &ProcessStat::updateStat,
            this,
            [statusLabel](unsigned long mem, float cpu) {
                statusLabel->setText(
                    QString("PID: %1    CPU: %2 %    MEM: %3 MB").arg(QCoreApplication::applicationPid()).arg(cpu, 0, 'f', 1).arg(mem / 1024 / 1024));
            });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::canUnknownFrameReceived(QCanBusFrame frame)
{
    if (false == ui->action_enable_filters->isChecked() || (ui->action_enable_filters->isChecked() && filterManager.isMessageAccept(frame.frameId())))
    {
        quint64           rawData                 = payloadTo64bits(frame.payload());
        quint32           id                      = (quint32)frame.frameId();
        QTableWidgetItem* items[NUMBER_TABLE_ROW] = {new QTableWidgetItem(QTime::currentTime().toString("HH:mm:ss.zzz")),
                                                     new QTableWidgetItem("---"),
                                                     new QTableWidgetItem(QString::number(id, 16)),
                                                     new QTableWidgetItem(frame.hasExtendedFrameFormat() ? "EXT" : "STD"),
                                                     new QTableWidgetItem(rawToAscii(rawData)),
                                                     new QTableWidgetItem(rawToString(rawData))};

        items[ID_ROW]->setData(Qt::UserRole, id);
        addMessageLine(id, items, true);
    }
}

void MainWindow::canFrameReceived(QCanBusFrame frame, QVariantMap signalsValues)
{
    if (false == ui->action_enable_filters->isChecked() || (ui->action_enable_filters->isChecked() && filterManager.isMessageAccept(frame.frameId())))
    {
        if ((int)frame.frameId() == selectedId)
        {
            dockSignalWatcher.setMessageSignals(canDevice.getMessageDescription((unsigned)frame.frameId()), signalsValues);
        }

        quint64           rawData                 = payloadTo64bits(frame.payload());
        quint32           id                      = (quint32)frame.frameId();
        QTableWidgetItem* items[NUMBER_TABLE_ROW] = {new QTableWidgetItem(QTime::currentTime().toString("HH:mm:ss.zzz")),
                                                     new QTableWidgetItem(canDevice.getMessageDescription((unsigned)frame.frameId()).name()),
                                                     new QTableWidgetItem(QString::number(id, 16)),
                                                     new QTableWidgetItem(frame.hasExtendedFrameFormat() ? "EXT" : "STD"),
                                                     new QTableWidgetItem(rawToAscii(rawData)),
                                                     new QTableWidgetItem(rawToString(rawData))};

        items[USERDATA_ROW]->setData(Qt::UserRole, signalsValues);
        items[ID_ROW]->setData(Qt::UserRole, id);
        addMessageLine(id, items, false);
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    settings->setValue(SETTINGS_KEY_WIN_STATE, saveState());
    settings->setValue(SETTINGS_KEY_WIN_GEOMETRY, saveGeometry());
    QMainWindow::closeEvent(event);
}

void MainWindow::initMessageTable()
{
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

    QFont font = items[0]->font();
    font.setItalic(isItalic);

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
    }

    for (unsigned i = 0; i < NUMBER_TABLE_ROW; i++)
    {
        ui->table_can_messages->setFont(font);
        ui->table_can_messages->setItem(row, i, items[i]);
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

int MainWindow::computeTextSize(const QString& text)
{
    QFontMetrics fm(ui->table_can_messages->font());
    return fm.horizontalAdvance(text) + 10; // +10 for padding
}
