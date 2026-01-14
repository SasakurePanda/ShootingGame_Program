#pragma once
#include <wrl/client.h>
#include <xaudio2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

class Sound
{
public:
    static bool Init();
    static void Update(float dt);
    static void Uninit();

    //--------BGM関連-------
    static bool PlayBgmWav(const std::wstring& filepath, float volume = 0.7f);
    static void StopBgm();
    static void FadeInBgm(float targetVolume, float durationSec);
    static void FadeOutBgm(float durationSec);

    //--------SE関連-------
    static bool PlaySeWav(const std::wstring& filepath, float volume = 1.0f);
    static void StopAllSe();
    static void ClearSeCache();

    //--------Set関数-------
    static void SetBgmVolume(float volume);
    static void SetSeVolume(float volume);

    //--------Get関数-------
    static float GetBgmVolume();
    static float GetSeVolume();

private:
    struct WavData
    {
        WAVEFORMATEX format{};
        std::vector<uint8_t> buffer;
    };

    struct SeVoiceEntry
    {
        IXAudio2SourceVoice* voice = nullptr;
        const WavData* wav = nullptr; // キャッシュ参照（バッファ寿命確保）
    };

    static bool LoadWavPcm(const std::wstring& filepath, WavData& outData);
    static const WavData* GetOrLoadSeWav(const std::wstring& filepath);

    static Microsoft::WRL::ComPtr<IXAudio2> m_XAudio2;
    static IXAudio2MasteringVoice* m_MasterVoice;

    //--------------BGM関連------------------
    static IXAudio2SourceVoice* m_BgmVoice;
    static float m_BgmVolume;
    static WavData m_BgmData;

    //--------------BGMフェード関連------------------
    static bool m_IsFading;
    static bool m_FadeIn;
    static float m_FadeTimer;
    static float m_FadeDuration;
    static float m_FadeStartVolume;
    static float m_FadeTargetVolume;

    //--------------SE関連------------------
    static float m_SeVolume;
    static std::unordered_map<std::wstring, WavData> m_SeCache;
    static std::vector<SeVoiceEntry> m_SeVoices;
};

