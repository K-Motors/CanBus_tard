#include "dock_send_message.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include "const.h"

constexpr int EDITABLE_COL = 1;

DockSendMessage::DockSendMessage(CanDevice& CanDevice, QSettings* settings, QWidget* parent)
    : QWidget{parent}, ui{nullptr}, canDevice{CanDevice}, settings{settings}, selectedIndex{-1}
{
}

DockSendMessage::~DockSendMessage()
{
}

void DockSendMessage::init(Ui::MainWindow* ui)
{
    this->ui = ui;

    initListMessages();
    initTableSignals();
    loadMessages();

    connect(ui->button_send_add, &QPushButton::clicked, this, [this]() { addMessage(); });
    connect(ui->button_send_duplicate, &QPushButton::clicked, this, [this]() { duplicateSelected(); });
    connect(ui->button_send_remove, &QPushButton::clicked, this, [this]() { removeSelected(); });
    connect(ui->button_send_save, &QPushButton::clicked, this, [this]() { saveMessagesToFile(); });
    connect(ui->button_send_load, &QPushButton::clicked, this, [this]() { loadMessagesToFile(); });
    connect(ui->button_send_message, &QPushButton::clicked, this, [this]() { sendMessage(); });

    connect(ui->list_send_message, &QListWidget::itemClicked, this, &DockSendMessage::onListItemSelected);
    connect(ui->check_send_every, &QCheckBox::checkStateChanged, this, &DockSendMessage::onRepeatCheckStateChanged);
    connect(ui->spin_send_every, &QSpinBox::valueChanged, this, &DockSendMessage::onSpinSendEveryValueChanged);
    connect(ui->line_send_bindkey, &QLineEdit::textChanged, this, &DockSendMessage::onLineBindKeyChanged);

    connect(ui->table_send_signals, &QTableWidget::cellClicked, this, &DockSendMessage::onSignalCellClicked);
    connect(ui->table_send_signals, &QTableWidget::cellChanged, this, &DockSendMessage::onSignalCellChanged);
}

void DockSendMessage::initListMessages()
{
    if (ui == nullptr) return;

    ui->list_send_message->clear();
}

void DockSendMessage::initTableSignals()
{
    if (ui == nullptr) return;

    ui->table_send_signals->clear();
    ui->table_send_signals->clearContents();

    ui->table_send_signals->setRowCount(0);
    ui->table_send_signals->setColumnCount(3);
    ui->table_send_signals->setHorizontalHeaderLabels({"Name", "Value", "Unit"});
    ui->table_send_signals->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_send_signals->setItemDelegateForColumn(1, new NumericDelegate(this));
}

void DockSendMessage::updateList()
{
    if (ui == nullptr) return;

    initListMessages();

    for (int i = 0; i < messageToSend.size(); ++i)
    {
        MessageToSend& item = messageToSend[i];
        int            row  = ui->list_send_message->count();

        ui->list_send_message->insertItem(row, item.print());
        ui->list_send_message->item(row)->setData(Qt::UserRole, i);
    }

    if (selectedIndex > -1)
    {
        ui->list_send_message->setCurrentRow(selectedIndex);
    }
}

void DockSendMessage::addMessage()
{
    if (ui == nullptr) return;

    QStringList                           listMessages;
    QMap<QString, QCanMessageDescription> messages;
    for (auto& msg : canDevice.getMessageDescriptions())
    {
        QString str = QString("%1 (0x%2)").arg(msg.name()).arg((unsigned)msg.uniqueId(), 3, 16);
        messages.insert(str, msg);
        listMessages << str;
    }

    listMessages.sort();

    bool    ok;
    QString chosen = QInputDialog::getItem(ui->centralwidget, "Add message", "Choose a message to send", listMessages, 0, false, &ok);

    if (ok && !chosen.isEmpty())
    {
        messageToSend.append(MessageToSend(messages[chosen]));
        updateList();
    }

    saveMessages();
}

void DockSendMessage::duplicateSelected()
{
    if (ui == nullptr) return;

    int currentRow = ui->list_send_message->currentRow();

    if (currentRow < 0 || selectedIndex < 0) return;

    messageToSend.append(messageToSend[selectedIndex]);
    updateList();
    saveMessages();
}

void DockSendMessage::removeSelected()
{
    if (ui == nullptr) return;

    int currentRow = ui->list_send_message->currentRow();

    if (currentRow < 0 || selectedIndex < 0) return;

    messageToSend.removeAt(selectedIndex);

    if (messageToSend.length() == 0)
    {
        selectedIndex = -1;
        initTableSignals();
    }
    else if (messageToSend.length() >= selectedIndex)
    {
        selectedIndex = messageToSend.length() - 1;
    }
    updateList();
    saveMessages();
}

void DockSendMessage::saveMessages()
{
    QStringList saveList;

    for (auto& msg : messageToSend)
    {
        QJsonDocument doc(msg.serialize());
        saveList.append(doc.toJson(QJsonDocument::Compact));
    }

    settings->setValue(SETTINGS_KEY_SEND_MSG, saveList);
}

void DockSendMessage::loadMessages()
{
    QStringList listMsg = settings->value(SETTINGS_KEY_SEND_MSG).toStringList();

    for (auto& str : listMsg)
    {
        QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());

        MessageToSend msg;
        msg.loadFromJson(doc.object());
        messageToSend.append(msg);
    }

    updateList();
}

void DockSendMessage::saveMessagesToFile()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Messages", "messages.json", "Messages file (*.json *.JSON);;All files (*.*)");

    if (filePath.isEmpty()) return;

    QFile saveFile(filePath);

    if (!saveFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text))
    {
        QMessageBox::critical(this, "Failed open file", "Unable to open the file (" + filePath + ")");
        return;
    }

    QJsonArray array;
    for (auto& msg : messageToSend) array.append(msg.serialize());

    QTextStream   out(&saveFile);
    QJsonDocument doc(array);
    out << doc.toJson(QJsonDocument::Compact);
}

void DockSendMessage::loadMessagesToFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open messages", "", "Messages file (*.json *.JSON);;All files (*.*)");

    if (filePath.isEmpty()) return;

    QFile openFile(filePath);

    if (!openFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
    {
        QMessageBox::critical(this, "Failed open file", "Unable to open the file (" + filePath + ")");
        return;
    }

    QTextStream in(&openFile);
    QString     content = in.readAll();

    QJsonArray jsonArray = QJsonDocument::fromJson(content.toUtf8()).array();

    for (auto elem : jsonArray)
    {
        QJsonObject   obj = elem.toObject();
        MessageToSend msg;
        msg.loadFromJson(obj);
        messageToSend.append(msg);
    }

    updateList();
}
void DockSendMessage::sendMessage()
{
    if (ui == nullptr || selectedIndex < 0) return;

    MessageToSend& message = messageToSend[selectedIndex];
    QString        error;

    if (canDevice.sendFrame(message.messageId, message.valuestoVariantMap(), &error) == false)
    {
        QMessageBox::critical(this, "Sending message", "Failed to send message.\n" + error);
    }
}

void DockSendMessage::onListItemSelected(QListWidgetItem* item)
{
    if (ui == nullptr) return;
    initTableSignals();

    selectedIndex      = item->data(Qt::UserRole).toInt();
    MessageToSend& msg = messageToSend[selectedIndex];

    for (QString& key : msg.values.keys())
    {
        int row = ui->table_send_signals->rowCount();

        ui->table_send_signals->insertRow(row);
        ui->table_send_signals->setItem(row, 0, new QTableWidgetItem(key));
        ui->table_send_signals->setItem(row, 1, new QTableWidgetItem(msg.values[key].value.toString()));
        ui->table_send_signals->setItem(row, 2, new QTableWidgetItem(msg.values[key].unit));

        ui->table_send_signals->item(row, EDITABLE_COL)->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    if (msg.repeatEvery > 0)
    {
        ui->check_send_every->setChecked(true);
        ui->spin_send_every->setValue(msg.repeatEvery);
    }
    else
    {
        ui->check_send_every->setChecked(false);
        ui->spin_send_every->setValue(20);
    }
}

void DockSendMessage::onSignalCellClicked(int row, int col)
{
    if (ui == nullptr) return;

    if (col == EDITABLE_COL)
    {
        QModelIndex index = this->ui->table_send_signals->model()->index(row, col);
        this->ui->table_send_signals->edit(index);
    }
}

void DockSendMessage::onSignalCellChanged(int row, int col)
{
    if (col != EDITABLE_COL || ui == nullptr || selectedIndex < 0) return;

    QString        signalName = ui->table_send_signals->item(row, 0)->text();
    MessageToSend& msg        = messageToSend[selectedIndex];

    msg.values[signalName].value = ui->table_send_signals->item(row, col)->data(Qt::DisplayRole);
    saveMessages();
}

void DockSendMessage::onRepeatCheckStateChanged(Qt::CheckState state)
{
    if (selectedIndex < 0) return;

    messageToSend[selectedIndex].repeatEvery = state == Qt::Checked ? ui->spin_send_every->value() : 0;
    ui->spin_send_every->setEnabled(state == Qt::Checked);
    updateList();
    saveMessages();
}

void DockSendMessage::onSpinSendEveryValueChanged(int value)
{
    if (selectedIndex < 0 || false == ui->check_send_every->isChecked()) return;

    messageToSend[selectedIndex].repeatEvery = value;
    updateList();
    saveMessages();
}

void DockSendMessage::onLineBindKeyChanged(const QString& value)
{
    if (selectedIndex < 0) return;

    messageToSend[selectedIndex].key = value.isEmpty() ? '\0' : value[0];
    updateList();
    saveMessages();
}
