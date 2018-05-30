#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_
#include "CommonInclude.h"

#include <atomic>
#include <set>

class wiArchive;

struct Node
{
private:
	static std::atomic<uint64_t> __Unique_ID_Counter;
	uint64_t ID;
	uint32_t layerMask = 0xFFFFFFFF; // all layers enabled by default
public:
	std::string name;

	Node();

	void SetLayerMask(uint32_t value) { layerMask = value; }
	virtual uint32_t GetLayerMask() const { return layerMask; }
	
	uint64_t GetID() const { return ID; }
	void SetID(uint64_t newID) { ID = newID; }
	static const uint64_t INVALID_ID = UINT64_MAX;

	void Serialize(wiArchive& archive);
};

struct Transform : public Node
{
	std::string parentName;
	Transform* parent;
	std::set<Transform*> children;

	XMFLOAT3 translation_rest, translation, translationPrev;
	XMFLOAT4 rotation_rest, rotation, rotationPrev;
	XMFLOAT3 scale_rest, scale, scalePrev;
	XMFLOAT4X4 world_rest, world, worldPrev;

	std::string boneParent;	//for transforms that are parented to bones (for blender-import compatibility)

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
	Transform* find(const std::string& name);
	Transform* find(uint64_t ID);
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
	void Lerp(const Transform* a, const Transform* b, float t);
	void CatmullRom(const Transform* a, const Transform* b, const Transform* c, const Transform* d, float t);
	// Update this transform and children recursively
	virtual void UpdateTransform();
	// Get the root of the tree
	Transform* GetRoot();
	// Layer mask with parent hierarchy masking
	virtual uint32_t GetLayerMask() const override;
	void Serialize(wiArchive& archive);
};

#endif // _TRANSFORM_H_
