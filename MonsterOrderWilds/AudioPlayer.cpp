#include "AudioPlayer.h"
#include "WriteLog.h"
#include "StringUtils.h"
#include <mmsystem.h>
#include <filesystem>

#pragma comment(lib, "winmm.lib")

AudioPlayer::AudioPlayer()
    : playing(false)
{
}

AudioPlayer::~AudioPlayer()
{
    Stop();
    CleanupTempFile();

    // Cleanup entire TempAudio directory
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    size_t pos = exeDir.rfind(L'\\');
    if (pos != std::wstring::npos) {
        exeDir = exeDir.substr(0, pos);
    }
    std::wstring tempDir = exeDir + L"\\TempAudio";

    // Delete all files in TempAudio
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((tempDir + L"\\*.*").c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring filePath = tempDir + L"\\" + findData.cFileName;
                DeleteFileW(filePath.c_str());
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    // Remove directory
    RemoveDirectoryW(tempDir.c_str());
}

bool AudioPlayer::Play(const std::vector<uint8_t>& audioData, const std::string& format)
{
    bool wasPlaying = false;
    std::wstring prevFilePath;
    {
        lock.lock();
        wasPlaying = playing;
        if (playing) {
            playing = false;
        }
        lock.unlock();
    }
    
    if (wasPlaying) {
        ExecuteMCICommand(L"stop mimo_audio_alias");
        ExecuteMCICommand(L"close mimo_audio_alias");
        prevFilePath = tempFilePath;
    }

    if (audioData.empty()) {
        lastError = "Audio data is empty";
        LOG_ERROR(TEXT("AudioPlayer: %s"), utf8_to_wstring(lastError).c_str());
        return false;
    }

    LOG_INFO(TEXT("AudioPlayer: Writing %d bytes to temp file"), (int)audioData.size());
    std::wstring filePath = WriteToTempFile(audioData, format);
    if (filePath.empty()) {
        LOG_ERROR(TEXT("AudioPlayer: WriteToTempFile failed"));
        return false;
    }

    tempFilePath = filePath;

    bool playResult = PlayFile(filePath);
    if (!playResult) {
        tempFilePath.clear();
        return false;
    }

    if (!prevFilePath.empty()) {
        DeleteFileW(prevFilePath.c_str());
        LOG_INFO(TEXT("AudioPlayer: Deleted previous temp file"));
    }

    LOG_INFO(TEXT("AudioPlayer: Playing file: %s"), filePath.c_str());
    return true;
}

bool AudioPlayer::PlayFile(const std::wstring& filePath)
{
    lock.lock();
    if (playing) {
        ExecuteMCICommand(L"stop mimo_audio_alias");
        ExecuteMCICommand(L"close mimo_audio_alias");
        playing = false;
    }
    lock.unlock();

    if (!std::filesystem::exists(filePath)) {
        lastError = "Audio file not found";
        LOG_ERROR(TEXT("AudioPlayer: %s"), utf8_to_wstring(lastError).c_str());
        return false;
    }

    tempFilePath = filePath;

    ExecuteMCICommand(L"close mimo_audio_alias", true);

    bool openSucceeded = false;
    std::wstring openCmd = L"open \"" + filePath + L"\" type mpegvideo alias mimo_audio_alias";
    MCIERROR openErr = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
    if (openErr != 0) {
        wchar_t errorMsg[256];
        mciGetErrorStringW(openErr, errorMsg, 256);
        std::wstring errStr = errorMsg;
        if (errStr.find(L"指定的设备未打开") != std::wstring::npos ||
            errStr.find(L"不被 MCI 所识别") != std::wstring::npos) {
            LOG_WARNING(TEXT("AudioPlayer: Open failed with MCI device error, treating as completed"));
            return true;
        }
        openCmd = L"open \"" + filePath + L"\" alias mimo_audio_alias";
        openErr = mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
        if (openErr != 0) {
            mciGetErrorStringW(openErr, errorMsg, 256);
            errStr = errorMsg;
            if (errStr.find(L"指定的设备未打开") != std::wstring::npos ||
                errStr.find(L"不被 MCI 所识别") != std::wstring::npos) {
                LOG_WARNING(TEXT("AudioPlayer: Open failed with MCI device error, treating as completed"));
                return true;
            }
        } else {
            openSucceeded = true;
        }
    } else {
        openSucceeded = true;
    }

    if (!openSucceeded) {
        LOG_WARNING(TEXT("AudioPlayer: Failed to open audio file, treating as completed"));
        return true;
    }

    MCIERROR playErr = mciSendStringW(L"play mimo_audio_alias", NULL, 0, NULL);
    if (playErr != 0) {
        wchar_t errorMsg[256];
        mciGetErrorStringW(playErr, errorMsg, 256);
        std::wstring errStr = errorMsg;
        
        if (errStr.find(L"指定的设备未打开") != std::wstring::npos ||
            errStr.find(L"不被 MCI 所识别") != std::wstring::npos) {
            LOG_WARNING(TEXT("AudioPlayer: Play failed with MCI device error, treating as completed"));
            ExecuteMCICommand(L"close mimo_audio_alias", true);
            return true;
        }
        
        ExecuteMCICommand(L"close mimo_audio_alias", true);
        lastError = "Failed to play audio";
        lock.lock();
        playing = false;
        lock.unlock();
        return false;
    }

    lock.lock();
    playing = true;
    lock.unlock();

    // 设置音量 (配置范围 0-200, MCI 音量范围 0-1000)
    int mciVolume = volume_ * 5;
    std::wstring volumeCmd = L"setaudio mimo_audio_alias volume to " + std::to_wstring(mciVolume);
    ExecuteMCICommand(volumeCmd, true);
    LOG_DEBUG(TEXT("AudioPlayer: Set volume to %d (MCI: %d)"), volume_, mciVolume);

    LOG_INFO(TEXT("AudioPlayer: Started playing audio"));
    return true;
}

void AudioPlayer::Stop()
{
    lock.lock();
    if (playing) {
        ExecuteMCICommand(L"stop mimo_audio_alias", true);
        ExecuteMCICommand(L"close mimo_audio_alias", true);
        playing = false;
        LOG_INFO(TEXT("AudioPlayer: Stopped playing audio"));
    }
    lock.unlock();
}

bool AudioPlayer::IsPlaying() const
{
    lock.lock();
    bool result = playing;
    lock.unlock();
    return result;
}

bool AudioPlayer::WaitForCompletion(DWORD timeoutMs)
{
    DWORD startTime = GetTickCount();
    int checkInterval = 50;  // Start with 50ms
    
    while (IsPlaying()) {
        wchar_t status[256] = {0};
        MCIERROR err = mciSendStringW(L"status mimo_audio_alias mode", status, 256, NULL);
        if (err == 0) {
            std::wstring mode = status;
            if (mode == L"stopped") {
                lock.lock();
                playing = false;
                lock.unlock();
                ExecuteMCICommand(L"close mimo_audio_alias");
                LOG_INFO(TEXT("AudioPlayer: Playback completed"));
                return true;
            }
            // Adaptive interval: check more frequently when playing
            checkInterval = (mode == L"playing") ? 100 : 50;
        }

        if (timeoutMs > 0) {
            DWORD elapsed = GetTickCount() - startTime;
            if (elapsed >= timeoutMs) {
                LOG_ERROR(TEXT("AudioPlayer: Playback timeout"));
                return false;
            }
            // Increase interval as we approach timeout (assume nearly done)
            if (elapsed > timeoutMs * 0.8) {
                checkInterval = 25;
            }
        }

        Sleep(checkInterval);
    }

    return true;
}

std::string AudioPlayer::GetLastError() const
{
    return lastError;
}

bool AudioPlayer::IsPlaybackComplete() const
{
    lock.lock();
    bool isPlaying = playing;
    lock.unlock();

    if (!isPlaying) {
        LOG_INFO(TEXT("AudioPlayer: IsPlaybackComplete=true (not playing)"));
        return true;  // Not playing = complete (or never started)
    }

    // Check MCI status
    wchar_t status[256] = {0};
    MCIERROR err = mciSendStringW(L"status mimo_audio_alias mode", status, 256, NULL);
    if (err == 0) {
        std::wstring mode = status;
        if (mode == L"stopped") {
            lock.lock();
            playing = false;
            lock.unlock();
            return true;
        }
        return false;
    }
    
    // MCI query failed, check if we've started playing before
    // If we have played before (tempFilePath exists), assume completed
    LOG_INFO(TEXT("AudioPlayer: MCI query failed (err=%d), tempFilePath=%s, returning %s"), 
        (int)err, tempFilePath.empty() ? TEXT("empty") : TEXT("exists"), 
        !tempFilePath.empty() ? TEXT("true") : TEXT("false"));
    return !tempFilePath.empty();
}

std::wstring AudioPlayer::WriteToTempFile(const std::vector<uint8_t>& audioData, const std::string& format)
{
    // Get exe directory
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir = exePath;
    size_t pos = exeDir.rfind(L'\\');
    if (pos != std::wstring::npos) {
        exeDir = exeDir.substr(0, pos);
    }

    // Create TempAudio subdirectory
    std::wstring tempDir = exeDir + L"\\TempAudio";
    if (!CreateDirectoryW(tempDir.c_str(), NULL)) {
        DWORD dirError = ::GetLastError();
        if (dirError != ERROR_ALREADY_EXISTS) {
            lastError = "Failed to create TempAudio directory";
            LOG_ERROR(TEXT("AudioPlayer: %s"), utf8_to_wstring(lastError).c_str());
            return L"";
        }
    }

    std::wstring extension = L"." + utf8_to_wstring(format);
    std::wstring filePath = tempDir + L"\\mimo_tts_" + std::to_wstring(GetTickCount64()) + extension;

    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        lastError = "Failed to create temp file";
        LOG_ERROR(TEXT("AudioPlayer: %s"), utf8_to_wstring(lastError).c_str());
        return L"";
    }

    DWORD bytesWritten;
    if (!WriteFile(hFile, audioData.data(), static_cast<DWORD>(audioData.size()), &bytesWritten, NULL)) {
        CloseHandle(hFile);
        lastError = "Failed to write audio data";
        LOG_ERROR(TEXT("AudioPlayer: %s"), utf8_to_wstring(lastError).c_str());
        return L"";
    }

    CloseHandle(hFile);
    LOG_INFO(TEXT("AudioPlayer: Temp file saved to: %s"), filePath.c_str());
    return filePath;
}

void AudioPlayer::SetVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 200) volume = 200;
    volume_ = volume;
    LOG_DEBUG(TEXT("AudioPlayer: Volume set to %d"), volume_);
}

void AudioPlayer::CleanupTempFile()
{
    if (!tempFilePath.empty()) {
        DeleteFileW(tempFilePath.c_str());
        tempFilePath.clear();
    }
}

bool AudioPlayer::ExecuteMCICommand(const std::wstring& command, bool ignoreError)
{
    MCIERROR err = mciSendStringW(command.c_str(), NULL, 0, NULL);
    if (err == 0) {
        return true;
    }
    wchar_t errorMsg[256];
    mciGetErrorStringW(err, errorMsg, 256);
    std::wstring errStr = errorMsg;
    if (ignoreError && (errStr.find(L"指定的设备未打开") != std::wstring::npos ||
        errStr.find(L"不被 MCI 所识别") != std::wstring::npos)) {
        LOG_WARNING(TEXT("AudioPlayer MCI: Ignored error: %s"), errorMsg);
        return true;
    }
    lastError = wstring_to_utf8(errorMsg);
    LOG_ERROR(TEXT("AudioPlayer MCI Error: %s"), errorMsg);
    return false;
}

bool AudioPlayer::CheckMCIError(DWORD error)
{
    if (error != 0) {
        wchar_t errorMsg[256];
        mciGetErrorStringW(error, errorMsg, 256);
        lastError = wstring_to_utf8(errorMsg);
        LOG_ERROR(TEXT("AudioPlayer MCI Error: %s"), errorMsg);
        return false;
    }
    return true;
}
