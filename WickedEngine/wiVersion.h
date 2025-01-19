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

	// Full formatted credits with creator, contributors, supporters...
	const char* GetCreditsString();

	// Only the supporters
	const char* GetSupportersString();
}

#endif // WICKEDENGINE_VERSION_DEFINED
