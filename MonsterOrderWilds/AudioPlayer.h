#pragma once
#include "framework.h"
#include <vector>

// 音频播放器
// 使用Windows多媒体API（mciSendString）播放音频文件
class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    // 播放音频数据
    bool Play(const std::vector<uint8_t>& audioData, const std::string& format = "mp3");

    // 播放音频文件
    bool PlayFile(const std::wstring& filePath);

    // 停止当前播放
    void Stop();

    // 检查是否正在播放
    bool IsPlaying() const;

    // 等待播放完成
    bool WaitForCompletion(DWORD timeoutMs = 0);

    // 获取最后一次错误信息
    std::string GetLastError() const;

private:
    std::wstring WriteToTempFile(const std::vector<uint8_t>& audioData, const std::string& format);
    void CleanupTempFile();
    bool ExecuteMCICommand(const std::wstring& command);
    bool CheckMCIError(DWORD error);

    std::wstring tempFilePath;
    std::string lastError;
    bool playing;
    mutable Lock lock;
};
