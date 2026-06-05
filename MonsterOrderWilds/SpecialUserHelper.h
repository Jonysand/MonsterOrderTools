#pragma once
#include <string>

namespace SpecialUser {
    // 特殊用户 OpenID 常量
    constexpr const char* SPECIAL_OPEN_ID = "6ed4fb45ecd94f938a2cf747c5487707";
    
    // 判断是否为特殊用户
    inline bool IsSpecialUser(const std::string& userId) {
        return userId == SPECIAL_OPEN_ID;
    }
}