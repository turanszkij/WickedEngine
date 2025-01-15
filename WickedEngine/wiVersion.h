#ifndef WICKEDENGINE_VERSION_DEFINED
#define WICKEDENGINE_VERSION_DEFINED

namespace wi::version
{
	// major features
	int GetMajor();
	// minor features, major bug fixes
	int GetMinor();
	// minor bug fixes, alterations
	int GetRevision();
	const char* GetVersionString();

	const char* GetCreditsString();
}

#endif // WICKEDENGINE_VERSION_DEFINED
