#include "wiSPTree.h"
#include "wiMath.h"
#include "wiLoader.h"
#include "wiFrustum.h"

#define SP_TREE_MAX_DEPTH 12
#define SP_TREE_OBJECT_PER_NODE 6
#define SP_TREE_BOX_CONTAIN


wiSPTree::wiSPTree()
{
	childCount = 0;
	root=nullptr;
}

wiSPTree::~wiSPTree()
{
	SAFE_DELETE(root);
}

void wiSPTree::initialize(const std::vector<Cullable*>& objects){
	initialize(objects,XMFLOAT3(FLOAT32_MAX,FLOAT32_MAX,FLOAT32_MAX)
		,XMFLOAT3(-FLOAT32_MAX,-FLOAT32_MAX,-FLOAT32_MAX));
}
void wiSPTree::initialize(const std::vector<Cullable*>& objects, const XMFLOAT3& newMin, const XMFLOAT3& newMax){
	XMFLOAT3 min = newMin;
	XMFLOAT3 max = newMax;

	for(Cullable* object : objects){
		XMFLOAT3 gMin = object->bounds.getMin();
		XMFLOAT3 gMax = object->bounds.getMax();
		min=wiMath::Min(gMin,min);
		max=wiMath::Max(gMax,max);
	}

	this->root = new wiSPTree::Node(NULL,AABB(min,max));
	AddObjects(this->root,objects);
}

void wiSPTree::AddObjects(Node* node, const std::vector<Cullable*>& newObjects)
{
	for(Cullable* object : newObjects)
	{
		
		node->objects.push_front(object);
	}

	if(newObjects.size()>SP_TREE_OBJECT_PER_NODE && node->depth<SP_TREE_MAX_DEPTH){
		node->count=childCount;
		node->children.reserve(node->count);
		XMFLOAT3 min = node->box.getMin();
		XMFLOAT3 max = node->box.getMax();
		AABB* boxes = new AABB[node->count];
		if(node->count==8){
			boxes[0] = AABB(XMFLOAT3(min.x,(min.y+max.y)*0.5f,(min.z+max.z)*0.5f),XMFLOAT3((min.x+max.x)*0.5f,max.y,max.z));
			boxes[1] = AABB(XMFLOAT3((min.x+max.x)*0.5f,(min.y+max.y)*0.5f,(min.z+max.z)*0.5f),max);
			boxes[2] = AABB(XMFLOAT3(min.x,(min.y+max.y)*0.5f,min.z),XMFLOAT3((min.x+max.x)*0.5f,max.y,(min.z+max.z)*0.5f));
			boxes[3] = AABB(XMFLOAT3((min.x+max.x)*0.5f,(min.y+max.y)*0.5f,min.z),XMFLOAT3(max.x,max.y,(min.z+max.z)*0.5f));
		
			boxes[4] = AABB(XMFLOAT3(min.x,min.y,(min.z+max.z)*0.5f),XMFLOAT3((min.x+max.x)*0.5f,(min.y+max.y)*0.5f,max.z));
			boxes[5] = AABB(XMFLOAT3((min.x+max.x)*0.5f,min.y,(min.z+max.z)*0.5f),XMFLOAT3(max.x,(min.y+max.y)*0.5f,max.z));
			boxes[6] = AABB(min,XMFLOAT3((min.x+max.x)*0.5f,(min.y+max.y)*0.5f,(min.z+max.z)*0.5f));
			boxes[7] = AABB(XMFLOAT3((min.x+max.x)*0.5f,min.y,min.z),XMFLOAT3(max.x,(min.y+max.y)*0.5f,(min.z+max.z)*0.5f));
		}
		else{
			boxes[0] = AABB(XMFLOAT3(min.x,min.y,(min.z+max.z)*0.5f),XMFLOAT3((min.x+max.x)*0.5f,max.y,max.z));
			boxes[1] = AABB(XMFLOAT3((min.x+max.x)*0.5f,min.y,(min.z+max.z)*0.5f),max);
			boxes[2] = AABB(min,XMFLOAT3((min.x+max.x)*0.5f,max.y,(min.z+max.z)*0.5f));
			boxes[3] = AABB(XMFLOAT3((min.x+max.x)*0.5f,min.y,min.z),XMFLOAT3(max.x,max.y,(min.z+max.z)*0.5f));
		}
		for(int i=0;i<node->count;++i){
			//children[i] = new Node(this,boxes[i],depth+1);

			std::vector<Cullable*> o(0);
			o.reserve(newObjects.size());
			for(Cullable* object : newObjects)
#ifdef SP_TREE_BOX_CONTAIN
				if( boxes[i].intersects(object->bounds)==AABB::INSIDE )
#else
				if( (*boxes[i]).intersects(object->translation) )
#endif
				{
					o.push_back(object);
					node->objects.remove(object);
				}
			if(!o.empty()){
				node->children.push_back( new Node(node,boxes[i],node->depth+1) );
				AddObjects(node->children.back(),o);
			}
		}
		delete[] boxes;
	}
	
}

void wiSPTree::Sort(const XMFLOAT3& origin, CulledList& objects, SortType sortType)
{
	switch (sortType)
	{
	case wiSPTree::SP_TREE_SORT_NONE:
		break;
	case wiSPTree::SP_TREE_SORT_UNIQUE:
		// Unique must only be called on a sorted forward_list!
		// And we cannot rely on the by distance sort for this because culled objects could have been gathered by different cullers, so sort by pointers!
		objects.sort([&](const void* a, const void* b) {
			return a < b;
		});
		objects.unique();
		break;
	case wiSPTree::SP_TREE_SORT_BACK_TO_FRONT:
		Sort(origin, objects, wiSPTree::SP_TREE_SORT_UNIQUE);
		objects.sort([&](const Cullable* a, const Cullable* b) {
			return wiMath::DistanceSquared(origin, a->bounds.getCenter()) > wiMath::DistanceSquared(origin, b->bounds.getCenter());
		});
		break;
	case wiSPTree::SP_TREE_SORT_FRONT_TO_BACK:
		Sort(origin, objects, wiSPTree::SP_TREE_SORT_UNIQUE);
		objects.sort([&](const Cullable* a, const Cullable* b) {
			return wiMath::DistanceSquared(origin, a->bounds.getCenter()) < wiMath::DistanceSquared(origin, b->bounds.getCenter());
		});
		break;
	default:
		break;
	}
}

void wiSPTree::getVisible(Frustum& frustum, CulledList& objects, SortType sortType, CullStrictness type, Node* node)
{
	if (node == nullptr)
	{
		node = root;
	}

	int contain_type = frustum.CheckBox(node->box);

	if (!contain_type)
	{
		return;
	}
	else{
		for (Cullable* object : node->objects)
		{
			if (
				type == SP_TREE_LOOSE_CULL ||
				(type == SP_TREE_STRICT_CULL &&
					contain_type == BOX_FRUSTUM_INSIDE ||
					(contain_type == BOX_FRUSTUM_INTERSECTS && frustum.CheckBox(object->bounds))
					)
				)
			{
				objects.push_front(object);
			}
		}
		if(node->count)
		{
			for (unsigned int i = 0; i < node->children.size(); ++i)
			{
				getVisible(frustum, objects, sortType, type, node->children[i]);
			}
		}
	}

	if (node == root)
	{
		Sort(frustum.getCamPos(), objects, sortType);
	}
}
void wiSPTree::getVisible(AABB& frustum, CulledList& objects, SortType sortType, CullStrictness type, Node* node)
{
	if (node == nullptr)
	{
		node = root;
	}

	int contain_type = frustum.intersects(node->box);
	if (!contain_type)
	{
		return;
	}
	else
	{
		for (Cullable* object : node->objects)
		{
			if (
				type == SP_TREE_LOOSE_CULL ||
				(type == SP_TREE_STRICT_CULL &&
					contain_type == AABB::INSIDE ||
					(contain_type == INTERSECTS && frustum.intersects(object->bounds))
					)
				) {
				objects.push_front(object);
			}
		}
		if(node->count)
		{
			for (unsigned int i = 0; i < node->children.size(); ++i)
			{
				getVisible(frustum, objects, sortType, type, node->children[i]);
			}
		}
	}

	if (node == root)
	{
		Sort(frustum.getCenter(), objects, sortType);
	}
}
void wiSPTree::getVisible(SPHERE& frustum, CulledList& objects, SortType sortType, CullStrictness type, Node* node)
{
	if (node == nullptr)
	{
		node = root;
	}

	int contain_type = frustum.intersects(node->box);

	if (!contain_type)
	{
		return;
	}
	else 
	{
		for (Cullable* object : node->objects)
		{
			if (frustum.intersects(object->bounds))
			{
				objects.push_front(object);
			}
		}
		if(node->count)
		{
			for (unsigned int i = 0; i < node->children.size(); ++i)
			{
				getVisible(frustum, objects, sortType, type, node->children[i]);
			}
		}
	}

	if (node == root)
	{
		Sort(frustum.center, objects, sortType);
	}
}
void wiSPTree::getVisible(RAY& frustum, CulledList& objects, SortType sortType, CullStrictness type, Node* node)
{
	if (node == nullptr)
	{
		node = root;
	}

	int contain_type = frustum.intersects(node->box);
	if(!contain_type) return;
	else {
		for(Cullable* object : node->objects)
			if(frustum.intersects(object->bounds))
			{
				objects.push_front(object);
			}
		if(node->count){
			for (unsigned int i = 0; i<node->children.size(); ++i)
				getVisible(frustum,objects, sortType,type,node->children[i]);
		}
	}

	if (node == root)
	{
		Sort(frustum.origin, objects, sortType);
	}
}
void wiSPTree::getAll(CulledList& objects, Node* node)
{
	if (node == nullptr)
	{
		node = root;
	}

	objects.insert_after(objects.end(), node->objects.begin(),node->objects.end());
	if(node->count)
	{
		for (unsigned int i = 0; i < node->children.size(); ++i)
		{
			getAll(objects, node->children[i]);
		}
	}
}
void wiSPTree::Remove(Cullable* value, Node* node)
{
	if (node == nullptr)
	{
		node = root;
	}
	node->objects.remove(value);
	for (auto& child : node->children)
	{
		Remove(value, child);
	}
}

wiSPTree* wiSPTree::updateTree(Node* node)
{
	if (node == nullptr)
	{
		node = root;
	}

	if(node)
	{

		std::vector<Cullable*> bad(0);
		for(Cullable* object : node->objects){
#ifdef SP_TREE_BOX_CONTAIN
			if( node->box.intersects(object->bounds)!=AABB::INSIDE )
#else
			if( !node->box.intersects(object->translation) )
#endif
				bad.push_back(object);
		}

		if(!bad.empty()){
		
			for(Cullable* object : bad){
				node->objects.remove(object);
			}

			if(node->parent){
				AddObjects(node->parent,bad);
			}
			else{
				CulledList culledItems;
				getVisible(node->box, culledItems, SP_TREE_SORT_UNIQUE, SP_TREE_STRICT_CULL, node);
				for(Cullable* item : culledItems){
					bad.push_back(item);
				}

				wiSPTree* tree;
				if(childCount==8) 
					tree = new Octree();
				else 
					tree = new QuadTree();
				AABB nbb = node->box*1.5;
				tree->initialize(bad,nbb.getMin(),nbb.getMax());
				return tree;
			}
		}
		if(!node->children.empty()){
			for (unsigned int i = 0; i<node->children.size(); ++i)
				updateTree(node->children[i]);
		}

	}
	return NULL;
}
