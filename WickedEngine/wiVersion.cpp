#include "wiVersion.h"

#include <string>

namespace wiVersion
{
	// main engine core
	const int major = 0;
	// minor features, major updates, breaking API changes
	const int minor = 48;
	// minor bug fixes, alterations, refactors, updates
	const int revision = 3;

	const std::string version_string = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(revision);

	long GetVersion()
	{
		return major * 1000000 + minor * 1000 + revision;
	}
	int GetMajor()
	{
		return major;
	}
	int GetMinor()
	{
		return minor;
	}
	int GetRevision()
	{
		return revision;
	}
	const char* GetVersionString()
	{
		return version_string.c_str();
	}

}
