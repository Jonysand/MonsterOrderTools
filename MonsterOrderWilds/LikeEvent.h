#pragma once
#include "framework.h"
#include <string>

struct LikeEvent {
    std::string uid;
    std::string username;
    std::string msgId;
    int32_t likeCount = 0;
    int64_t timestamp = 0;
    int32_t date = 0;
};
