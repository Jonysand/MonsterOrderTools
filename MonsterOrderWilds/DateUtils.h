#pragma once
#include "framework.h"

namespace DateUtils {
    bool IsLeapYear(int32_t year);
    int32_t GetDaysInMonth(int32_t year, int32_t month);
    int32_t TimestampToDate(int64_t timestamp);
    int32_t GetCurrentDate();
    bool IsNextCalendarDay(int32_t lastDate, int32_t currentDate);
}
