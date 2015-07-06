#include "CommonInclude.h"
#include "Hitbox2D.h"


bool Hitbox2D::intersects(const Hitbox2D& b)
{
	if( (!siz.x && !siz.y) || (!b.siz.x && !b.siz.y) )
		return false;

	if( abs(pos.x-b.pos.x)>(siz.x+b.siz.x)*0.5f )
		return false;
	if( abs(pos.y-b.pos.y)>(siz.y+b.siz.y)*0.5f )
		return false;
	
	return true;
}
