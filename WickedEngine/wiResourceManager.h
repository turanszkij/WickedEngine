#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiAudio.h"
#include "wiArchive.h"
#include "wiJobSystem.h"
#include "wiVector.h"
#include "wiVideo.h"
#include "wiUnorderedSet.h"

#include <memory>
#include <string>

namespace wi
{
	// This can hold an asset
	//	It can be loaded from file or memory using wi::resourcemanager::Load()
	struct Resource
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }

		const wi::vector<uint8_t>& GetFileData() const;
		const wi::graphics::Texture& GetTexture() const;
		const wi::audio::Sound& GetSound() const;
		const std::string& GetScript() const;
		const wi::video::Video& GetVideo() const;
		int GetTextureSRGBSubresource() const;
		int GetFontStyle() const;

		void SetFileData(const wi::vector<uint8_t>& data);
		void SetFileData(wi::vector<uint8_t>&& data);
		// Allows to set a Texture to the resource from outside
		//	srgb_subresource: you can provide a subresource for SRGB view if the texture is going to be used as SRGB with the GetTextureSRGBSubresource() (optional)
		void SetTexture(const wi::graphics::Texture& texture, int srgb_subresource = -1);
		void SetSound(const wi::audio::Sound& sound);
		void SetScript(const std::string& script);
		void SetVideo(const wi::video::Video& script);

		// Resource marked for recreate on resourcemanager::Load()
		//	It keeps embedded file data if exists
		void SetOutdated();
	};

	namespace resourcemanager
	{
		enum class Mode
		{
			DISCARD_FILEDATA_AFTER_LOAD,	// default behaviour: file data will be discarded after loaded. This will not allow serialization of embedded resources, but less memory will be used overall
			ALLOW_RETAIN_FILEDATA,			// allows keeping the file datas around. This mode allows serialization of embedded resources which request it via IMPORT_RETAIN_FILEDATA
			ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING // allows keeping file datas, but they won't be embedded by serializer
		};
		void SetMode(Mode param);
		Mode GetMode();
		wi::vector<std::string> GetSupportedImageExtensions();
		wi::vector<std::string> GetSupportedSoundExtensions();
		wi::vector<std::string> GetSupportedVideoExtensions();
		wi::vector<std::string> GetSupportedScriptExtensions();
		wi::vector<std::string> GetSupportedFontStyleExtensions();

		// Order of these must not change as the flags can be serialized!
		enum class Flags
		{
			NONE = 0,
			IMPORT_COLORGRADINGLUT = 1 << 0, // image import will convert resource to 3D color grading LUT
			IMPORT_RETAIN_FILEDATA = 1 << 1, // file data will be kept for later reuse. This is necessary for keeping the resource serializable
			IMPORT_NORMALMAP = 1 << 2, // image import will try to use optimal normal map encoding
			IMPORT_BLOCK_COMPRESSED = 1 << 3, // image import will request block compression for uncompressed or transcodable formats
			IMPORT_DELAY = 1 << 4, // delay importing resource until later, for example when proper flags can be determined
		};

		// Load a resource
		//	name : file name of resource
		//	flags : specify flags that modify behaviour (optional)
		//	filedata : pointer to file data, if file was loaded manually (optional)
		//	filesize : size of file data, if file was loaded manually (optional)
		Resource Load(
			const std::string& name,
			Flags flags = Flags::NONE,
			const uint8_t* filedata = nullptr,
			size_t filesize = 0
		);
		// Check if a resource is currently loaded
		bool Contains(const std::string& name);
		// Invalidate all resources
		void Clear();

		struct ResourceSerializer
		{
			wi::vector<Resource> resources;
		};

		// Serializes all resources that are compatible
		//	Compatible resources are those whose file data is kept around using the IMPORT_RETAIN_FILEDATA flag when loading.
		void Serialize_READ(wi::Archive& archive, ResourceSerializer& resources);
		void Serialize_WRITE(wi::Archive& archive, const wi::unordered_set<std::string>& resource_names);
	}

}

template<>
struct enable_bitmask_operators<wi::resourcemanager::Flags> {
	static const bool enable = true;
};
