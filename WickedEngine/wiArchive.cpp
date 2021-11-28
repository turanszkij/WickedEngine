#include "wiArchive.h"
#include "wiHelper.h"

#include <fstream>

// this should always be only INCREMENTED and only if a new serialization is implemeted somewhere!
uint64_t __archiveVersion = 73;
// this is the version number of which below the archive is not compatible with the current version
uint64_t __archiveVersionBarrier = 22;

// version history is logged in ArchiveVersionHistory.txt file!

wiArchive::wiArchive()
{
	CreateEmpty();
}
wiArchive::wiArchive(const std::string& fileName, bool readMode) : fileName(fileName), readMode(readMode)
{
	if (!fileName.empty())
	{
		directory = wiHelper::GetDirectoryFromPath(fileName);
		if (readMode)
		{
			if (wiHelper::FileRead(fileName, DATA))
			{
				data_ptr = DATA.data();
				(*this) >> version;
				if (version < __archiveVersionBarrier)
				{
					wiHelper::messageBox("The archive version (" + std::to_string(version) + ") is no longer supported!", "Error!");
					Close();
				}
				if (version > __archiveVersion)
				{
					wiHelper::messageBox("The archive version (" + std::to_string(version) + ") is higher than the program's (" + std::to_string(__archiveVersion) + ")!", "Error!");
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

wiArchive::wiArchive(const uint8_t* data)
{
	data_ptr = data;
	SetReadModeAndResetPos(true);
}

void wiArchive::CreateEmpty()
{
	version = __archiveVersion;
	DATA.resize(128); // starting size
	data_ptr = DATA.data();
	SetReadModeAndResetPos(false);
}

void wiArchive::SetReadModeAndResetPos(bool isReadMode)
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

void wiArchive::Close()
{
	if (!readMode && !fileName.empty())
	{
		SaveFile(fileName);
	}
	DATA.clear();
}

bool wiArchive::SaveFile(const std::string& fileName)
{
	return wiHelper::FileWrite(fileName, data_ptr, pos);
}

bool wiArchive::SaveHeaderFile(const std::string& fileName, const std::string& dataName)
{
	return wiHelper::Bin2H(data_ptr, pos, fileName, dataName.c_str());
}

const std::string& wiArchive::GetSourceDirectory() const
{
	return directory;
}

const std::string& wiArchive::GetSourceFileName() const
{
	return fileName;
}
