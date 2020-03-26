#include "wiArchive.h"
#include "wiHelper.h"

#include <fstream>
#include <sstream>

using namespace std;

// this should always be only INCREMENTED and only if a new serialization is implemeted somewhere!
uint64_t __archiveVersion = 39;
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
		if (readMode)
		{
			ifstream file(fileName, ios::binary | ios::ate);
			if (file.is_open())
			{
				size_t dataSize = (size_t)file.tellg();
				file.seekg(0, file.beg);
				DATA.resize((size_t)dataSize);
				file.read((char*)DATA.data(), dataSize);
				file.close();
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
	if (pos <= 0)
	{
		return false;
	}

	ofstream file(fileName, ios::binary | ios::trunc);
	if (file.is_open())
	{
		file.write((const char*)DATA.data(), (streamsize)pos);
		file.close();
		return true;
	}

	return false;
}

string wiArchive::GetSourceDirectory() const
{
	return wiHelper::GetDirectoryFromPath(fileName);
}

string wiArchive::GetSourceFileName() const
{
	return fileName;
}
