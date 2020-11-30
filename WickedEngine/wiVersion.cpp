#include "wiVersion.h"

#include <string>

namespace wiVersion
{
	// main engine core
	const int major = 0;
	// minor features, major updates, breaking API changes
	const int minor = 50;
	// minor bug fixes, alterations, refactors, updates
	const int revision = 7;

	const std::string version_string = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(revision);

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
