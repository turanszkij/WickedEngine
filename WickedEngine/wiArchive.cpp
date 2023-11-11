#include "wiArchive.h"
#include "wiHelper.h"

namespace wi
{

	// this should always be only INCREMENTED and only if a new serialization is implemeted somewhere!
	static constexpr uint64_t __archiveVersion = 90;
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
					(*this) >> version;
					if (version < __archiveVersionBarrier)
					{
						wi::helper::messageBox("The archive version (" + std::to_string(version) + ") is no longer supported!", "Error!");
						Close();
					}
					if (version > __archiveVersion)
					{
						wi::helper::messageBox("The archive version (" + std::to_string(version) + ") is higher than the program's (" + std::to_string(__archiveVersion) + ")!", "Error!");
						Close();
					}
				}
			}
			else
			{
				CreateEmpty();
			}
		}
	}

	Archive::Archive(const uint8_t* data)
	{
		data_ptr = data;
		SetReadModeAndResetPos(true);
	}

	void Archive::CreateEmpty()
	{
		version = __archiveVersion;
		DATA.resize(128); // starting size
		data_ptr = DATA.data();
		SetReadModeAndResetPos(false);
	}

	void Archive::SetReadModeAndResetPos(bool isReadMode)
	{
		readMode = isReadMode;
		pos = 0;

		if (readMode)
		{
			(*this) >> version;
		}
		else
		{
			(*this) << version;
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

}
