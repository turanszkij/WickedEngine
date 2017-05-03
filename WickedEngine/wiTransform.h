#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_
#include "CommonInclude.h"

#include <set>

class wiArchive;

static unsigned long long __Unique_ID_Counter = 0;

struct Node
{
private:
	unsigned long long ID;
public:
	string name;

	Node() {
		name = "";
		ID = __Unique_ID_Counter;
		__Unique_ID_Counter++;
	}


	string GetLayerID()
	{
		auto x = name.find_last_of('_');
		if (x != string::npos)
		{
			return name.substr(x + 1);
		}
		return "";
	}
	unsigned long long GetID() { return ID; }

	void Serialize(wiArchive& archive);
};

struct Transform : public Node
{
	string parentName;
	Transform* parent;
	set<Transform*> children;

	XMFLOAT3 translation_rest, translation, translationPrev;
	XMFLOAT4 rotation_rest, rotation, rotationPrev;
	XMFLOAT3 scale_rest, scale, scalePrev;
	XMFLOAT4X4 world_rest, world, worldPrev;

	string boneParent;	//for transforms that are parented to bones (for blender-import compatibility)

	XMFLOAT4X4 parent_inv_rest;
	int copyParentT, copyParentR, copyParentS;

	Transform();
	virtual ~Transform();
	void Clear()
	{
		copyParentT = copyParentR = copyParentS = 1;
		translation_rest = translation = translationPrev = XMFLOAT3(0, 0, 0);
		scale_rest = scale = scalePrev = XMFLOAT3(1, 1, 1);
		rotation_rest = rotation = rotationPrev = XMFLOAT4(0, 0, 0, 1);
		world_rest = world = worldPrev = parent_inv_rest = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	}

	virtual XMMATRIX getMatrix(int getTranslation = 1, int getRotation = 1, int getScale = 1);
	//attach to parent
	void attachTo(Transform* newParent, int copyTranslation = 1, int copyRotation = 1, int copyScale = 1);
	//find transform in tree
	Transform* find(const string& name);
	Transform* find(unsigned long long ID);
	//detach child - detach all if no parameters
	void detachChild(Transform* child = nullptr);
	//detach from parent
	void detach();
	void applyTransform(int t = 1, int r = 1, int s = 1);
	void transform(const XMFLOAT3& t = XMFLOAT3(0, 0, 0), const XMFLOAT4& r = XMFLOAT4(0, 0, 0, 1), const XMFLOAT3& s = XMFLOAT3(1, 1, 1));
	void transform(const XMMATRIX& m = XMMatrixIdentity());
	void Translate(const XMFLOAT3& value);
	void RotateRollPitchYaw(const XMFLOAT3& value);
	void Rotate(const XMFLOAT4& quaternion);
	void Scale(const XMFLOAT3& value);
	// Update this transform and children recursively
	virtual void UpdateTransform();
	// Get the root of the tree
	Transform* GetRoot();
	void Serialize(wiArchive& archive);
};

#endif // _TRANSFORM_H_
