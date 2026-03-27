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
}

bool AudioPlayer::Play(const std::vector<uint8_t>& audioData, const std::string& format)
{
    lock.lock();
    if (playing) {
        Stop();
    }
    lock.unlock();

    if (audioData.empty()) {
        lastError = "Audio data is empty";
        LOG_ERROR(TEXT("AudioPlayer: %s"), utf8_to_wstring(lastError).c_str());
        return false;
    }

    std::wstring filePath = WriteToTempFile(audioData, format);
    if (filePath.empty()) {
        return false;
    }

    return PlayFile(filePath);
}

bool AudioPlayer::PlayFile(const std::wstring& filePath)
{
    lock.lock();
    if (playing) {
        Stop();
    }
    lock.unlock();

    if (!std::filesystem::exists(filePath)) {
        lastError = "Audio file not found";
        LOG_ERROR(TEXT("AudioPlayer: %s"), utf8_to_wstring(lastError).c_str());
        return false;
    }

    tempFilePath = filePath;

    ExecuteMCICommand(L"close mimo_audio_alias");

    std::wstring openCmd = L"open \"" + filePath + L"\" type mpegvideo alias mimo_audio_alias";
    if (!ExecuteMCICommand(openCmd)) {
        openCmd = L"open \"" + filePath + L"\" alias mimo_audio_alias";
        if (!ExecuteMCICommand(openCmd)) {
            lastError = "Failed to open audio file";
            return false;
        }
    }

    if (!ExecuteMCICommand(L"play mimo_audio_alias")) {
        ExecuteMCICommand(L"close mimo_audio_alias");
        lastError = "Failed to play audio";
        return false;
    }

    lock.lock();
    playing = true;
    lock.unlock();

    LOG_INFO(TEXT("AudioPlayer: Started playing audio"));
    return true;
}

void AudioPlayer::Stop()
{
    lock.lock();
    if (playing) {
        ExecuteMCICommand(L"stop mimo_audio_alias");
        ExecuteMCICommand(L"close mimo_audio_alias");
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

std::wstring AudioPlayer::WriteToTempFile(const std::vector<uint8_t>& audioData, const std::string& format)
{
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);

    std::wstring extension = L"." + utf8_to_wstring(format);
    std::wstring filePath = std::wstring(tempPath) + L"mimo_tts_" + std::to_wstring(GetTickCount64()) + extension;

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
    return filePath;
}

void AudioPlayer::CleanupTempFile()
{
    if (!tempFilePath.empty()) {
        DeleteFileW(tempFilePath.c_str());
        tempFilePath.clear();
    }
}

bool AudioPlayer::ExecuteMCICommand(const std::wstring& command)
{
    MCIERROR err = mciSendStringW(command.c_str(), NULL, 0, NULL);
    return CheckMCIError(err);
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
