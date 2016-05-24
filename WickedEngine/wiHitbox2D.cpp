#include "CommonInclude.h"
#include "wiHitbox2D.h"
#include "wiMath.h"


bool Hitbox2D::intersects(const Hitbox2D& b)
{
	return wiMath::Collision2D(pos, siz, b.pos, b.siz);
}
