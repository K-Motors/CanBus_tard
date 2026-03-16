#include "dbc_manager.h"
#include "ui_dbc_manager.h"

#include <QFileDialog>
#include "const.h"

DbcManager::DbcManager(QSettings* settings, QWidget* parent) : QDialog(parent), ui(new Ui::dbc_manager), settings{settings}
{
    ui->setupUi(this);

    loadFiles();
    connect(ui->button_dbc_add, &QPushButton::clicked, this, [this]() { addFile(); });
    connect(ui->button_dbc_remove, &QPushButton::clicked, this, [this]() { removeFile(); });
}

DbcManager::~DbcManager()
{
    delete ui;
}

QStringList DbcManager::getDbcFiles()
{
    QStringList result;

    for(int i = 0; i < ui->list_dbc->count(); ++i)
        result << ui->list_dbc->item(i)->text();

    return result;
}

void DbcManager::loadFiles()
{
    ui->list_dbc->clear();
    ui->list_dbc->addItems(settings->value(SETTINGS_KEY_DBC_FILES).toStringList());
}

void DbcManager::saveFiles()
{
    settings->setValue(SETTINGS_KEY_DBC_FILES, getDbcFiles());
}

void DbcManager::addFile()
{
    QStringList filePathList = QFileDialog::getOpenFileNames(this, "Add DBC file", "", "DBC file (*.dbc *.DBC);;All files (*.*)");

    for (auto& file : filePathList)
    {
        ui->list_dbc->addItem(file);
    }

    saveFiles();
}

void DbcManager::removeFile()
{
    for (auto& item : ui->list_dbc->selectedItems())
    {
        delete ui->list_dbc->takeItem(ui->list_dbc->row(item));
    }

    saveFiles();
}
