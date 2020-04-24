#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiAudio.h"

#include <memory>
#include <mutex>
#include <unordered_map>

struct wiResource
{
	union
	{
		const void* data = nullptr;
		const wiGraphics::Texture* texture;
		const wiAudio::Sound* sound;
	};

	enum DATA_TYPE
	{
		EMPTY,
		IMAGE,
		SOUND,
	} type = EMPTY;

	~wiResource();
};

namespace wiResourceManager
{
	// Load a resource
	std::shared_ptr<wiResource> Load(const std::string& name);
	// Check if a resource is currently loaded
	bool Contains(const std::string& name);
	// Register a pre-created resource
	std::shared_ptr<wiResource> Register(const std::string& name, void* data, wiResource::DATA_TYPE data_type);
	// Invalidate all resources
	void Clear();
};
