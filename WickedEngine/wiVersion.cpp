#include "wiVersion.h"

#include <string>

namespace wi::version
{
	// main engine core
	const int major = 0;
	// minor features, major updates, breaking compatibility changes
	const int minor = 71;
	// minor bug fixes, alterations, refactors, updates
	const int revision = 841;

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

#define SUPPORTERS \
"Nemerle, James Webb, Quifeng Jin, TheGameCreators, Joseph Goldin, Yuri, Sergey K, Yukawa Kanta, Dragon Josh, John, LurkingNinja, Bernardo Del Castillo, " \
"Invictus, Scott Hunt, Yazan Altaki, Tuan NV, Robert MacGregor, cybernescence, Alexander Dahlin, blueapples, Delhills, NI NI, Sherief, ktopoet, Justin Macklin, " \
"Cédric Fabre, TogetherTeam, Bartosz Boczula, Arne Koenig, Ivan Trajchev, nathants, Fahd Ahmed, Gabriel Jadderson, SAS_Controller, Dominik Madarász, Segfault, " \
"Mike amanfo, Dennis Brakhane, rookie, Peter Moore, therealjtgill, Nicolas Embleton, Desuuc, radino1977, Anthony Curtis, manni heck, Matthias Hölzl, Phyffer, " \
"Lucas Pinheiro, Tapkaara, gpman, Anthony Python, Gnowos, Klaus, slaughternaut, Paul Brain, Connor Greaves, Alexandr, Lee Bamber, MCAlarm MC2, Titoutan, Willow, " \
"Aldo, lokimx, K. Osterman, Nomad, ykl, Alex Krokos, Timmy, Avaflow, mat, Hexegonel Samael Michael, Joe Spataro, soru, GeniokV, Mammoth, Ignacio, datae, Jason Rice, " \
"MarsBEKET, Tim, Twisty, Zelf ieats kiezen, Romildo Franco, zNachoh, Dmitriy, Alex Minerva, Stefan Kent, Natty, Sunny Krishna, Vilmos Malárik, Ferrata, Rossakis, " \
"Stefana Andrei, Taylor, Gunnar Kriik, 赟 杨, Rex, Lemon Brother, flxy, meta_leap, Edik, jusik5348, Agnares, Fred Naar, Saki Asui, DarkRaVen, Ray, Russell Searle, Alexandr Dem'yanenko, "\
"Portaloffreedom, nxrighthere, わさび わさび "

	const char* GetCreditsString()
	{
		static const char* credits = R"(
Credits
-----------
Created by Turánszki János

Contributors:
---------------------------
Silas Oler
Matteo De Carlo
Amer Koleci
James Webb
Megumumpkin
Preben Eriksen
Dennis Brakhane

All contributors: https://github.com/turanszkij/WickedEngine/graphs/contributors

Patreon supporters
---------------------------
)" SUPPORTERS;

		return credits;
	}
	const char* GetSupportersString()
	{
		return SUPPORTERS;
	}

}
