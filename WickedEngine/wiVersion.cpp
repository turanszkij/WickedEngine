#include "wiVersion.h"

namespace wiVersion
{
	// major features
	const int major = 0;
	// minor features, major bug fixes
	const int minor = 8;
	// minor bug fixes, alterations, refactors
	const int revision = 45;


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
