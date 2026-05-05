#ifdef RUN_UNIT_TESTS
#include "LocalVoiceManager.h"
#include "UnitTestLog.h"

void TestLocalVoiceManagerMatch() {
    LocalVoiceManager* mgr = LocalVoiceManager::Inst();

    // 应该匹配
    if (mgr->MatchVoice(TEXT("曼波")) != "manbo/manbo.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '曼波' should match manbo/manbo.mp3");
        return;
    }
    if (mgr->MatchVoice(TEXT("曼波曼波")) != "manbo/manbo_3x.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '曼波曼波' should match manbo/manbo_3x.mp3");
        return;
    }
    if (mgr->MatchVoice(TEXT("duang")) != "manbo/duang.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: 'duang' should match manbo/duang.mp3");
        return;
    }
    if (mgr->MatchVoice(TEXT("噢耶")) != "manbo/ohyeah.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '噢耶' should match manbo/ohyeah.mp3");
        return;
    }
    if (mgr->MatchVoice(TEXT("哦耶")) != "manbo/ohyeah.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '哦耶' should match manbo/ohyeah.mp3");
        return;
    }
    if (mgr->MatchVoice(TEXT("欧耶")) != "manbo/ohyeah.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '欧耶' should match manbo/ohyeah.mp3");
        return;
    }
    if (mgr->MatchVoice(TEXT("wow")) != "manbo/wow.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: 'wow' should match manbo/wow.mp3");
        return;
    }
    if (mgr->MatchVoice(TEXT("WOW")) != "manbo/wow.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: 'WOW' should match manbo/wow.mp3 (case insensitive)");
        return;
    }
    if (mgr->MatchVoice(TEXT("痛快！！")) != "mho/tongkuai.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '痛快！！' should match mho/tongkuai.mp3");
        return;
    }
    if (mgr->MatchVoice(TEXT("痛快!!")) != "mho/tongkuai.mp3") {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '痛快!!' should match mho/tongkuai.mp3");
        return;
    }
    if (!mgr->IsSpecialVoice(TEXT("痛快！！"))) {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '痛快！！' should be special voice");
        return;
    }
    if (!mgr->IsSpecialVoice(TEXT("痛快!!"))) {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '痛快!!' should be special voice");
        return;
    }
    if (mgr->IsSpecialVoice(TEXT("曼波"))) {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '曼波' should not be special voice");
        return;
    }

    // 不应该匹配
    if (!mgr->MatchVoice(TEXT("哈哈曼波")).empty()) {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '哈哈曼波' should not match");
        return;
    }
    if (!mgr->MatchVoice(TEXT("曼波哈哈")).empty()) {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '曼波哈哈' should not match");
        return;
    }
    if (!mgr->MatchVoice(TEXT("未知弹幕")).empty()) {
        TestLog("[FAIL] TestLocalVoiceManagerMatch: '未知弹幕' should not match");
        return;
    }

    TestLog("[PASS] TestLocalVoiceManagerMatch");
}

void RunLocalVoiceManagerTests() {
    TestLog("[--- LocalVoiceManager Tests ---]");
    TestLocalVoiceManagerMatch();
}
#endif
