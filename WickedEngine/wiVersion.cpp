#include "wiVersion.h"

namespace wiVersion
{
	// main engine core
	const int major = 0;
	// minor features, major updates
	const int minor = 9;
	// minor bug fixes, alterations, refactors, updates
	const int revision = 5;


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
	string GetVersionString()
	{
		stringstream ss("");
		ss << GetMajor() << "." << GetMinor() << "." << GetRevision();
		return ss.str();
	}

}
