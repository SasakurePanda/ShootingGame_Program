#pragma once
#include <wrl/client.h>
#include <xaudio2.h>
#include <cstdint>
#include <string>
#include <vector>

class Sound
{
public:
    static bool Init();
    static void Uninit();

    // ループBGM用：開始
    static bool PlayBgmWav(const std::wstring& filepath, float volume = 0.7f);

    // 停止
    static void StopBgm();

    //--------Set関数-------
    static void SetBgmVolume(float volume);

    //--------Get関数-------
    static float GetBgmVolume();

private:
    struct WavData
    {
        WAVEFORMATEX format{};
        std::vector<uint8_t> buffer;
    };

    static bool LoadWavPcm(const std::wstring& filepath, WavData& outData);

    static Microsoft::WRL::ComPtr<IXAudio2> m_XAudio2;
    static IXAudio2MasteringVoice* m_MasterVoice;
    static IXAudio2SourceVoice* m_BgmVoice;

    static float m_BgmVolume;

    static WavData m_BgmData; 
};

