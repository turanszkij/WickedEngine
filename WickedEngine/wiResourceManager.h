#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiAudio.h"
#include "wiArchive.h"
#include "wiJobSystem.h"
#include "wiVector.h"

#include <memory>

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
		int GetTextureNonSRGBSubresource() const;

		void SetFileData(const wi::vector<uint8_t>& data);
		void SetFileData(wi::vector<uint8_t>&& data);
		void SetTexture(const wi::graphics::Texture& texture);
		void SetSound(const wi::audio::Sound& sound);
		void SetScript(const std::string& script);
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
		wi::vector<std::string> GetSupportedScriptExtensions();

		// Order of these must not change as the flags can be serialized!
		enum class Flags
		{
			NONE = 0,
			IMPORT_COLORGRADINGLUT = 1 << 0, // image import will convert resource to 3D color grading LUT
			IMPORT_RETAIN_FILEDATA = 1 << 1, // file data will be kept for later reuse. This is necessary for keeping the resource serializable
			IMPORT_NORMALMAP = 1 << 2, // image import will try to use optimal normal map encoding
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
		void Serialize(wi::Archive& archive, ResourceSerializer& seri);
	}

}

template<>
struct enable_bitmask_operators<wi::resourcemanager::Flags> {
	static const bool enable = true;
};
