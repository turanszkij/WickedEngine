#include "Moon.h"

#include <DirectXMath.h>
#include <wiMath/AngularDisk/AngularDisk.h>

using namespace wi::math;
using namespace wi::scene::environment;

/*
################################################################################
Public
################################################################################
*/

// Methods
//==============================================================================

float Moon::GetAngularRadius() const noexcept {
	return 0.5F * MOON_ANGULAR_DIAMETER * size_multiplier;
}

AngularDisk Moon::GetAngularDisk(const XMVECTOR &direction) const noexcept {
	return AngularDisk{ direction, GetAngularRadius() };
}
