#pragma once

#include <QDialog>
#include <QSettings>

namespace Ui
{
    class dbc_manager;
}

class DbcManager : public QDialog
{
    Q_OBJECT

  public:
    explicit DbcManager(QSettings* settings, QWidget* parent = nullptr);
    ~DbcManager();

    QStringList getDbcFiles();

  private:
    Ui::dbc_manager* ui;
    QSettings* settings;

    void loadFiles();
    void saveFiles();

    void addFile();
    void removeFile();
};
