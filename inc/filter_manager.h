#pragma once

#include <QDialog>
#include <QKeyEvent>
#include <QButtonGroup>
#include <QIntValidator>
#include <QRegularExpressionValidator>
#include <QSettings>

#include "message_filter.h"

namespace Ui
{
    class FilterManager;
}

class FilterManager : public QDialog
{
    Q_OBJECT

  public:
    explicit FilterManager(QSettings* settings, QWidget* parent = nullptr);
    ~FilterManager();

    bool        isMessageAccept(unsigned idMsg);
    QStringList toString();
    void        loadFromStrings(QStringList& lines);

  protected:
    void keyPressEvent(QKeyEvent* event) override;

  private:
    Ui::FilterManager* ui;
    QSettings*         settings;

    QButtonGroup groupRadioMsgId;
    QButtonGroup groupRadioMsgRange;

    QList<MessageFilter> filters;

    void initTable();

    void addMessageIdFilter();
    void addMessageRangeFilter();
    void removeFilters();

    void saveFilter();
    void loadFilter();
    void updateFilter();
};
