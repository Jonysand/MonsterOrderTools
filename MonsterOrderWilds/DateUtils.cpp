#include "framework.h"
#include "DateUtils.h"
#include <ctime>

namespace DateUtils {

    bool IsLeapYear(int32_t year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    int32_t GetDaysInMonth(int32_t year, int32_t month) {
        switch (month) {
            case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                return 31;
            case 4: case 6: case 9: case 11:
                return 30;
            case 2:
                return IsLeapYear(year) ? 29 : 28;
            default:
                return 30;
        }
    }

    int32_t TimestampToDate(int64_t timestamp) {
        if (timestamp <= 0) return 0;
        std::time_t timeSec = timestamp;
        std::tm tmResult = {};
        if (localtime_s(&tmResult, &timeSec) == 0) {
            return (tmResult.tm_year + 1900) * 10000 + (tmResult.tm_mon + 1) * 100 + tmResult.tm_mday;
        }
        return 0;
    }

    int32_t GetCurrentDate() {
        std::time_t now = std::time(nullptr);
        std::tm tmResult = {};
        if (localtime_s(&tmResult, &now) == 0) {
            return (tmResult.tm_year + 1900) * 10000 + (tmResult.tm_mon + 1) * 100 + tmResult.tm_mday;
        }
        return 0;
    }

    bool IsNextCalendarDay(int32_t lastDate, int32_t currentDate) {
        if (lastDate <= 0 || currentDate <= 0) return false;
        if (currentDate <= lastDate) return false;

        int32_t lastYear = lastDate / 10000;
        int32_t lastMonth = (lastDate % 10000) / 100;
        int32_t lastDay = lastDate % 100;
        int32_t year = currentDate / 10000;
        int32_t month = (currentDate % 10000) / 100;
        int32_t day = currentDate % 100;

        if (year == lastYear && month == lastMonth) {
            return day == lastDay + 1;
        }
        if (year == lastYear && month == lastMonth + 1 && day == 1) {
            int32_t lastMonthDays = GetDaysInMonth(lastYear, lastMonth);
            return lastDay == lastMonthDays;
        }
        if (year == lastYear + 1 && month == 1 && lastMonth == 12 && day == 1 && lastDay == 31) {
            return true;
        }
        return false;
    }

}
