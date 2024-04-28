#include "wiArchive.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"

#include "Utility/stb_image.h"

namespace wi
{
	// this should always be only INCREMENTED and only if a new serialization is implemeted somewhere!
	static constexpr uint64_t __archiveVersion = 91;
	// this is the version number of which below the archive is not compatible with the current version
	static constexpr uint64_t __archiveVersionBarrier = 22;

	// version history is logged in ArchiveVersionHistory.txt file!

	Archive::Archive()
	{
		CreateEmpty();
	}
	Archive::Archive(const std::string& fileName, bool readMode)
		: readMode(readMode), fileName(fileName)
	{
		if (!fileName.empty())
		{
			directory = wi::helper::GetDirectoryFromPath(fileName);
			if (readMode)
			{
				if (wi::helper::FileRead(fileName, DATA))
				{
					data_ptr = DATA.data();
					data_ptr_size = DATA.size();
					SetReadModeAndResetPos(true);
				}
			}
			else
			{
				CreateEmpty();
			}
		}
	}

	Archive::Archive(const uint8_t* data, size_t size)
	{
		data_ptr = data;
		data_ptr_size = size;
		SetReadModeAndResetPos(true);
	}

	void Archive::CreateEmpty()
	{
		version = __archiveVersion;
		DATA.resize(128); // starting size
		data_ptr = DATA.data();
		data_ptr_size = DATA.size();
		SetReadModeAndResetPos(false);
	}

	void Archive::SetReadModeAndResetPos(bool isReadMode)
	{
		readMode = isReadMode;
		pos = 0;

		if (readMode)
		{
			(*this) >> version;
			if (version < __archiveVersionBarrier)
			{
				wi::helper::messageBox("File is not supported!\nReason: The archive version (" + std::to_string(version) + ") is no longer supported! This is likely because trying to open a file that was created by a version of Wicked Engine that is too old.", "Error!");
				Close();
				return;
			}
			if (version > __archiveVersion)
			{
				wi::helper::messageBox("File is not supported!\nReason: The archive version (" + std::to_string(version) + ") is higher than the program's (" + std::to_string(__archiveVersion) + ")!\nThis is likely due to trying to open an Archive file that was not created by Wicked Engine.", "Error!");
				Close();
				return;
			}
			if (GetVersion() >= 91)
			{
				(*this) >> thumbnail_data_size;
				thumbnail_data_ptr = data_ptr + pos;
				pos += thumbnail_data_size;
			}
		}
		else
		{
			(*this) << version;
			(*this) << thumbnail_data_size;
			const uint8_t* thumbnail_data_dst = data_ptr + pos;
			for (size_t i = 0; i < thumbnail_data_size; ++i)
			{
				(*this) << thumbnail_data_ptr[i];
			}
			thumbnail_data_ptr = thumbnail_data_dst;
		}
	}

	void Archive::Close()
	{
		if (!readMode && !fileName.empty())
		{
			SaveFile(fileName);
		}
		DATA.clear();
	}

	bool Archive::SaveFile(const std::string& fileName)
	{
		return wi::helper::FileWrite(fileName, data_ptr, pos);
	}

	bool Archive::SaveHeaderFile(const std::string& fileName, const std::string& dataName)
	{
		return wi::helper::Bin2H(data_ptr, pos, fileName, dataName.c_str());
	}

	const std::string& Archive::GetSourceDirectory() const
	{
		return directory;
	}

	const std::string& Archive::GetSourceFileName() const
	{
		return fileName;
	}

	wi::graphics::Texture Archive::CreateThumbnailTexture() const
	{
		if (thumbnail_data_size == 0)
			return {};
		int width = 0;
		int height = 0;
		int channels = 0;
		uint8_t* rgba = stbi_load_from_memory(thumbnail_data_ptr, (int)thumbnail_data_size, &width, &height, &channels, 4);
		if (rgba == nullptr)
			return {};
		wi::graphics::Texture texture;
		wi::texturehelper::CreateTexture(texture, rgba, (uint32_t)width, (uint32_t)height);
		stbi_image_free(rgba);
		return texture;
	}

	void Archive::SetThumbnailAndResetPos(const wi::graphics::Texture& texture)
	{
		wi::vector<uint8_t> thumbnail_data;
		wi::helper::saveTextureToMemoryFile(texture, "JPG", thumbnail_data);
		thumbnail_data_size = thumbnail_data.size();
		thumbnail_data_ptr = thumbnail_data.data();
		SetReadModeAndResetPos(false); // start over in write mode with thumbnail image data
	}

	wi::graphics::Texture Archive::PeekThumbnail(const std::string& filename)
	{
		wi::vector<uint8_t> filedata;

		size_t required_size = sizeof(uint64_t) * 2; // version and thumbnail data size

		wi::helper::FileRead(filename, filedata, required_size); // read only up to version and thumbnail data size
		if (filedata.empty())
			return {};

		{
			wi::Archive archive(filedata.data(), filedata.size());
			if (archive.IsOpen())
			{
				if (archive.thumbnail_data_size == 0)
					return {};
				required_size += archive.thumbnail_data_size;
			}
		}

		wi::helper::FileRead(filename, filedata, required_size); // read up to end of thumbnail data
		if (filedata.empty())
			return {};

		wi::Archive archive(filedata.data(), filedata.size());
		return archive.CreateThumbnailTexture();
	}

}
