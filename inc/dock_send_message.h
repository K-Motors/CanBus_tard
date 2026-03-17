#pragma once

#include "can_device.h"
#include "ui_mainwindow.h"
#include "message_to_send.h"

#include <QStyledItemDelegate>

class DockSendMessage : public QWidget
{
    Q_OBJECT

  public:
    DockSendMessage(CanDevice& canDevice, QWidget* parent);
    ~DockSendMessage();

    void init(Ui::MainWindow* ui);

  private:
    Ui::MainWindow*      ui;
    CanDevice&           canDevice;
    QList<MessageToSend> messageToSend;
    int                  selectedIndex;

    void initListMessages();
    void initTableSignals();

    void updateList();

    void addMessage();
    void duplicateSelected();
    void removeSelected();
    void saveMessages();
    void loadMessages();
    void sendMessage();

    void onListItemSelected(QListWidgetItem* item);
    void onSignalCellClicked(int row, int col);
    void onSignalCellChanged(int row, int col);
    void onRepeatCheckStateChanged(Qt::CheckState state);
    void onSpinSendEveryValueChanged(int value);
    void onLineBindKeyChanged(const QString& value);
};

class NumericDelegate : public QStyledItemDelegate
{
    Q_OBJECT
  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        QLineEdit* editor = new QLineEdit(parent);
        editor->setValidator(new QIntValidator(editor));
        return editor;
    }
};
