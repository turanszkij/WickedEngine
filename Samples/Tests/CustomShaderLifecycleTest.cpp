#include "WickedEngine.h"

#include <iostream>
#include <string>

namespace
{
	bool Check(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << "CustomShader lifecycle test failed: " << message << '\n';
		}
		return condition;
	}
}

int main()
{
	using namespace wi::renderer;

	const size_t initialCount = GetCustomShaders().size();
	CustomShader first;
	first.name = "first";
	first.filterMask = wi::enums::FILTER_OPAQUE;
	const int firstID = RegisterCustomShader(first);
	if (!Check(firstID >= 0, "registration returned an invalid ID"))
		return 1;

	CustomShader resolved;
	if (!Check(!GetCustomShader(-1, resolved), "negative ID resolved") ||
		!Check(!GetCustomShader(firstID + 1, resolved), "unregistered ID resolved"))
		return 1;
	if (!Check(GetCustomShader(firstID, resolved), "registered shader could not be resolved") ||
		!Check(resolved.id == firstID, "resolved shader ID changed") ||
		!Check(resolved.name == first.name, "resolved shader payload differs"))
		return 1;

	CustomShader replacement = first;
	replacement.name = "replacement";
	replacement.filterMask = wi::enums::FILTER_TRANSPARENT;
	replacement.pso[wi::enums::RENDERPASS_MAIN].desc.pt = wi::graphics::PrimitiveTopology::LINELIST;
	const size_t countBeforeUpdate = GetCustomShaders().size();
	if (!Check(UpdateCustomShader(firstID, replacement), "valid update failed") ||
		!Check(GetCustomShaders().size() == countBeforeUpdate, "update grew active registry storage") ||
		!Check(GetCustomShader(firstID, resolved), "updated shader could not be resolved") ||
		!Check(resolved.id == firstID, "update changed stable ID") ||
		!Check(resolved.name == replacement.name, "update did not publish replacement") ||
		!Check(resolved.filterMask == replacement.filterMask, "update did not replace filter mask") ||
		!Check(resolved.pso[wi::enums::RENDERPASS_MAIN].desc.pt == wi::graphics::PrimitiveTopology::LINELIST, "update did not replace PSO payload"))
		return 1;

	CustomShader retainedCopy = resolved;
	CustomShader invalidReplacement;
	invalidReplacement.name = "invalid";
	if (!Check(!UpdateCustomShader(-1, invalidReplacement), "invalid update succeeded") ||
		!Check(GetCustomShader(firstID, resolved), "invalid update removed valid shader") ||
		!Check(resolved.name == replacement.name, "invalid update modified valid shader"))
		return 1;

	CustomShader second;
	second.name = "second";
	const int secondID = RegisterCustomShader(second);
	if (!Check(secondID >= 0 && secondID != firstID, "registration did not return a unique ID") ||
		!Check(UnregisterCustomShader(firstID), "valid unregister failed") ||
		!Check(!GetCustomShader(firstID, resolved), "stale ID resolved after unregister") ||
		!Check(!UnregisterCustomShader(firstID), "repeated unregister succeeded") ||
		!Check(GetCustomShader(secondID, resolved), "swap-removal invalidated another shader") ||
		!Check(retainedCopy.name == replacement.name, "published copy did not survive retirement"))
		return 1;
	// Emulate consecutive render-side lookups between lifecycle safe points.
	for (int draw = 0; draw < 128; ++draw)
	{
		if (!Check(GetCustomShader(secondID, resolved), "render-style lookup failed") ||
			!Check(resolved.id == secondID, "render-style lookup resolved wrong ID"))
			return 1;
	}

	int activeID = RegisterCustomShader(first);
	if (!Check(activeID >= 0 && activeID != firstID && activeID != secondID, "unregistered ID was reused"))
		return 1;

	for (int cycle = 0; cycle < 512; ++cycle)
	{
		CustomShader update = first;
		update.name = "cycle-" + std::to_string(cycle);
		if (!Check(UpdateCustomShader(activeID, update), "stress update failed") ||
			!Check(GetCustomShader(activeID, resolved), "stress resolve failed") ||
			!Check(resolved.name == update.name, "stress update payload mismatch") ||
			!Check(GetCustomShaders().size() == initialCount + 2, "stress update grew active registry"))
			return 1;

		const int staleID = activeID;
		if (!Check(UnregisterCustomShader(staleID), "stress unregister failed") ||
			!Check(!GetCustomShader(staleID, resolved), "stress stale ID resolved"))
			return 1;

		activeID = RegisterCustomShader(first);
		if (!Check(activeID >= 0 && activeID != staleID, "stress registration reused stale ID") ||
			!Check(GetCustomShaders().size() == initialCount + 2, "stress cycle grew active registry"))
			return 1;
	}

	if (!Check(UnregisterCustomShader(activeID), "final stress shader unregister failed") ||
		!Check(UnregisterCustomShader(secondID), "second shader unregister failed") ||
		!Check(GetCustomShaders().size() == initialCount, "registry did not return to initial size"))
		return 1;

	std::cout << "CustomShader lifecycle test passed (512 update/unregister cycles)\n";
	return 0;
}
