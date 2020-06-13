#include "wiVersion.h"

#include <sstream>

namespace wiVersion
{
	// main engine core
	const int major = 0;
	// minor features, major updates
	const int minor = 42;
	// minor bug fixes, alterations, refactors, updates
	const int revision = 10;

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
	const std::string& GetVersionString()
	{
		return version_string;
	}

}
