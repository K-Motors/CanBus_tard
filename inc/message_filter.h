#pragma once

#include <array>
#include <QString>
#include <QStringList>

enum class FilterResult : unsigned
{
    DISABLE = 0,
    REJECT,
    ACCEPT
};

class MessageFilter
{
  public:
    MessageFilter(unsigned messageId) : isEnable{true}, isRange{false}, startOrId{messageId}, end{0} {}
    MessageFilter(unsigned startId, unsigned stopId) : isEnable{true}, isRange{true}, startOrId{startId}, end{stopId} {}

    bool isFilterEnable() { return isEnable; }
    bool isRangeFilter() { return isRange; }
    bool isUniqueFilter() { return !isRange; }

    std::array<unsigned, 2> getRange() { return {startOrId, end}; }
    unsigned                getMessageId() { return startOrId; }

    void setEnable(bool isEnable) { this->isEnable = isEnable; }

    QString toString()
    {
        QString enable = isEnable ? "1" : "0";
        QString type   = isRange ? "R" : "U";
        QString ids    = isRange ? QString("%1-%2").arg(startOrId).arg(end) : QString::number(startOrId);

        return QString("%1 %2 %3").arg(enable, type, ids);
    }

    FilterResult filter(unsigned idMsg)
    {
        if (false == isEnable)
            return FilterResult::DISABLE;
        else if (false == isRange && idMsg == startOrId)
            return FilterResult::ACCEPT;
        else if (true == isRange && startOrId <= idMsg && idMsg <= end)
            return FilterResult::ACCEPT;
        else
            return FilterResult::REJECT;
    }

    static bool fromString(QString& str, MessageFilter* result, QString* errorMsg)
    {
        errorMsg->clear();

        QStringList parts = str.split(" ");
        if (parts.length() != 3)
        {
            *errorMsg = "The input size is incorrect.";
            return false;
        }

        bool isEnable;
        bool isRange;

        if (parts[0] != '0' && parts[0] != '1')
        {
            *errorMsg = "The first parameter is incorrect.";
            return false;
        }
        else
        {
            isEnable = parts[0] == '1';
        }

        if (parts[1] != 'R' && parts[1] != 'U')
        {
            *errorMsg = "The second parameter is incorrect.";
            return false;
        }
        else
        {
            isRange = parts[1] == 'R';
        }

        if (isRange)
        {
            QStringList rawRange = parts[2].split("-");

            if (rawRange.length() != 2)
            {
                *errorMsg = "The range parameter is incorrect (size error).";
                return false;
            }

            bool     isOkStart, isOkEnd;
            unsigned start = rawRange[0].toInt(&isOkStart);
            unsigned end   = rawRange[1].toInt(&isOkEnd);

            if (!isOkStart || !isOkEnd)
            {
                *errorMsg = "The range parameter is incorrect (incorrect number).";
                return false;
            }

            *result = MessageFilter(start, end);
        }
        else
        {
            bool     isOk;
            unsigned id = parts[2].toInt(&isOk);

            if (!isOk)
            {
                *errorMsg = "The message ID parameter is incorrect.";
                return false;
            }
            *result = MessageFilter(id);
        }

        result->isEnable = isEnable;
        return true;
    }

  private:
    bool     isEnable;
    bool     isRange;
    unsigned startOrId;
    unsigned end;
};
