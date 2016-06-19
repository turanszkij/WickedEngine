#include "wiArchive.h"

// this should always be only INCREMENTED and only if a new serialization is implemeted somewhere!
unsigned long __archiveVersion = 0;

wiArchive::wiArchive(const string& fileName, bool readMode):readMode(readMode),pos(0)
{
	if (!fileName.empty())
	{
		if (readMode)
		{
			file.open(fileName.c_str(), ios::in | ios::binary);
			if (IsOpen())
			{
				(*this) >> version;
			}
		}
		else
		{
			file.open(fileName.c_str(), ios::out | ios::binary | ios::trunc);
			if (IsOpen())
			{
				version = __archiveVersion;
				(*this) << version;
			}
		}
	}
}


wiArchive::~wiArchive()
{
	file.close();
}

bool wiArchive::IsOpen()
{
	return file.is_open();
}
