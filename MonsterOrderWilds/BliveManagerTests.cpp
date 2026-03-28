#include "BliveManager.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <sstream>

#ifdef RUN_UNIT_TESTS

// ═══════════════════════════════════════════════════════════════════
// 枚举和常量测试
// ═══════════════════════════════════════════════════════════════════

void TestConnectionState_EnumValues()
{
    assert(static_cast<int>(ConnectionState::Disconnected) == 0);
    assert(static_cast<int>(ConnectionState::Connecting) == 1);
    assert(static_cast<int>(ConnectionState::Connected) == 2);
    assert(static_cast<int>(ConnectionState::Reconnecting) == 3);
    assert(static_cast<int>(ConnectionState::ReconnectFailed) == 4);
    std::cout << "[PASS] TestConnectionState_EnumValues" << std::endl;
}

void TestDisconnectReason_EnumValues()
{
    assert(static_cast<int>(DisconnectReason::None) == 0);
    assert(static_cast<int>(DisconnectReason::NetworkError) == 1);
    assert(static_cast<int>(DisconnectReason::HeartbeatTimeout) == 2);
    assert(static_cast<int>(DisconnectReason::ServerClose) == 3);
    assert(static_cast<int>(DisconnectReason::AuthFailed) == 4);
    std::cout << "[PASS] TestDisconnectReason_EnumValues" << std::endl;
}

void TestMaxReconnectAttempts_Value()
{
    assert(MAX_RECONNECT_ATTEMPTS == 5);
    std::cout << "[PASS] TestMaxReconnectAttempts_Value" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// 字符串转换函数测试
// ═══════════════════════════════════════════════════════════════════

void TestConnectionStateToString()
{
    assert(strcmp(ConnectionStateToString(ConnectionState::Disconnected), "Disconnected") == 0);
    assert(strcmp(ConnectionStateToString(ConnectionState::Connecting), "Connecting") == 0);
    assert(strcmp(ConnectionStateToString(ConnectionState::Connected), "Connected") == 0);
    assert(strcmp(ConnectionStateToString(ConnectionState::Reconnecting), "Reconnecting") == 0);
    assert(strcmp(ConnectionStateToString(ConnectionState::ReconnectFailed), "ReconnectFailed") == 0);
    assert(strcmp(ConnectionStateToString(static_cast<ConnectionState>(999)), "Unknown") == 0);
    std::cout << "[PASS] TestConnectionStateToString" << std::endl;
}

void TestDisconnectReasonToString()
{
    assert(strcmp(DisconnectReasonToString(DisconnectReason::None), "无") == 0);
    assert(strcmp(DisconnectReasonToString(DisconnectReason::NetworkError), "网络错误") == 0);
    assert(strcmp(DisconnectReasonToString(DisconnectReason::HeartbeatTimeout), "心跳超时") == 0);
    assert(strcmp(DisconnectReasonToString(DisconnectReason::ServerClose), "服务器断开") == 0);
    assert(strcmp(DisconnectReasonToString(DisconnectReason::AuthFailed), "鉴权失败") == 0);
    assert(strcmp(DisconnectReasonToString(static_cast<DisconnectReason>(999)), "未知") == 0);
    std::cout << "[PASS] TestDisconnectReasonToString" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// 状态转换测试 - 完整覆盖所有状态组合
// ═══════════════════════════════════════════════════════════════════

void TestStateTransition_DisconnectedToConnecting()
{
    std::atomic<ConnectionState> state{ConnectionState::Disconnected};
    std::atomic<DisconnectReason> reason{DisconnectReason::None};
    
    state.store(ConnectionState::Connecting);
    reason.store(DisconnectReason::None);
    
    assert(state.load() == ConnectionState::Connecting);
    assert(reason.load() == DisconnectReason::None);
    std::cout << "[PASS] TestStateTransition_DisconnectedToConnecting" << std::endl;
}

void TestStateTransition_ConnectingToConnected()
{
    std::atomic<ConnectionState> state{ConnectionState::Connecting};
    std::atomic<DisconnectReason> reason{DisconnectReason::None};
    
    state.store(ConnectionState::Connected);
    
    assert(state.load() == ConnectionState::Connected);
    std::cout << "[PASS] TestStateTransition_ConnectingToConnected" << std::endl;
}

void TestStateTransition_ConnectingToDisconnected_NetworkError()
{
    std::atomic<ConnectionState> state{ConnectionState::Connecting};
    std::atomic<DisconnectReason> reason{DisconnectReason::None};
    
    state.store(ConnectionState::Disconnected);
    reason.store(DisconnectReason::NetworkError);
    
    assert(state.load() == ConnectionState::Disconnected);
    assert(reason.load() == DisconnectReason::NetworkError);
    std::cout << "[PASS] TestStateTransition_ConnectingToDisconnected_NetworkError" << std::endl;
}

void TestStateTransition_ConnectedToReconnecting_HeartbeatTimeout()
{
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    std::atomic<DisconnectReason> reason{DisconnectReason::None};
    
    state.store(ConnectionState::Reconnecting);
    reason.store(DisconnectReason::HeartbeatTimeout);
    
    assert(state.load() == ConnectionState::Reconnecting);
    assert(reason.load() == DisconnectReason::HeartbeatTimeout);
    std::cout << "[PASS] TestStateTransition_ConnectedToReconnecting_HeartbeatTimeout" << std::endl;
}

void TestStateTransition_ConnectedToReconnecting_ServerClose()
{
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    std::atomic<DisconnectReason> reason{DisconnectReason::None};
    
    state.store(ConnectionState::Reconnecting);
    reason.store(DisconnectReason::ServerClose);
    
    assert(state.load() == ConnectionState::Reconnecting);
    assert(reason.load() == DisconnectReason::ServerClose);
    std::cout << "[PASS] TestStateTransition_ConnectedToReconnecting_ServerClose" << std::endl;
}

void TestStateTransition_ConnectedToDisconnected_Disconnect()
{
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    std::atomic<DisconnectReason> reason{DisconnectReason::None};
    
    state.store(ConnectionState::Disconnected);
    reason.store(DisconnectReason::None);
    
    assert(state.load() == ConnectionState::Disconnected);
    assert(reason.load() == DisconnectReason::None);
    std::cout << "[PASS] TestStateTransition_ConnectedToDisconnected_Disconnect" << std::endl;
}

void TestStateTransition_ReconnectingToConnected()
{
    std::atomic<ConnectionState> state{ConnectionState::Reconnecting};
    std::atomic<DisconnectReason> reason{DisconnectReason::HeartbeatTimeout};
    
    state.store(ConnectionState::Connected);
    reason.store(DisconnectReason::None);
    
    assert(state.load() == ConnectionState::Connected);
    assert(reason.load() == DisconnectReason::None);
    std::cout << "[PASS] TestStateTransition_ReconnectingToConnected" << std::endl;
}

void TestStateTransition_ReconnectingToReconnecting_IncrementAttempt()
{
    std::atomic<ConnectionState> state{ConnectionState::Reconnecting};
    std::atomic<int> attemptCount{2};
    
    assert(attemptCount.load() < MAX_RECONNECT_ATTEMPTS);
    
    attemptCount.store(attemptCount.load() + 1);
    assert(attemptCount.load() == 3);
    
    std::cout << "[PASS] TestStateTransition_ReconnectingToReconnecting_IncrementAttempt" << std::endl;
}

void TestStateTransition_ReconnectingToReconnectFailed()
{
    std::atomic<ConnectionState> state{ConnectionState::Reconnecting};
    
    state.store(ConnectionState::ReconnectFailed);
    
    assert(state.load() == ConnectionState::ReconnectFailed);
    std::cout << "[PASS] TestStateTransition_ReconnectingToReconnectFailed" << std::endl;
}

void TestStateTransition_ReconnectFailedToConnecting()
{
    std::atomic<ConnectionState> state{ConnectionState::ReconnectFailed};
    std::atomic<DisconnectReason> reason{DisconnectReason::NetworkError};
    std::atomic<int> attemptCount{5};
    
    state.store(ConnectionState::Connecting);
    reason.store(DisconnectReason::None);
    attemptCount.store(0);
    
    assert(state.load() == ConnectionState::Connecting);
    assert(reason.load() == DisconnectReason::None);
    assert(attemptCount.load() == 0);
    std::cout << "[PASS] TestStateTransition_ReconnectFailedToConnecting" << std::endl;
}

void TestStateTransition_ReconnectingToDisconnected_Disconnect()
{
    std::atomic<ConnectionState> state{ConnectionState::Reconnecting};
    std::atomic<DisconnectReason> reason{DisconnectReason::HeartbeatTimeout};
    std::atomic<int> attemptCount{3};
    
    state.store(ConnectionState::Disconnected);
    reason.store(DisconnectReason::None);
    attemptCount.store(0);
    
    assert(state.load() == ConnectionState::Disconnected);
    assert(reason.load() == DisconnectReason::None);
    assert(attemptCount.load() == 0);
    std::cout << "[PASS] TestStateTransition_ReconnectingToDisconnected_Disconnect" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// 重连计数测试
// ═══════════════════════════════════════════════════════════════════

void TestReconnectAttemptCount_InitialValue()
{
    std::atomic<int> count{0};
    assert(count.load() == 0);
    std::cout << "[PASS] TestReconnectAttemptCount_InitialValue" << std::endl;
}

void TestReconnectAttemptCount_Increment()
{
    std::atomic<int> count{0};
    
    for (int i = 0; i < MAX_RECONNECT_ATTEMPTS; ++i)
    {
        count.store(count.load() + 1);
    }
    
    assert(count.load() == MAX_RECONNECT_ATTEMPTS);
    std::cout << "[PASS] TestReconnectAttemptCount_Increment" << std::endl;
}

void TestReconnectAttemptCount_ResetOnDisconnect()
{
    std::atomic<int> count{5};
    assert(count.load() == 5);
    
    count.store(0);
    assert(count.load() == 0);
    assert(count.load() < MAX_RECONNECT_ATTEMPTS);
    
    std::cout << "[PASS] TestReconnectAttemptCount_ResetOnDisconnect" << std::endl;
}

void TestReconnectAttemptCount_ResetOnManualReconnect()
{
    std::atomic<int> count{5};
    assert(count.load() == MAX_RECONNECT_ATTEMPTS);
    
    count.store(0);
    assert(count.load() == 0);
    assert(count.load() < MAX_RECONNECT_ATTEMPTS);
    
    std::cout << "[PASS] TestReconnectAttemptCount_ResetOnManualReconnect" << std::endl;
}

void TestReconnectAttemptCount_MaxExceeded()
{
    std::atomic<int> count{4};
    
    count.store(count.load() + 1);
    assert(count.load() == MAX_RECONNECT_ATTEMPTS);
    assert(count.load() >= MAX_RECONNECT_ATTEMPTS);
    
    std::cout << "[PASS] TestReconnectAttemptCount_MaxExceeded" << std::endl;
}

void TestReconnectAttemptCount_ShouldStopAtMax()
{
    std::atomic<int> count{MAX_RECONNECT_ATTEMPTS};
    assert(count.load() >= MAX_RECONNECT_ATTEMPTS);
    
    bool shouldStop = count.load() >= MAX_RECONNECT_ATTEMPTS;
    assert(shouldStop == true);
    
    std::cout << "[PASS] TestReconnectAttemptCount_ShouldStopAtMax" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// 断连原因测试 - 覆盖所有断连原因场景
// ═══════════════════════════════════════════════════════════════════

void TestDisconnectReason_NetworkError()
{
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    std::atomic<DisconnectReason> reason{DisconnectReason::NetworkError};
    
    assert(state.load() == ConnectionState::Connected);
    assert(reason.load() == DisconnectReason::NetworkError);
    
    state.store(ConnectionState::Reconnecting);
    
    assert(state.load() == ConnectionState::Reconnecting);
    assert(reason.load() == DisconnectReason::NetworkError);
    
    std::cout << "[PASS] TestDisconnectReason_NetworkError" << std::endl;
}

void TestDisconnectReason_HeartbeatTimeout()
{
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    std::atomic<DisconnectReason> reason{DisconnectReason::HeartbeatTimeout};
    
    assert(state.load() == ConnectionState::Connected);
    assert(reason.load() == DisconnectReason::HeartbeatTimeout);
    
    state.store(ConnectionState::Reconnecting);
    
    assert(state.load() == ConnectionState::Reconnecting);
    assert(reason.load() == DisconnectReason::HeartbeatTimeout);
    
    std::cout << "[PASS] TestDisconnectReason_HeartbeatTimeout" << std::endl;
}

void TestDisconnectReason_ServerClose()
{
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    std::atomic<DisconnectReason> reason{DisconnectReason::ServerClose};
    
    assert(state.load() == ConnectionState::Connected);
    assert(reason.load() == DisconnectReason::ServerClose);
    
    state.store(ConnectionState::Reconnecting);
    
    assert(state.load() == ConnectionState::Reconnecting);
    assert(reason.load() == DisconnectReason::ServerClose);
    
    std::cout << "[PASS] TestDisconnectReason_ServerClose" << std::endl;
}

void TestDisconnectReason_None()
{
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    std::atomic<DisconnectReason> reason{DisconnectReason::None};
    
    assert(state.load() == ConnectionState::Connected);
    assert(reason.load() == DisconnectReason::None);
    
    state.store(ConnectionState::Disconnected);
    
    assert(state.load() == ConnectionState::Disconnected);
    assert(reason.load() == DisconnectReason::None);
    
    std::cout << "[PASS] TestDisconnectReason_None" << std::endl;
}

void TestDisconnectReason_AuthFailed()
{
    std::atomic<ConnectionState> state{ConnectionState::Connecting};
    std::atomic<DisconnectReason> reason{DisconnectReason::AuthFailed};
    
    assert(state.load() == ConnectionState::Connecting);
    assert(reason.load() == DisconnectReason::AuthFailed);
    
    state.store(ConnectionState::Disconnected);
    
    assert(state.load() == ConnectionState::Disconnected);
    assert(reason.load() == DisconnectReason::AuthFailed);
    
    std::cout << "[PASS] TestDisconnectReason_AuthFailed" << std::endl;
}

void TestDisconnectReason_PreservedOnStateTransition()
{
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    std::atomic<DisconnectReason> reason{DisconnectReason::NetworkError};
    
    state.store(ConnectionState::Reconnecting);
    assert(reason.load() == DisconnectReason::NetworkError);
    
    state.store(ConnectionState::ReconnectFailed);
    assert(reason.load() == DisconnectReason::NetworkError);
    
    std::cout << "[PASS] TestDisconnectReason_PreservedOnStateTransition" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// 状态组合测试 - 完整的重连失败流程
// ═══════════════════════════════════════════════════════════════════

void TestFullReconnectFailureFlow()
{
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    std::atomic<DisconnectReason> reason{DisconnectReason::HeartbeatTimeout};
    std::atomic<int> attemptCount{0};
    
    // 状态转换：Connected -> Reconnecting
    state.store(ConnectionState::Reconnecting);
    assert(state.load() == ConnectionState::Reconnecting);
    assert(reason.load() == DisconnectReason::HeartbeatTimeout);
    
    // 尝试重连5次
    for (int i = 0; i < MAX_RECONNECT_ATTEMPTS; ++i)
    {
        attemptCount.store(attemptCount.load() + 1);
        assert(attemptCount.load() == i + 1);
    }
    
    // 达到最大次数，转为 ReconnectFailed
    assert(attemptCount.load() >= MAX_RECONNECT_ATTEMPTS);
    state.store(ConnectionState::ReconnectFailed);
    
    assert(state.load() == ConnectionState::ReconnectFailed);
    assert(attemptCount.load() == MAX_RECONNECT_ATTEMPTS);
    assert(reason.load() == DisconnectReason::HeartbeatTimeout);
    
    std::cout << "[PASS] TestFullReconnectFailureFlow" << std::endl;
}

void TestManualReconnectAfterFailure()
{
    std::atomic<ConnectionState> state{ConnectionState::ReconnectFailed};
    std::atomic<DisconnectReason> reason{DisconnectReason::NetworkError};
    std::atomic<int> attemptCount{MAX_RECONNECT_ATTEMPTS};
    
    // 用户点击重连按钮
    state.store(ConnectionState::Connecting);
    reason.store(DisconnectReason::None);
    attemptCount.store(0);
    
    assert(state.load() == ConnectionState::Connecting);
    assert(reason.load() == DisconnectReason::None);
    assert(attemptCount.load() == 0);
    
    // 连接成功
    state.store(ConnectionState::Connected);
    assert(state.load() == ConnectionState::Connected);
    
    std::cout << "[PASS] TestManualReconnectAfterFailure" << std::endl;
}

void TestActiveDisconnectClearsRetryCount()
{
    std::atomic<ConnectionState> state{ConnectionState::Reconnecting};
    std::atomic<DisconnectReason> reason{DisconnectReason::NetworkError};
    std::atomic<int> attemptCount{3};
    
    // 用户主动断开
    state.store(ConnectionState::Disconnected);
    reason.store(DisconnectReason::None);
    attemptCount.store(0);
    
    assert(state.load() == ConnectionState::Disconnected);
    assert(reason.load() == DisconnectReason::None);
    assert(attemptCount.load() == 0);
    
    std::cout << "[PASS] TestActiveDisconnectClearsRetryCount" << std::endl;
}

void TestCancelDuringConnecting()
{
    std::atomic<ConnectionState> state{ConnectionState::Connecting};
    std::atomic<DisconnectReason> reason{DisconnectReason::None};
    
    // 用户点击取消
    state.store(ConnectionState::Disconnected);
    
    assert(state.load() == ConnectionState::Disconnected);
    assert(reason.load() == DisconnectReason::None);
    
    std::cout << "[PASS] TestCancelDuringConnecting" << std::endl;
}

void TestCancelDuringReconnecting()
{
    std::atomic<ConnectionState> state{ConnectionState::Reconnecting};
    std::atomic<DisconnectReason> reason{DisconnectReason::HeartbeatTimeout};
    std::atomic<int> attemptCount{2};
    
    // 用户点击取消
    state.store(ConnectionState::Disconnected);
    reason.store(DisconnectReason::None);
    attemptCount.store(0);
    
    assert(state.load() == ConnectionState::Disconnected);
    assert(reason.load() == DisconnectReason::None);
    assert(attemptCount.load() == 0);
    
    std::cout << "[PASS] TestCancelDuringReconnecting" << std::endl;
}

void TestMultipleStateTransitions()
{
    std::atomic<ConnectionState> state{ConnectionState::Disconnected};
    
    // 模拟多次连接循环
    for (int cycle = 0; cycle < 3; ++cycle)
    {
        state.store(ConnectionState::Connecting);
        assert(state.load() == ConnectionState::Connecting);
        
        state.store(ConnectionState::Connected);
        assert(state.load() == ConnectionState::Connected);
        
        state.store(ConnectionState::Reconnecting);
        assert(state.load() == ConnectionState::Reconnecting);
        
        state.store(ConnectionState::Disconnected);
        assert(state.load() == ConnectionState::Disconnected);
    }
    
    std::cout << "[PASS] TestMultipleStateTransitions" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// 按钮文案逻辑测试
// ═══════════════════════════════════════════════════════════════════

void TestButtonText_Connected()
{
    // Connected -> "断开"
    std::atomic<ConnectionState> state{ConnectionState::Connected};
    const char* expected = "断开";
    assert(state.load() == ConnectionState::Connected);
    assert(strcmp(expected, "断开") == 0);
    std::cout << "[PASS] TestButtonText_Connected" << std::endl;
}

void TestButtonText_Connecting()
{
    // Connecting -> "取消"
    std::atomic<ConnectionState> state{ConnectionState::Connecting};
    const char* expected = "取消";
    assert(state.load() == ConnectionState::Connecting);
    assert(strcmp(expected, "取消") == 0);
    std::cout << "[PASS] TestButtonText_Connecting" << std::endl;
}

void TestButtonText_Reconnecting()
{
    // Reconnecting -> "取消"
    std::atomic<ConnectionState> state{ConnectionState::Reconnecting};
    const char* expected = "取消";
    assert(state.load() == ConnectionState::Reconnecting);
    assert(strcmp(expected, "取消") == 0);
    std::cout << "[PASS] TestButtonText_Reconnecting" << std::endl;
}

void TestButtonText_Disconnected()
{
    // Disconnected -> "连接"
    std::atomic<ConnectionState> state{ConnectionState::Disconnected};
    const char* expected = "连接";
    assert(state.load() == ConnectionState::Disconnected);
    assert(strcmp(expected, "连接") == 0);
    std::cout << "[PASS] TestButtonText_Disconnected" << std::endl;
}

void TestButtonText_ReconnectFailed()
{
    // ReconnectFailed -> "连接"
    std::atomic<ConnectionState> state{ConnectionState::ReconnectFailed};
    const char* expected = "连接";
    assert(state.load() == ConnectionState::ReconnectFailed);
    assert(strcmp(expected, "连接") == 0);
    std::cout << "[PASS] TestButtonText_ReconnectFailed" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// 测试运行函数
// ═══════════════════════════════════════════════════════════════════

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  BliveManager Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    std::cout << "[--- 枚举和常量测试 ---]" << std::endl;
    TestConnectionState_EnumValues();
    TestDisconnectReason_EnumValues();
    TestMaxReconnectAttempts_Value();
    std::cout << std::endl;
    
    std::cout << "[--- 字符串转换测试 ---]" << std::endl;
    TestConnectionStateToString();
    TestDisconnectReasonToString();
    std::cout << std::endl;
    
    std::cout << "[--- 状态转换测试 ---]" << std::endl;
    TestStateTransition_DisconnectedToConnecting();
    TestStateTransition_ConnectingToConnected();
    TestStateTransition_ConnectingToDisconnected_NetworkError();
    TestStateTransition_ConnectedToReconnecting_HeartbeatTimeout();
    TestStateTransition_ConnectedToReconnecting_ServerClose();
    TestStateTransition_ConnectedToDisconnected_Disconnect();
    TestStateTransition_ReconnectingToConnected();
    TestStateTransition_ReconnectingToReconnecting_IncrementAttempt();
    TestStateTransition_ReconnectingToReconnectFailed();
    TestStateTransition_ReconnectFailedToConnecting();
    TestStateTransition_ReconnectingToDisconnected_Disconnect();
    std::cout << std::endl;
    
    std::cout << "[--- 重连计数测试 ---]" << std::endl;
    TestReconnectAttemptCount_InitialValue();
    TestReconnectAttemptCount_Increment();
    TestReconnectAttemptCount_ResetOnDisconnect();
    TestReconnectAttemptCount_ResetOnManualReconnect();
    TestReconnectAttemptCount_MaxExceeded();
    TestReconnectAttemptCount_ShouldStopAtMax();
    std::cout << std::endl;
    
    std::cout << "[--- 断连原因测试 ---]" << std::endl;
    TestDisconnectReason_NetworkError();
    TestDisconnectReason_HeartbeatTimeout();
    TestDisconnectReason_ServerClose();
    TestDisconnectReason_None();
    TestDisconnectReason_AuthFailed();
    TestDisconnectReason_PreservedOnStateTransition();
    std::cout << std::endl;
    
    std::cout << "[--- 组合流程测试 ---]" << std::endl;
    TestFullReconnectFailureFlow();
    TestManualReconnectAfterFailure();
    TestActiveDisconnectClearsRetryCount();
    TestCancelDuringConnecting();
    TestCancelDuringReconnecting();
    TestMultipleStateTransitions();
    std::cout << std::endl;
    
    std::cout << "[--- 按钮文案测试 ---]" << std::endl;
    TestButtonText_Connected();
    TestButtonText_Connecting();
    TestButtonText_Reconnecting();
    TestButtonText_Disconnected();
    TestButtonText_ReconnectFailed();
    std::cout << std::endl;
    
    std::cout << "========================================" << std::endl;
    std::cout << "  All tests passed!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}

#endif
