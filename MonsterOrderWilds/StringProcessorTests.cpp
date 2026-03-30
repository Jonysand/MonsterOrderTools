#include "framework.h"
#include "StringProcessor.h"
#include "UnitTestLog.h"
#include <cassert>

#ifdef RUN_UNIT_TESTS

void TestUtf8ToWstring_Basic()
{
    // UTF-8 中文字符串
    std::string utf8 = "测试";
    std::wstring wide = StringProcessor::Utf8ToWstring(utf8);
    assert(wide.length() == 2);
    assert(wide[0] == L'测');
    assert(wide[1] == L'试');

    TestLog("[PASS] TestUtf8ToWstring_Basic");
}

void TestUtf8ToWstring_Empty()
{
    std::wstring wide = StringProcessor::Utf8ToWstring("");
    assert(wide.empty());

    TestLog("[PASS] TestUtf8ToWstring_Empty");
}

void TestWstringToUtf8_Basic()
{
    std::wstring wide = L"测试";
    std::string utf8 = StringProcessor::WstringToUtf8(wide);
    assert(!utf8.empty());
    // 验证可以转换回来
    std::wstring wide2 = StringProcessor::Utf8ToWstring(utf8);
    assert(wide == wide2);

    TestLog("[PASS] TestWstringToUtf8_Basic");
}

void TestNormalizeName_MixedCase()
{
    std::string result = StringProcessor::NormalizeName("AbC Def");
    assert(result == "abcdef");

    TestLog("[PASS] TestNormalizeName_MixedCase");
}

void TestNormalizeName_Chinese()
{
    std::string result = StringProcessor::NormalizeName("火龙");
    assert(!result.empty());

    TestLog("[PASS] TestNormalizeName_Chinese");
}

void TestContainsChinese_WithChinese()
{
    assert(StringProcessor::ContainsChinese("hello你好"));
    assert(StringProcessor::ContainsChinese("火龙"));
    assert(!StringProcessor::ContainsChinese("hello"));

    TestLog("[PASS] TestContainsChinese_WithChinese");
}

void TestTrim_Basic()
{
    assert(StringProcessor::Trim("  hello  ") == "hello");
    assert(StringProcessor::Trim("hello") == "hello");
    assert(StringProcessor::Trim("  ") == "");
    assert(StringProcessor::Trim("") == "");

    TestLog("[PASS] TestTrim_Basic");
}

void TestSplit_Basic()
{
    auto result = StringProcessor::Split("a,b,c", ",");
    assert(result.size() == 3);
    assert(result[0] == "a");
    assert(result[1] == "b");
    assert(result[2] == "c");

    TestLog("[PASS] TestSplit_Basic");
}

void TestSplit_NoDelimiter()
{
    auto result = StringProcessor::Split("abc", ",");
    assert(result.size() == 1);
    assert(result[0] == "abc");

    TestLog("[PASS] TestSplit_NoDelimiter");
}

void TestReplace_Basic()
{
    assert(StringProcessor::Replace("hello world", "world", "there") == "hello there");
    assert(StringProcessor::Replace("aaa", "a", "b") == "bbb");
    assert(StringProcessor::Replace("abc", "x", "y") == "abc");

    TestLog("[PASS] TestReplace_Basic");
}

// 运行所有测试
void RunAllStringProcessorTests()
{
    TestLog("=== StringProcessor Tests ===");
    TestUtf8ToWstring_Basic();
    TestUtf8ToWstring_Empty();
    TestWstringToUtf8_Basic();
    TestNormalizeName_MixedCase();
    TestNormalizeName_Chinese();
    TestContainsChinese_WithChinese();
    TestTrim_Basic();
    TestSplit_Basic();
    TestSplit_NoDelimiter();
    TestReplace_Basic();
    TestLog("=== StringProcessor Tests Done ===");
}

#endif // RUN_UNIT_TESTS