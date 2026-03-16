#include "dock_signal_watcher.h"

#include <QCanSignalDescription>

DockSignalWatcher::DockSignalWatcher() : ui{nullptr}
{
}

DockSignalWatcher::~DockSignalWatcher()
{
}

void DockSignalWatcher::init(Ui::MainWindow* ui)
{
    this->ui = ui;
    initTable();
}

void DockSignalWatcher::setMessageSignals(const QCanMessageDescription& message, const QVariantMap& values)
{
    if (ui == nullptr) return;

    initTable();
    ui->line_watcher_msg_id->setText(QString("%1 - 0x%2").arg((unsigned)message.uniqueId(), 0, 10).arg((unsigned)message.uniqueId(), 0, 16));
    ui->line_watcher_msg_name->setText(message.name());

    for (auto& signal : message.signalDescriptions())
    {
        int row = ui->table_watcher_signals->rowCount();

        ui->table_watcher_signals->insertRow(row);

        ui->table_watcher_signals->setItem(row, 0, new QTableWidgetItem(signal.name()));
        ui->table_watcher_signals->setItem(row, 1, new QTableWidgetItem(signal.physicalUnit()));
        ui->table_watcher_signals->setItem(row, 2, new QTableWidgetItem(values[signal.name()].toString()));
    }
}

void DockSignalWatcher::initTable()
{
    if (ui == nullptr) return;

    ui->table_watcher_signals->clear();
    ui->table_watcher_signals->clearContents();

    ui->table_watcher_signals->setRowCount(0);
    ui->table_watcher_signals->setColumnCount(3);
    ui->table_watcher_signals->setHorizontalHeaderLabels({"Name", "Unit", "Value"});
    ui->table_watcher_signals->horizontalHeader()->setStretchLastSection(true);
}
