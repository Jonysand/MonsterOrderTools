#pragma once
#include "framework.h"

namespace DateUtils {
    bool IsLeapYear(int32_t year);
    int32_t GetDaysInMonth(int32_t year, int32_t month);
    int32_t TimestampToDate(int64_t timestamp);
    int32_t GetCurrentDate();
    bool IsNextCalendarDay(int32_t lastDate, int32_t currentDate);
    int32_t GetPreviousDate(int32_t date);  // 新增：获取前一天日期
    int32_t GetWeekStartDate(int32_t date);  // 返回 date 所在自然周周一的 YYYYMMDD（周一为周首日）
    bool IsSameWeek(int32_t dateA, int32_t dateB);  // 两日期是否在同一自然周
}
