#include "framework.h"
#include "ConfigManager.h"
#include "UnitTestLog.h"
#include <cassert>
#include <WinReg.h>

#ifdef RUN_UNIT_TESTS

namespace
{
    const char* REG_SUBKEY = "Software\\MonsterOrderWilds";
    const char* REG_VALUE_NAME = "IdCode";

    void DeleteRegistryValue()
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_SUBKEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
        {
            RegDeleteValueA(hKey, REG_VALUE_NAME);
            RegCloseKey(hKey);
        }
    }

    void WriteRegistryValue(const std::string& value)
    {
        HKEY hKey;
        DWORD disp;
        if (RegCreateKeyExA(HKEY_CURRENT_USER, REG_SUBKEY, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disp) == ERROR_SUCCESS)
        {
            RegSetValueExA(hKey, REG_VALUE_NAME, 0, REG_SZ, (const BYTE*)value.c_str(), value.length() + 1);
            RegCloseKey(hKey);
        }
    }

    std::string ReadRegistryValue()
    {
        std::string result;
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_SUBKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            char buffer[256];
            DWORD bufferSize = sizeof(buffer);
            if (RegQueryValueExA(hKey, REG_VALUE_NAME, nullptr, nullptr, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS)
            {
                result = buffer;
            }
            RegCloseKey(hKey);
        }
        return result;
    }
}

void TestConfigManagerRegistry_ReadEmptyValue()
{
    DeleteRegistryValue();

    ConfigManager* mgr = ConfigManager::Inst();
    mgr->SetIdCode("test_before");
    mgr->SaveConfig(true);

    DeleteRegistryValue();

    bool loaded = mgr->LoadConfig();
    assert(loaded == true);
    assert(mgr->GetConfig().idCode == "");

    TestLog("[PASS] TestConfigManagerRegistry_ReadEmptyValue");
}

void TestConfigManagerRegistry_WriteAndRead()
{
    DeleteRegistryValue();

    ConfigManager* mgr = ConfigManager::Inst();
    mgr->SetIdCode("registry_test_code");
    bool saved = mgr->SaveConfig(true);
    assert(saved == true);

    std::string registryValue = ReadRegistryValue();
    assert(registryValue == "registry_test_code");

    DeleteRegistryValue();

    TestLog("[PASS] TestConfigManagerRegistry_WriteAndRead");
}

void TestConfigManagerRegistry_Integration()
{
    DeleteRegistryValue();

    ConfigManager* mgr = ConfigManager::Inst();

    mgr->SetIdCode("integration_test");
    mgr->SaveConfig(true);

    mgr->SetIdCode("");

    bool loaded = mgr->LoadConfig();
    assert(loaded == true);
    assert(mgr->GetConfig().idCode == "integration_test");

    DeleteRegistryValue();

    TestLog("[PASS] TestConfigManagerRegistry_Integration");
}

void RunAllConfigManagerRegistryTests()
{
    TestLog("=== ConfigManager Registry Tests ===");
    TestConfigManagerRegistry_ReadEmptyValue();
    TestConfigManagerRegistry_WriteAndRead();
    TestConfigManagerRegistry_Integration();
    TestLog("=== ConfigManager Registry Tests Done ===");
}

#endif // RUN_UNIT_TESTS
