#include "wiTransform.h"
#include "wiArchive.h"
#include "wiMath.h"

#include <vector>

std::atomic<uint64_t> Node::__Unique_ID_Counter = 0;

Node::Node() {
	name = "";
	ID = __Unique_ID_Counter.fetch_add(1);
}


void Node::Serialize(wiArchive& archive)
{
	if (archive.IsReadMode())
	{
		archive >> name;
	}
	else
	{
		archive << name;
	}
}


Transform::Transform() :Node() {
	parent = nullptr;
	parentName = "";
	boneParent = "";
	Clear();
}
Transform::~Transform()
{
	detach();
	detachChild();
}


XMMATRIX Transform::getMatrix(int getTranslation, int getRotation, int getScale) {
	return XMLoadFloat4x4(&world);

	//worldPrev=world;
	//translationPrev=translation;
	//scalePrev=scale;
	//rotationPrev=rotation;

	//XMVECTOR s = XMLoadFloat3(&scale_rest);
	//XMVECTOR r = XMLoadFloat4(&rotation_rest);
	//XMVECTOR t = XMLoadFloat3(&translation_rest);
	//XMMATRIX& w = 
	//	XMMatrixScalingFromVector(s)*
	//	XMMatrixRotationQuaternion(r)*
	//	XMMatrixTranslationFromVector(t)
	//	;
	//XMStoreFloat4x4( &world_rest,w );

	//if(parent!=nullptr)
	//{
	//	w = w * XMLoadFloat4x4(&parent_inv_rest) * parent->getMatrix();
	//	XMVECTOR v[3];
	//	XMMatrixDecompose(&v[0],&v[1],&v[2],w);
	//	XMStoreFloat3( &scale,v[0] );
	//	XMStoreFloat4( &rotation,v[1] );
	//	XMStoreFloat3( &translation,v[2] );
	//	XMStoreFloat4x4( &world, w );
	//}
	//else
	//{
	//	world = world_rest;
	//	translation=translation_rest;
	//	rotation=rotation_rest;
	//	scale=scale_rest;
	//}

	//return w;
}
//attach to parent
void Transform::attachTo(Transform* newParent, int copyTranslation, int copyRotation, int copyScale) {
	if (newParent != nullptr) {
		if (parent != nullptr)
		{
			detach();
		}
		parent = newParent;
		parentName = newParent->name;
		copyParentT = copyTranslation;
		copyParentR = copyRotation;
		copyParentS = copyScale;
		XMStoreFloat4x4(&parent_inv_rest, XMMatrixInverse(nullptr, parent->getMatrix(copyParentT, copyParentR, copyParentS)));
		parent->children.insert(this);
	}
}
Transform* Transform::find(const std::string& findname)
{
	if (!name.compare(findname))
	{
		return this;
	}
	for (Transform* x : children)
	{
		if (x != nullptr)
		{
			Transform* found = x->find(findname);
			if (found != nullptr)
			{
				return found;
			}
		}
	}
	return nullptr;
}
Transform* Transform::find(uint64_t id)
{
	if (GetID()==id)
	{
		return this;
	}
	for (Transform* x : children)
	{
		if (x != nullptr)
		{
			Transform* found = x->find(id);
			if (found != nullptr)
			{
				return found;
			}
		}
	}
	return nullptr;
}
//detach child - detach all if no parameters
void Transform::detachChild(Transform* child) {
	if (child == nullptr) {
		if (children.empty())
		{
			return;
		}
		std::vector<Transform*> delChildren(children.begin(), children.end());
		for (Transform* c : delChildren) {
			if (c != nullptr) {
				c->detach();
			}
		}
		children.clear();
	}
	else {
		if (children.find(child) != children.end()) {
			child->detach();
		}
	}
}
//detach from parent
void Transform::detach() {
	if (parent != nullptr) {
		if (parent->children.find(this) != parent->children.end()) {
			parent->children.erase(this);
		}
		applyTransform(copyParentT, copyParentR, copyParentS);
	}
	parent = nullptr;
}
void Transform::applyTransform(int t, int r, int s) {
	if (t)
		translation_rest = translation;
	if (r)
		rotation_rest = rotation;
	if (s)
		scale_rest = scale;
}
void Transform::transform(const XMFLOAT3& t, const XMFLOAT4& r, const XMFLOAT3& s) {
	translation_rest.x += t.x;
	translation_rest.y += t.y;
	translation_rest.z += t.z;

	XMStoreFloat4(&rotation_rest, XMQuaternionNormalize(XMQuaternionMultiply(XMLoadFloat4(&rotation_rest), XMLoadFloat4(&r))));

	scale_rest.x *= s.x;
	scale_rest.y *= s.y;
	scale_rest.z *= s.z;

	UpdateTransform();
}
void Transform::transform(const XMMATRIX& m) {
	XMVECTOR v[3];
	if (XMMatrixDecompose(&v[0], &v[1], &v[2], m)) {
		XMFLOAT3 t, s;
		XMFLOAT4 r;
		XMStoreFloat3(&s, v[0]);
		XMStoreFloat4(&r, v[1]);
		XMStoreFloat3(&t, v[2]);
		transform(t, r, s);
	}
}
void Transform::UpdateTransform()
{
	worldPrev = world;
	translationPrev = translation;
	scalePrev = scale;
	rotationPrev = rotation;

	XMVECTOR s = XMLoadFloat3(&scale_rest);
	XMVECTOR r = XMLoadFloat4(&rotation_rest);
	XMVECTOR t = XMLoadFloat3(&translation_rest);
	XMMATRIX& w =
		XMMatrixScalingFromVector(s)*
		XMMatrixRotationQuaternion(r)*
		XMMatrixTranslationFromVector(t)
		;
	XMStoreFloat4x4(&world_rest, w);

	if (parent != nullptr)
	{
		w = w * XMLoadFloat4x4(&parent_inv_rest) * parent->getMatrix();
		XMVECTOR v[3];
		XMMatrixDecompose(&v[0], &v[1], &v[2], w);
		XMStoreFloat3(&scale, v[0]);
		XMStoreFloat4(&rotation, v[1]);
		XMStoreFloat3(&translation, v[2]);
		XMStoreFloat4x4(&world, w);
	}
	else
	{
		world = world_rest;
		translation = translation_rest;
		rotation = rotation_rest;
		scale = scale_rest;
	}

	for (Transform* child : children)
	{
		child->UpdateTransform();
	}
}
void Transform::Translate(const XMFLOAT3& value)
{
	transform(value);
}
void Transform::RotateRollPitchYaw(const XMFLOAT3& value)
{
	// This needs to be handled a bit differently
	XMVECTOR quat = XMLoadFloat4(&rotation_rest);
	XMVECTOR x = XMQuaternionRotationRollPitchYaw(value.x, 0, 0);
	XMVECTOR y = XMQuaternionRotationRollPitchYaw(0, value.y, 0);
	XMVECTOR z = XMQuaternionRotationRollPitchYaw(0, 0, value.z);

	quat = XMQuaternionMultiply(x, quat);
	quat = XMQuaternionMultiply(quat, y);
	quat = XMQuaternionMultiply(z, quat);

	XMStoreFloat4(&rotation_rest, quat);

	UpdateTransform();
}
void Transform::Rotate(const XMFLOAT4& quaternion)
{
	transform(XMFLOAT3(0, 0, 0), quaternion);
}
void Transform::Scale(const XMFLOAT3& value)
{
	transform(XMFLOAT3(0, 0, 0), XMFLOAT4(0, 0, 0, 1), value);
}
void Transform::Lerp(const Transform* a, const Transform* b, float t)
{
	translation_rest = wiMath::Lerp(a->translation, b->translation, t);
	rotation_rest = wiMath::Slerp(a->rotation, b->rotation, t);
	scale_rest = wiMath::Lerp(a->scale, b->scale, t);
	UpdateTransform();
}
void Transform::CatmullRom(const Transform* a, const Transform* b, const Transform* c, const Transform* d, float t)
{
	XMVECTOR T = XMVectorCatmullRom(
		XMLoadFloat3(&a->translation),
		XMLoadFloat3(&b->translation),
		XMLoadFloat3(&c->translation),
		XMLoadFloat3(&d->translation),
		t
	);
	XMVECTOR R = XMVectorCatmullRom( // not the best choice for quaternions, but it will do for now...
		XMLoadFloat4(&a->rotation),
		XMLoadFloat4(&b->rotation),
		XMLoadFloat4(&c->rotation),
		XMLoadFloat4(&d->rotation),
		t
	);
	R = XMQuaternionNormalize(R);
	XMVECTOR S = XMVectorCatmullRom(
		XMLoadFloat3(&a->scale),
		XMLoadFloat3(&b->scale),
		XMLoadFloat3(&c->scale),
		XMLoadFloat3(&d->scale),
		t
	);

	XMStoreFloat3(&translation_rest, T);
	XMStoreFloat4(&rotation_rest, R);
	XMStoreFloat3(&scale_rest, S);

	UpdateTransform();
}

Transform* Transform::GetRoot()
{
	if (parent != nullptr)
	{
		return parent->GetRoot();
	}
	return this;
}
uint32_t Transform::GetLayerMask() const
{
	if (parent != nullptr)
	{
		return parent->GetLayerMask() & Node::GetLayerMask();
	}
	return Node::GetLayerMask();
}
void Transform::Serialize(wiArchive& archive)
{
	Node::Serialize(archive);

	if (archive.IsReadMode())
	{
		archive >> parentName;
		archive >> translation_rest;
		archive >> rotation_rest;
		archive >> scale_rest;
		archive >> world_rest;
		archive >> boneParent;
		archive >> parent_inv_rest;
		archive >> copyParentT;
		archive >> copyParentR;
		archive >> copyParentS;

		UpdateTransform();
	}
	else
	{
		archive << parentName;
		archive << translation_rest;
		archive << rotation_rest;
		archive << scale_rest;
		archive << world_rest;
		archive << boneParent;
		archive << parent_inv_rest;
		archive << copyParentT;
		archive << copyParentR;
		archive << copyParentS;

	}
}
