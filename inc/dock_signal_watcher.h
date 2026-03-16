#pragma once

#include <QWidget>
#include <QCanMessageDescription>
#include "ui_mainwindow.h"

class DockSignalWatcher
{
  public:
    DockSignalWatcher();
    ~DockSignalWatcher();

    void init(Ui::MainWindow* ui);
    void setMessageSignals(const QCanMessageDescription& message, const QVariantMap& values);

  private:
    Ui::MainWindow* ui;

    void initTable();
};
