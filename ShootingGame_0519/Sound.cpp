#include "Sound.h"
#include <fstream>
#include <algorithm>

Microsoft::WRL::ComPtr<IXAudio2> Sound::m_XAudio2;
IXAudio2MasteringVoice* Sound::m_MasterVoice = nullptr;
IXAudio2SourceVoice* Sound::m_BgmVoice = nullptr;
float Sound::m_BgmVolume = 0.7f;
Sound::WavData Sound::m_BgmData;

static uint32_t ReadU32(std::ifstream& ifs)
{
    uint32_t v = 0;
    ifs.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

static uint16_t ReadU16(std::ifstream& ifs)
{
    uint16_t v = 0;
    ifs.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

bool Sound::Init()
{
    HRESULT hr = XAudio2Create(m_XAudio2.ReleaseAndGetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        return false;
    }

    hr = m_XAudio2->CreateMasteringVoice(&m_MasterVoice);
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

void Sound::Uninit()
{
    StopBgm();

    if (m_MasterVoice)
    {
        m_MasterVoice->DestroyVoice();
        m_MasterVoice = nullptr;
    }

    m_XAudio2.Reset();
}

bool Sound::LoadWavPcm(const std::wstring& filepath, WavData& outData)
{
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs.is_open())
    {
        return false;
    }

    char riff[4]{};
    ifs.read(riff, 4);
    if (std::memcmp(riff, "RIFF", 4) != 0)
    {
        return false;
    }

    (void)ReadU32(ifs); // file size

    char wave[4]{};
    ifs.read(wave, 4);
    if (std::memcmp(wave, "WAVE", 4) != 0)
    {
        return false;
    }

    bool foundFmt = false;
    bool foundData = false;

    WAVEFORMATEX fmt{};
    std::vector<uint8_t> data;

    while (ifs && (!foundFmt || !foundData))
    {
        char chunkId[4]{};
        ifs.read(chunkId, 4);
        if (!ifs)
        {
            break;
        }

        uint32_t chunkSize = ReadU32(ifs);

        if (std::memcmp(chunkId, "fmt ", 4) == 0)
        {
            uint16_t audioFormat = ReadU16(ifs);
            uint16_t numChannels = ReadU16(ifs);
            uint32_t sampleRate = ReadU32(ifs);
            uint32_t byteRate = ReadU32(ifs);
            uint16_t blockAlign = ReadU16(ifs);
            uint16_t bitsPerSample = ReadU16(ifs);

            if (chunkSize > 16)
            {
                ifs.seekg(chunkSize - 16, std::ios::cur);
            }

            if (audioFormat != 1)
            {
                // PCMà»äOÇÕÇ±ÇÃç≈è¨é¿ëïÇ≈ÇÕîÒëŒâû
                return false;
            }

            fmt.wFormatTag = WAVE_FORMAT_PCM;
            fmt.nChannels = numChannels;
            fmt.nSamplesPerSec = sampleRate;
            fmt.nAvgBytesPerSec = byteRate;
            fmt.nBlockAlign = blockAlign;
            fmt.wBitsPerSample = bitsPerSample;
            fmt.cbSize = 0;

            foundFmt = true;
        }
        else if (std::memcmp(chunkId, "data", 4) == 0)
        {
            data.resize(chunkSize);
            ifs.read(reinterpret_cast<char*>(data.data()), chunkSize);
            foundData = true;
        }
        else
        {
            ifs.seekg(chunkSize, std::ios::cur);
        }
    }

    if (!foundFmt || !foundData)
    {
        return false;
    }

    outData.format = fmt;
    outData.buffer = std::move(data);
    return true;
}

bool Sound::PlayBgmWav(const std::wstring& filepath, float volume)
{
    if (!m_XAudio2)
    {
        return false;
    }

    StopBgm();

    WavData wav{};
    if (!LoadWavPcm(filepath, wav))
    {
        return false;
    }

    m_BgmData = std::move(wav);

    HRESULT hr = m_XAudio2->CreateSourceVoice(&m_BgmVoice, &m_BgmData.format);
    if (FAILED(hr))
    {
        m_BgmVoice = nullptr;
        return false;
    }

    XAUDIO2_BUFFER buf{};
    buf.AudioBytes = static_cast<UINT32>(m_BgmData.buffer.size());
    buf.pAudioData = m_BgmData.buffer.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;
    buf.LoopCount = XAUDIO2_LOOP_INFINITE;

    hr = m_BgmVoice->SubmitSourceBuffer(&buf);
    if (FAILED(hr))
    {
        StopBgm();
        return false;
    }

    m_BgmVolume = std::clamp(volume, 0.0f, 1.0f);
    m_BgmVoice->SetVolume(m_BgmVolume);

    hr = m_BgmVoice->Start();
    if (FAILED(hr))
    {
        StopBgm();
        return false;
    }

    return true;
}

void Sound::StopBgm()
{
    if (m_BgmVoice)
    {
        m_BgmVoice->Stop();
        m_BgmVoice->FlushSourceBuffers();
        m_BgmVoice->DestroyVoice();
        m_BgmVoice = nullptr;
    }

    m_BgmData.buffer.clear();
}

void Sound::SetBgmVolume(float volume)
{
    m_BgmVolume = std::clamp(volume, 0.0f, 1.0f);
    if (m_BgmVoice)
    {
        m_BgmVoice->SetVolume(m_BgmVolume);
    }
}

float Sound::GetBgmVolume()
{
    return m_BgmVolume;
}
