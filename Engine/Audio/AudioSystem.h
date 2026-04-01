#pragma once

#include "Core/Types.h"
#include <string>
#include <unordered_map>

namespace Gini {

using SoundID = u32;
using MusicID = u32;

class AudioSystem {
public:
    static AudioSystem& Get() {
        static AudioSystem instance;
        return instance;
    }
    
    bool Init();
    void Shutdown();
    void Update();
    
    // Sound effects (short, can overlap)
    SoundID LoadSound(const std::string& filepath);
    void UnloadSound(SoundID id);
    void PlaySound(SoundID id, f32 volume = 1.0f, f32 pitch = 1.0f);
    void StopSound(SoundID id);
    
    // Music (streaming, one at a time)
    MusicID LoadMusic(const std::string& filepath);
    void UnloadMusic(MusicID id);
    void PlayMusic(MusicID id, bool loop = true);
    void StopMusic();
    void PauseMusic();
    void ResumeMusic();
    void SetMusicVolume(f32 volume);
    
    // Global controls
    void SetMasterVolume(f32 volume);
    void SetSoundVolume(f32 volume);
    void MuteAll(bool mute);
    bool IsMuted() const { return m_Muted; }
    
    // 3D Audio (for RTS)
    void SetListenerPosition(const Vec3& position);
    void PlaySoundAt(SoundID id, const Vec3& position, f32 volume = 1.0f);
    
private:
    AudioSystem() = default;
    ~AudioSystem() = default;
    
    bool m_Initialized = false;
    bool m_Muted = false;
    f32 m_MasterVolume = 1.0f;
    f32 m_SoundVolume = 1.0f;
    f32 m_MusicVolume = 1.0f;
    
    // OpenAL handles would go here
    // ALCdevice* m_Device = nullptr;
    // ALCcontext* m_Context = nullptr;
    
    std::unordered_map<SoundID, u32> m_SoundBuffers;
    std::unordered_map<MusicID, u32> m_MusicStreams;
    SoundID m_NextSoundID = 1;
    MusicID m_NextMusicID = 1;
};

} // namespace Gini
