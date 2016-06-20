#include "wiArchive.h"

// this should always be only INCREMENTED and only if a new serialization is implemeted somewhere!
unsigned long __archiveVersion = 0;

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
