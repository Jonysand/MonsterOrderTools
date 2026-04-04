# AudioPlayer - 音频播放

## 概述

使用 Windows 多媒体 API (MCI) 播放 MP3 音频文件和数据流。

## 核心 API

```cpp
class AudioPlayer {
    AudioPlayer();
    ~AudioPlayer();
    
    bool Play(const std::vector<uint8_t>& audioData, const std::string& format = "mp3");
    bool PlayFile(const std::wstring& filePath);
    void Stop();
    bool IsPlaying() const;
    bool WaitForCompletion(DWORD timeoutMs = 0);
    bool IsPlaybackComplete() const;
    std::string GetLastError() const;
};
```

## 实现细节

### MCI 命令
- `open`: 打开音频设备
- `play`: 开始播放
- `stop`: 停止播放
- `close`: 关闭设备
- `status`: 查询播放状态

### 临时文件
- 内存音频数据先写入临时文件再播放
- 播放完成后自动清理临时文件

### 线程安全
- 使用 `Lock` 保护内部状态
- 所有公共方法线程安全

## 文件结构

- `AudioPlayer.h`: 接口声明
- `AudioPlayer.cpp`: 实现

## 依赖

- Windows API: `mciSendString`, `mciGetErrorString`
