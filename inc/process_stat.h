#pragma once

#include <QThread>

class ProcessStat : public QThread
{
    Q_OBJECT

  public:
    ProcessStat(unsigned long everyMs);
    ~ProcessStat();

  signals:
    void updateStat(unsigned long ram, float cpu);

  protected:
    void run() override;

  private:
    unsigned long everyMs;

    unsigned long getRamUsage(qint64 pid = -1);
    float         getCPUUsage(qint64 pid = -1);
    long          runCommandAndGetNumber(const QString& cmd, bool* ok = nullptr);
    float         runCommandAndGetFloat(const QString& cmd, bool* ok = nullptr);
};
