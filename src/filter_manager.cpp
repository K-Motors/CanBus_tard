#include "filter_manager.h"
#include "ui_filter_manager.h"
#include "const.h"

#include <QMessageBox>
#include <QCheckBox>

bool convertDecimal(QString value, unsigned* result)
{
    bool isOk;
    *result = value.trimmed().toInt(&isOk, 10);

    return isOk;
}

bool convertHexadecimal(QString value, unsigned* result)
{
    bool isOk;
    *result = value.trimmed().toInt(&isOk, 16);

    return isOk;
}

FilterManager::FilterManager(QSettings* settings, QWidget* parent) : QDialog(parent), ui(new Ui::FilterManager), settings{settings}
{
    ui->setupUi(this);

    initTable();
    loadFilter();
    updateFilter();

    groupRadioMsgId.addButton(ui->radio_msg_hex);
    groupRadioMsgId.addButton(ui->radio_msg_dec);

    groupRadioMsgRange.addButton(ui->radio_range_hex);
    groupRadioMsgRange.addButton(ui->radio_range_dec);

    ui->radio_msg_hex->setChecked(true);
    ui->radio_range_hex->setChecked(true);

    connect(ui->button_msg_add, &QPushButton::clicked, this, [this]() { addMessageIdFilter(); });
    connect(ui->button_range_add, &QPushButton::clicked, this, [this]() { addMessageRangeFilter(); });

    connect(ui->line_msg_id, &QLineEdit::returnPressed, this, &FilterManager::addMessageIdFilter);
    connect(ui->line_range_start, &QLineEdit::returnPressed, this, &FilterManager::addMessageRangeFilter);
    connect(ui->line_range_stop, &QLineEdit::returnPressed, this, &FilterManager::addMessageRangeFilter);

    connect(ui->button_remove, &QPushButton::pressed, this, &FilterManager::removeFilters);
}

FilterManager::~FilterManager()
{
    delete ui;
}

bool FilterManager::isMessageAccept(unsigned idMsg)
{
    FilterResult maxResult = FilterResult::DISABLE;

    for (auto& filter : filters)
    {
        FilterResult res = filter.filter(idMsg);

        if (res == FilterResult::ACCEPT) return true; // At least one ACCEPT don't need to continue
        maxResult = qMax(maxResult, res);
    }

    if (maxResult == FilterResult::DISABLE)
        return true;
    else // At least one REJECT
        return false;
}

QStringList FilterManager::toString()
{
    QStringList list;
    for (auto& filter : filters) list << filter.toString();
    return list;
}

void FilterManager::loadFromStrings(QStringList& lines)
{
    MessageFilter filter(0);
    QString       error;

    filters.clear();

    for (int i = 0; i < lines.count(); ++i)
    {
        QString line = lines[i];

        if (MessageFilter::fromString(line, &filter, &error))
            filters.append(filter);
        else
            qWarning() << "Failed to load filter on line" << (i + 1) << ":" << error;
    }

    updateFilter();
    saveFilter();
}

void FilterManager::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete)
    {
        removeFilters();
    }
    else
    {
        QDialog::keyPressEvent(event);
    }
}

void FilterManager::initTable()
{
    ui->table_filter->clear();
    ui->table_filter->clearContents();

    ui->table_filter->setRowCount(0);
    ui->table_filter->setColumnCount(3);
    ui->table_filter->setHorizontalHeaderLabels({"Enable", "Type", "ID / Range"});
    ui->table_filter->horizontalHeader()->setStretchLastSection(true);
}

void FilterManager::addMessageIdFilter()
{
    unsigned messageId = 0;
    bool     isSuccess = false;

    if (ui->radio_msg_hex->isChecked())
        isSuccess = convertHexadecimal(ui->line_msg_id->text(), &messageId);
    else
        isSuccess = convertDecimal(ui->line_msg_id->text(), &messageId);

    if (isSuccess)
    {
        filters.append(MessageFilter(messageId));
        ui->line_msg_id->clear();
        updateFilter();
        saveFilter();
    }
    else
    {
        QMessageBox::critical(this, "Bad input", "The message ID is invalid.");
    }
}

void FilterManager::addMessageRangeFilter()
{
    unsigned messageIdStart = 0;
    unsigned messageIdEnd   = 0;
    bool     isSuccess      = false;

    if (ui->radio_range_hex->isChecked())
        isSuccess = convertHexadecimal(ui->line_range_start->text(), &messageIdStart) && convertHexadecimal(ui->line_range_stop->text(), &messageIdEnd);
    else
        isSuccess = convertDecimal(ui->line_range_start->text(), &messageIdStart) && convertDecimal(ui->line_range_stop->text(), &messageIdEnd);

    if (isSuccess)
    {
        filters.append(MessageFilter(messageIdStart, messageIdEnd));
        ui->line_range_start->clear();
        ui->line_range_stop->clear();
        updateFilter();
        saveFilter();
    }
    else
    {
        QMessageBox::critical(this, "Bad input", "One of the message IDs is invalid.");
    }
}

void FilterManager::removeFilters()
{
    QSet<int> rows;

    for (auto& range : ui->table_filter->selectedRanges())
    {
        for (int i = range.topRow(); i <= range.bottomRow(); ++i)
        {
            rows.insert(i);
        }
    }

    QList<int> listRows = rows.values();
    std::sort(listRows.begin(), listRows.end(), std::greater<int>());
    for (int& row : listRows)
    {
        filters.remove(row);
    }

    updateFilter();
    saveFilter();
}

void FilterManager::saveFilter()
{

    settings->setValue(SETTINGS_KEY_FILTER_LIST, toString());
}

void FilterManager::loadFilter()
{
    QStringList list = settings->value(SETTINGS_KEY_FILTER_LIST).toStringList();
    loadFromStrings(list);
}

void FilterManager::updateFilter()
{
    initTable();

    for (int i = 0; i < filters.count(); ++i)
    {
        MessageFilter& filter    = filters[i];
        QWidget*       container = new QWidget();
        QHBoxLayout*   layout    = new QHBoxLayout(container);
        QCheckBox*     enableBox = new QCheckBox();
        unsigned       row       = ui->table_filter->rowCount();

        enableBox->setCheckState(filter.isFilterEnable() ? Qt::Checked : Qt::Unchecked);
        layout->addWidget(enableBox);
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);

        ui->table_filter->insertRow(row);
        ui->table_filter->setCellWidget(row, 0, container);
        ui->table_filter->setItem(row, 1, new QTableWidgetItem(filter.isRangeFilter() ? "Range" : "Unique ID"));

        if (filter.isRangeFilter())
        {
            std::array<unsigned, 2> range    = filter.getRange();
            QString                 rangeStr = QString("%1 - %2 (0x%3 - 0x%4)").arg(range[0]).arg(range[1]).arg(range[0], 0, 16).arg(range[1], 0, 16);
            ui->table_filter->setItem(row, 2, new QTableWidgetItem(rangeStr));
        }
        else
        {
            QString msgId = QString("%1 (0x%3)").arg(filter.getMessageId()).arg(filter.getMessageId(), 0, 16);
            ui->table_filter->setItem(row, 2, new QTableWidgetItem(msgId));
        }

        connect(enableBox,
                &QCheckBox::checkStateChanged,
                this,
                [this, i](Qt::CheckState state)
                {
                    filters[i].setEnable(state == Qt::Checked);
                    saveFilter();
                });
    }
}
