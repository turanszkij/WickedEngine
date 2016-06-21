#include "wiArchive.h"
#include "wiHelper.h"

// this should always be only INCREMENTED and only if a new serialization is implemeted somewhere!
uint64_t __archiveVersion = 1;
// this is the version number of which below the archive is not compatible with the current version
uint64_t __archiveVersionBarrier = 1;

wiArchive::wiArchive(const string& fileName, bool readMode):readMode(readMode),pos(0),DATA(nullptr)
{
	if (!fileName.empty())
	{
		if (readMode)
		{
			file.open(fileName.c_str(), ios::in | ios::binary | ios::ate);
			if (file.is_open())
			{
				streamsize dataSize = file.tellg();
				file.seekg(0, file.beg);
				DATA = new char[(size_t)dataSize];
				file.read(DATA, dataSize);
				file.close();
				(*this) >> version;
				if (version < __archiveVersionBarrier)
				{
					stringstream ss("");
					ss << "The archive version (" << version << ") is no longer supported!";
					wiHelper::messageBox(ss.str(), "Error!");
				}
			}
		}
		else
		{
			file.open(fileName.c_str(), ios::out | ios::binary | ios::trunc);
			if (file.is_open())
			{
				version = __archiveVersion;
				(*this) << version;
			}
		}
	}
}


wiArchive::~wiArchive()
{
	Close();
}

bool wiArchive::IsOpen()
{
	if (IsReadMode())
	{
		return DATA != nullptr;
	}
	return file.is_open();
}

void wiArchive::Close()
{
	if (IsReadMode())
	{
		SAFE_DELETE_ARRAY(DATA);
	}
	else
	{
		file.close();
	}
}
