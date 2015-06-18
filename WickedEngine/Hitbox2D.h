#pragma once
class Hitbox2D
{
public:
	XMFLOAT2 pos;
	XMFLOAT2 siz;
	
	Hitbox2D():pos(XMFLOAT2(0,0)),siz(XMFLOAT2(0,0)){}
	Hitbox2D(const XMFLOAT2& newPos, const XMFLOAT2 newSiz):pos(newPos),siz(newSiz){}
	~Hitbox2D(){};

	bool intersects(const Hitbox2D& b);
};

