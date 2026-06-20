#include "Sun.h"

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

float Sun::GetAngularRadius() const noexcept {
	return 0.5F * SUN_ANGULAR_DIAMETER * size_multiplier;
}

AngularDisk Sun::GetAngularDisk(const XMVECTOR &direction) const noexcept {
	return AngularDisk{ direction, GetAngularRadius() };
}
