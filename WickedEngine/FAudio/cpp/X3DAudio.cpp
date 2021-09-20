#include "X3DAudio.h"

F3DAUDIOCPP_API X3DAudioInitialize(
	uint32_t SpeakerChannelMask,
	float SpeedOfSound,
	X3DAUDIO_HANDLE Instance
) {
	F3DAudioInitialize(SpeakerChannelMask, SpeedOfSound, Instance);
}

F3DAUDIOCPP_API X3DAudioCalculate(
	const X3DAUDIO_HANDLE Instance,
	const X3DAUDIO_LISTENER* pListener,
	const X3DAUDIO_EMITTER* pEmitter,
	uint32_t Flags,
	X3DAUDIO_DSP_SETTINGS* pDSPSettings
) {
	F3DAudioCalculate(Instance, pListener, pEmitter, Flags, pDSPSettings);
}

