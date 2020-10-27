#include "wiArchive.h"
#include "wiHelper.h"

#include <fstream>
#include <sstream>

using namespace std;

// this should always be only INCREMENTED and only if a new serialization is implemeted somewhere!
uint64_t __archiveVersion = 52;
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
				(*this) >> version;
				if (version < __archiveVersionBarrier)
				{
					stringstream ss("");
					ss << "The archive version (" << version << ") is no longer supported!";
					wiHelper::messageBox(ss.str(), "Error!");
					Close();
				}
				if (version > __archiveVersion)
				{
					stringstream ss("");

					ss << "The archive version (" << version << ") is higher than the program's ("<<__archiveVersion<<")!";
					wiHelper::messageBox(ss.str(), "Error!");
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

void wiArchive::CreateEmpty()
{
	readMode = false;
	pos = 0;

	version = __archiveVersion;
	DATA.resize(128); // starting size
	(*this) << version;
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

bool wiArchive::IsOpen()
{
	// when it is open, DATA is not null because it contains the version number at least!
	return !DATA.empty();
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
	return wiHelper::FileWrite(fileName, DATA.data(), pos);
}

const string& wiArchive::GetSourceDirectory() const
{
	return directory;
}

const string& wiArchive::GetSourceFileName() const
{
	return fileName;
}
