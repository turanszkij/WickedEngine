#include "wiVersion.h"

#include <string>

namespace wi::version
{
	// main engine core
	const int major = 0;
	// minor features, major updates, breaking compatibility changes
	const int minor = 71;
	// minor bug fixes, alterations, refactors, updates
	const int revision = 235;

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

	const char* GetCreditsString()
	{
		static const char* credits = R"(
Credits
-----------
Created by Turánszki János | https://github.com/turanszkij


Contributors
------------------
- Silas Oler | https://github.com/Kliaxe
	- Stochastic Screen Space Reflections
	- Realistic Sky
	- Volumetric clouds
	- Exponential height fog
- Matteo De Carlo | https://github.com/portaloffreedom
	- Linux core systems
	- Cmake
- XThemeCore | https://github.com/XthemeCore
	- Morph targets
- Amer Koleci https://github.com/amerkoleci
	- Windows Cmake
	- Graphics API improvements
	- Dear ImGui integration
	- Refactors, fixes
- James Webb | https://github.com/jdswebb
	- Graphics API improvements
	- Fixes
- Ben Vinson | https://github.com/BenV
	- Fixes
- Alexander Weinrauch | https://github.com/AlexAUT
	- Fixes
- Eric Fedosejevs | https://github.com/efedo
	- Fixes
- Johan Aires Rastén | https://github.com/JohanAR
	- Fixes
- Megumumpkin | https://github.com/megumumpkin
	- Linux audio implementation
	- Linux controller implementation
	- Linux network implementation
	- ComponentLibrary implementation
	- Lua property bindings support, improvements
	- GLTF Export
	- Fixes
- Maeve Garside | https://github.com/MolassesLover
	- Linux package files
	- Visual Studio Code support improvement
	- Linux template
- Igor Alexey | https://github.com/sororfortuna
	- QOI image loader integration
- Eddie Ataberk | https://github.com/eddieataberk
	- Physics improvements
- Preben Eriksen | https://github.com/plemsoft
	- Fixes
	- Dear ImGui docking sample.
- matpx | https://github.com/matpx
	- Fixes
- Dennis Brakhane | https://github.com/brakhane
	- Added Liberation Sans default font
- Cop46 | https://github.com/Cop46
	- Brightness, Contrast, Saturation post process

Patreon supporters
---------------------------
- Nemerle
- James Webb
- Quifeng Jin
- TheGameCreators
- Joseph Goldin
- Yuri
- Sergey K
- Yukawa Kanta
- Dragon Josh
- John
- LurkingNinja
- Bernardo Del Castillo
- Invictus
- Scott Hunt
- Yazan Altaki
- Tuan NV
- Robert MacGregor
- cybernescence
- Alexander Dahlin
- blueapples
- Delhills
- NI NI
- Sherief
- ktopoet
- Justin Macklin
- Cédric Fabre
- TogetherTeam
- Bartosz Boczula
- Arne Koenig
- Ivan Trajchev
- nathants
- Fahd Ahmed
- Gabriel Jadderson
- SAS_Controller
- Dominik Madarász
- Segfault
		)";

		return credits;
	}

}
