#include "framework.h"
#include "DateUtils.h"
#include <cassert>
#include <iostream>

#ifdef RUN_UNIT_TESTS

void TestGetWeekStartDate() {
    assert(DateUtils::GetWeekStartDate(20260421) == 20260421); // 周一
    assert(DateUtils::GetWeekStartDate(20260422) == 20260421); // 周二
    assert(DateUtils::GetWeekStartDate(20260425) == 20260421); // 周五
    assert(DateUtils::GetWeekStartDate(20260427) == 20260421); // 周日
    std::cout << "[PASS] TestGetWeekStartDate" << std::endl;
}

void TestGetWeekStartDate_CrossMonth() {
    // 2026-06-01 周一，上周日是 2026-05-31
    assert(DateUtils::GetWeekStartDate(20260601) == 20260601);
    assert(DateUtils::GetWeekStartDate(20260531) == 20260525);
    std::cout << "[PASS] TestGetWeekStartDate_CrossMonth" << std::endl;
}

void TestGetWeekStartDate_CrossYear() {
    // 2025-12-29 周一 -> 2026-01-04 周日 同周
    assert(DateUtils::GetWeekStartDate(20260101) == 20251229);
    assert(DateUtils::GetWeekStartDate(20260104) == 20251229);
    // 2026-01-05 周一 新的一周
    assert(DateUtils::GetWeekStartDate(20260105) == 20260105);
    std::cout << "[PASS] TestGetWeekStartDate_CrossYear" << std::endl;
}

void TestIsSameWeek() {
    assert(DateUtils::IsSameWeek(20260421, 20260421));
    assert(DateUtils::IsSameWeek(20260421, 20260427));
    assert(DateUtils::IsSameWeek(20260422, 20260425));
    assert(!DateUtils::IsSameWeek(20260420, 20260421)); // 周日与周一不同周
    assert(!DateUtils::IsSameWeek(20260427, 20260428));
    std::cout << "[PASS] TestIsSameWeek" << std::endl;
}

void RunDateUtilsTests() {
    std::cout << "========== DateUtils Tests ==========" << std::endl;
    TestGetWeekStartDate();
    TestGetWeekStartDate_CrossMonth();
    TestGetWeekStartDate_CrossYear();
    TestIsSameWeek();
    std::cout << "========== DateUtils Tests: ALL PASS ==========" << std::endl;
}

#endif // RUN_UNIT_TESTS
