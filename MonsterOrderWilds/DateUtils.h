#pragma once
#include "framework.h"

namespace DateUtils {
    bool IsLeapYear(int32_t year);
    int32_t GetDaysInMonth(int32_t year, int32_t month);
    int32_t TimestampToDate(int64_t timestamp);
    int32_t GetCurrentDate();
    bool IsNextCalendarDay(int32_t lastDate, int32_t currentDate);
    int32_t GetPreviousDate(int32_t date);  // 新增：获取前一天日期
}
