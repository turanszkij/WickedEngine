#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiAudio.h"
#include "wiArchive.h"
#include "wiJobSystem.h"

#include <memory>
#include <mutex>
#include <unordered_map>

struct wiResource
{
	wiGraphics::Texture texture;
	wiAudio::Sound sound;

	enum DATA_TYPE
	{
		EMPTY,
		IMAGE,
		SOUND,
	} type = EMPTY;

	uint32_t flags = 0;
	std::vector<uint8_t> filedata;
};

namespace wiResourceManager
{
	enum MODE
	{
		MODE_DISCARD_FILEDATA_AFTER_LOAD,	// default behaviour: file data will be discarded after loaded. This will not allow serialization of embedded resources, but less memory will be used overall
		MODE_ALLOW_RETAIN_FILEDATA,			// allows keeping the file datas around. This mode allows serialization of embedded resources which request it via IMPORT_RETAIN_FILEDATA
		MODE_ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING // allows keeping file datas, but they won't be embedded by serializer
	};
	void SetMode(MODE param);
	MODE GetMode();

	// Order of these must not change as the flags can be serialized!
	enum FLAGS
	{
		EMPTY = 0,
		IMPORT_COLORGRADINGLUT = 1 << 0, // image import will convert resource to 3D color grading LUT
		IMPORT_RETAIN_FILEDATA = 1 << 1, // file data will be kept for later reuse. This is necessary for keeping the resource serializable
	};

	// Load a resource
	//	name : file name of resource
	//	flags : specify flags that modify behaviour (optional)
	//	filedata : pointer to file data, if file was loaded manually (optional)
	//	filesize : size of file data, if file was loaded manually (optional)
	std::shared_ptr<wiResource> Load(
		const std::string& name,
		uint32_t flags = EMPTY,
		const uint8_t* filedata = nullptr,
		size_t filesize = 0
	);
	// Check if a resource is currently loaded
	bool Contains(const std::string& name);
	// Invalidate all resources
	void Clear();

	struct ResourceSerializer
	{
		std::vector<std::shared_ptr<wiResource>> resources;
	};
	// Serializes all resources that are compatible
	//	Compatible resources are those whose file data is kept around using the IMPORT_RETAIN_FILEDATA flag when loading.
	void Serialize(wiArchive& archive, ResourceSerializer& seri);
};
