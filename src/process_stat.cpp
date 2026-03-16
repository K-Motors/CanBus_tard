#include "process_stat.h"

#include <QCoreApplication>
#include <QProcess>
#include <QDebug>

ProcessStat::ProcessStat(unsigned long everyMs) : QThread{nullptr}
{
    this->everyMs = everyMs < 100 ? 100 : everyMs;
    moveToThread(this);
    // start();
}

ProcessStat::~ProcessStat()
{
    terminate();
    this->wait(1000);
}

void ProcessStat::run()
{
    forever
    {
        unsigned long mem = getRamUsage();
        float         cpu = getCPUUsage();

        emit updateStat(mem, cpu);

        QThread::msleep(everyMs);
    }
}

unsigned long ProcessStat::getRamUsage(qint64 pid)
{
    if (-1 == pid)
    {
        pid = QCoreApplication::applicationPid();
    }

#if defined(Q_OS_WIN)

    return runCommandAndGetNumber("powershell -NoProfile -Command (Get-Process -Id " + QString::number(pid) + ").PM");

#elif defined(Q_OS_LINUX)
    return runCommandAndGetNumber("top -p " + QString::number(pid) + " -b -n 1 | tail -n 1 | awk '{print $6}'");
#else
#error "Unsupported OS"
#endif
}

float ProcessStat::getCPUUsage(qint64 pid)
{
    if (-1 == pid)
    {
        pid = QCoreApplication::applicationPid();
    }

#if defined(Q_OS_WIN)

    return runCommandAndGetFloat(
        "powershell -NoProfile -Command $p=" + QString::number(pid) +
        ";$a=(Get-Process -Id $p).CPU; Start-sleep -Milliseconds 100; $b=(Get-Process -Id $p).CPU; $r=((($b-$a) / 0.1)/(Get-CimInstance "
        "Win32_ComputerSystem).NumberOfLogicalProcessors)*100;echo $r");

#elif defined(Q_OS_LINUX)
    return runCommandAndGetFloat("top -p " + QString::number(pid) + " -b -n 1 | tail -n 1 | awk '{print $9}'");
#else
#error "Unsupported OS"
#endif
}

long ProcessStat::runCommandAndGetNumber(const QString& cmd, bool* ok)
{
    QProcess p;
    p.startCommand(cmd);

    if (!p.waitForFinished())
    {
        if (ok) *ok = false;
        return 0;
    }

    QString output = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();

    bool localOk = false;
    long value   = output.toLong(&localOk);

    if (ok) *ok = localOk;
    return value;
}

float ProcessStat::runCommandAndGetFloat(const QString& cmd, bool* ok)
{
    QProcess p;
    p.startCommand(cmd);

    if (!p.waitForFinished())
    {
        if (ok) *ok = false;
        return 0.0f;
    }

    QString output = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed().replace(",", ".");

    bool  localOk = false;
    float value   = output.toFloat(&localOk);

    if (ok) *ok = localOk;
    return localOk ? value : 0.0f;
}
