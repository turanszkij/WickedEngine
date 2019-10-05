#pragma once
#include "CommonInclude.h"

#include <string>

namespace wiAudio
{
	void Initialize();
	void CleanUp();

	enum SUBMIX_TYPE
	{
		SUBMIX_TYPE_SOUNDEFFECT,
		SUBMIX_TYPE_MUSIC,
		SUBMIX_TYPE_USER0,
		SUBMIX_TYPE_USER1,
		SUBMIX_TYPE_COUNT,
	};

	struct Sound
	{
		wiCPUHandle handle = WI_NULL_HANDLE;
		~Sound();
	};
	struct SoundInstance
	{
		SUBMIX_TYPE type = SUBMIX_TYPE_SOUNDEFFECT;
		wiCPUHandle handle = WI_NULL_HANDLE;
		~SoundInstance();
	};

	HRESULT CreateSound(const std::string& filename, Sound* sound);
	HRESULT CreateSoundInstance(const Sound* sound, SoundInstance* instance);
	void Destroy(Sound* sound);
	void Destroy(SoundInstance* instance);

	void Play(SoundInstance* instance);
	void Pause(SoundInstance* instance);
	void Stop(SoundInstance* instance);
	void SetVolume(float volume, SoundInstance* instance = nullptr);
	float GetVolume(const SoundInstance* instance = nullptr);

	void SetSubmixVolume(SUBMIX_TYPE type, float volume);
	float GetSubmixVolume(SUBMIX_TYPE type);

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
	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D);
}
