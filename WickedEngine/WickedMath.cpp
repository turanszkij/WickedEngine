#include "WickedMath.h"


float wiMath::Distance(const XMFLOAT3& v1,const XMFLOAT3& v2)
{
    XMVECTOR vector1 = XMLoadFloat3(&v1);
    XMVECTOR vector2 = XMLoadFloat3(&v2);
    XMVECTOR vectorSub = XMVectorSubtract(vector1,vector2);
    XMVECTOR length = XMVector3Length(vectorSub);

    float Distance = 0.0f;
    XMStoreFloat(&Distance,length);
    return Distance;
}
float wiMath::DistanceSqaured(const XMFLOAT3& v1,const XMFLOAT3& v2)
{
    XMVECTOR vector1 = XMLoadFloat3(&v1);
    XMVECTOR vector2 = XMLoadFloat3(&v2);
    XMVECTOR vectorSub = XMVectorSubtract(vector1,vector2);
    XMVECTOR length = XMVector3LengthSq(vectorSub);

    float Distance = 0.0f;
    XMStoreFloat(&Distance,length);
    return Distance;
}
float wiMath::DistanceEstimated(const XMFLOAT3& v1,const XMFLOAT3& v2)
{
    XMVECTOR vector1 = XMLoadFloat3(&v1);
    XMVECTOR vector2 = XMLoadFloat3(&v2);
    XMVECTOR vectorSub = XMVectorSubtract(vector1,vector2);
    XMVECTOR length = XMVector3LengthEst(vectorSub);

    float Distance = 0.0f;
    XMStoreFloat(&Distance,length);
    return Distance;
}
XMFLOAT3 wiMath::getVectorHalfWayPoint(const XMFLOAT3& a, const XMFLOAT3& b)
{
	return XMFLOAT3( (a.x+b.x)*0.5f,(a.y+b.y)*0.5f,(a.z+b.z)*0.5f );
}

bool wiMath::Collision(const XMFLOAT2& hitBox1Pos, const XMFLOAT2& hitBox1Siz, const XMFLOAT2& hitBox2Pos, const XMFLOAT2& hitBox2Siz)
{
	if( hitBox1Pos.x+hitBox1Siz.x<hitBox2Pos.x )
		return false;
	else if( hitBox1Pos.x>hitBox2Pos.x+hitBox2Siz.x )
		return false;
	else if( hitBox1Pos.y-hitBox1Siz.y>hitBox2Pos.y )
		return false;
	else if( hitBox1Pos.y<hitBox2Pos.y-hitBox2Siz.y )
		return false;
	else
		return true;
}



float wiMath::Lerp(const float& value1,const float& value2,const float& amount)
{
	return value1 + (value2 - value1) * amount;
}
XMFLOAT2 wiMath::Lerp(const XMFLOAT2& a,const XMFLOAT2& b,const float& i)
{
	return XMFLOAT2(Lerp(a.x,b.x,i),Lerp(a.y,b.y,i));
}
XMFLOAT3 wiMath::Lerp(const XMFLOAT3& a,const XMFLOAT3& b,const float& i)
{
	return XMFLOAT3(Lerp(a.x,b.x,i),Lerp(a.y,b.y,i),Lerp(a.z,b.z,i));
}
XMFLOAT4 wiMath::Lerp(const XMFLOAT4& a,const XMFLOAT4& b,const float& i)
{
	return XMFLOAT4(Lerp(a.x,b.x,i),Lerp(a.y,b.y,i),Lerp(a.z,b.z,i),Lerp(a.w,b.w,i));
}
XMFLOAT3 wiMath::Max(const XMFLOAT3& a, const XMFLOAT3& b){
	return XMFLOAT3(max(a.x,b.x),max(a.y,b.y),max(a.z,b.z));
}
XMFLOAT3 wiMath::Min(const XMFLOAT3& a, const XMFLOAT3& b){
	return XMFLOAT3(min(a.x,b.x),min(a.y,b.y),min(a.z,b.z));
}
float wiMath::Clamp(float val, float min, float max)
{
	if(val<min) return min;
	else if(val>max) return max;
	return val;
}

XMFLOAT3 wiMath::getCubicHermiteSplinePos(const XMFLOAT3& startPos, const XMFLOAT3& endPos
									, const XMFLOAT3& startTangent, const XMFLOAT3& endTangent
									, const float& atInterval){
	float x,y,z,t; float r1=1.0f,r4=1.0f;
	t=atInterval;
	x=(2*t*t*t-3*t*t+1)*startPos.x+(-2*t*t*t+3*t*t)*endPos.x+(t*t*t-2*t*t+t)*startTangent.x+(t*t*t-t*t)*endTangent.x; 
	y=(2*t*t*t-3*t*t+1)*startPos.y+(-2*t*t*t+3*t*t)*endPos.y+(t*t*t-2*t*t+1)*startTangent.y+(t*t*t-t*t)*endTangent.y; 
	z=(2*t*t*t-3*t*t+1)*startPos.z+(-2*t*t*t+3*t*t)*endPos.z+(t*t*t-2*t*t+1)*startTangent.z+(t*t*t-t*t)*endTangent.z; 
	return XMFLOAT3(x,y,z);
}
XMFLOAT3 wiMath::getQuadraticBezierPos(const XMFLOAT3& a,const XMFLOAT3&b, const XMFLOAT3& c, const float& t){
	float param0,param1,param2;
	param0=pow(1-t,2);
	param1=2*(1-t)*t;
	param2=pow(t,2);
	float x = param0*a.x+param1*b.x+param2*c.x;
	float y = param0*a.y+param1*b.y+param2*c.y;
	float z = param0*a.z+param1*b.z+param2*c.z;
	return XMFLOAT3(x,y,z);
}
XMFLOAT3 wiMath::getQuadraticBezierPos(const XMFLOAT4& a,const XMFLOAT4&b, const XMFLOAT4& c, const float& t){
	return getQuadraticBezierPos(XMFLOAT3(a.x,a.y,a.z),XMFLOAT3(b.x,b.y,b.z),XMFLOAT3(c.x,c.y,c.z),t);
}