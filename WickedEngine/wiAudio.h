#pragma once
#include "CommonInclude.h"
#include "wiMath.h"

#include <memory>
#include <string>

#ifdef SDL2
#include <SDL2/SDL.h>
#endif

namespace wi::audio
{
	void Initialize();

	// SUBMIX_TYPE specifies the playback channel of sound instances
	//	Do not change the order as this enum can be serialized!
	enum SUBMIX_TYPE
	{
		SUBMIX_TYPE_SOUNDEFFECT,
		SUBMIX_TYPE_MUSIC,
		SUBMIX_TYPE_USER0,
		SUBMIX_TYPE_USER1,
		SUBMIX_TYPE_COUNT,

		ENUM_FORCE_UINT32 = 0xFFFFFFFF, // submix type can be serialized
	};

	// Sound is holding the sound data needed for playback, but it can't be played by itself.
	//	Use SoundInstance for playback the sound data
	struct Sound
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }
	};
	// SoundInstance can be used to play back a Sound with specified effects
	struct SoundInstance
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }

		// You can specify these params before creating the sound instance:
		//	The sound instance will need to be recreated for changes to take effect
		SUBMIX_TYPE type = SUBMIX_TYPE_SOUNDEFFECT;
		float begin = 0;		// beginning of the playback in seconds, relative to the Sound it will be created from (0 = from beginning)
		float length = 0;		// length in seconds (0 = until end)
		float loop_begin = 0;	// loop region begin in seconds, relative to the instance begin time (0 = from beginning)
		float loop_length = 0;	// loop region length in seconds (0 = until the end)

		enum FLAGS
		{
			EMPTY = 0,
			ENABLE_REVERB = 1 << 0,
			LOOPED = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		inline void SetEnableReverb(bool value = true) { if (value) { _flags |= ENABLE_REVERB; } else { _flags &= ~ENABLE_REVERB; } }
		inline bool IsEnableReverb() const { return _flags & ENABLE_REVERB; }
		inline void SetLooped(bool value = true) { if (value) { _flags |= LOOPED; } else { _flags &= ~LOOPED; } }
		inline bool IsLooped() const { return _flags & LOOPED; }
	};

	bool CreateSound(const std::string& filename, Sound* sound);
	bool CreateSound(const uint8_t* data, size_t size, Sound* sound);
	bool CreateSoundInstance(const Sound* sound, SoundInstance* instance);

	void Play(SoundInstance* instance);
	void Pause(SoundInstance* instance);
	void Stop(SoundInstance* instance);
	void SetVolume(float volume, SoundInstance* instance = nullptr);
	float GetVolume(const SoundInstance* instance = nullptr);
	void ExitLoop(SoundInstance* instance);
	bool IsEnded(SoundInstance* instance);

	struct SampleInfo
	{
		const short* samples = nullptr;	// array of samples in the sound
		size_t sample_count = 0;	// number of samples in the sound
		int sample_rate = 0;	// number of samples per second
		uint32_t channel_count = 1;	// number of channels in the samples array (1: mono, 2:stereo, etc.)
	};
	SampleInfo GetSampleInfo(const Sound* sound);
	// Returns the total number of samples that were played since the creation of the sound instance
	uint64_t GetTotalSamplesPlayed(const SoundInstance* instance);

	void SetSubmixVolume(SUBMIX_TYPE type, float volume);
	float GetSubmixVolume(SUBMIX_TYPE type);

	// SoundInstance3D can be attached to a SoundInstance for a 3D effect
	struct SoundInstance3D
	{
		XMFLOAT3 listenerPos = XMFLOAT3(0, 0, 0);
		XMFLOAT3 listenerUp = XMFLOAT3(0, 1, 0);
		XMFLOAT3 listenerFront = XMFLOAT3(0, 0, 1);
		XMFLOAT3 listenerVelocity = XMFLOAT3(0, 0, 0);
		XMFLOAT3 emitterPos = XMFLOAT3(0, 0, 0);
		XMFLOAT3 emitterUp = XMFLOAT3(0, 1, 0);
		XMFLOAT3 emitterFront = XMFLOAT3(0, 0, 1);
		XMFLOAT3 emitterVelocity = XMFLOAT3(0, 0, 0);
		float emitterRadius = 0;
	};
	// Call this every frame the listener or the sound instance 3D orientation changes
	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D);

	// Reverb effects can be used for 3D sound instances globally
	enum REVERB_PRESET
	{
		REVERB_PRESET_DEFAULT,
		REVERB_PRESET_GENERIC,
		REVERB_PRESET_FOREST,
		REVERB_PRESET_PADDEDCELL,
		REVERB_PRESET_ROOM,
		REVERB_PRESET_BATHROOM,
		REVERB_PRESET_LIVINGROOM,
		REVERB_PRESET_STONEROOM,
		REVERB_PRESET_AUDITORIUM,
		REVERB_PRESET_CONCERTHALL,
		REVERB_PRESET_CAVE,
		REVERB_PRESET_ARENA,
		REVERB_PRESET_HANGAR,
		REVERB_PRESET_CARPETEDHALLWAY,
		REVERB_PRESET_HALLWAY,
		REVERB_PRESET_STONECORRIDOR,
		REVERB_PRESET_ALLEY,
		REVERB_PRESET_CITY,
		REVERB_PRESET_MOUNTAINS,
		REVERB_PRESET_QUARRY,
		REVERB_PRESET_PLAIN,
		REVERB_PRESET_PARKINGLOT,
		REVERB_PRESET_SEWERPIPE,
		REVERB_PRESET_UNDERWATER,
		REVERB_PRESET_SMALLROOM,
		REVERB_PRESET_MEDIUMROOM,
		REVERB_PRESET_LARGEROOM,
		REVERB_PRESET_MEDIUMHALL,
		REVERB_PRESET_LARGEHALL,
		REVERB_PRESET_PLATE,
	};
	void SetReverb(REVERB_PRESET preset);
}
