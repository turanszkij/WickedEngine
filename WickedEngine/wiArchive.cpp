#include "wiArchive.h"

// this should always be only INCREMENTED and only if a new serialization is implemeted somewhere!
unsigned long __archiveVersion = 0;

wiArchive::wiArchive(const string& fileName, bool readMode):readMode(readMode),pos(0)
{
	if (!fileName.empty())
	{
		if (readMode)
		{
			file = fstream(fileName.c_str(), ios::binary | ios::in);
			if (IsOpen())
			{
				file >> version;
			}
		}
		else
		{
			file = fstream(fileName.c_str(), ios::binary | ios::out);
			if (IsOpen())
			{
				version = __archiveVersion;
				file << version;
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
