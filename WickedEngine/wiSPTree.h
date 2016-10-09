#pragma once
#include "CommonInclude.h"
#include "wiLoader.h"

class Frustum;

typedef list<Cullable*> CulledList;

typedef list<Object*> CulledObjectList;
typedef unordered_map<Mesh*,CulledObjectList> CulledCollection;


class wiSPTree
{
protected:
	int childCount;
	wiSPTree();
public:
	~wiSPTree();
	void initialize(const vector<Cullable*>& objects);
	void initialize(const vector<Cullable*>& objects, const XMFLOAT3& newMin, const XMFLOAT3& newMax);

	struct Node{
		int depth;
		AABB box;
		vector<Node*> children;
		Node* parent;
		int count;
		CulledList objects;

		Node(Node* newParent, const AABB& newBox = AABB(), int newDepth = 0):parent(newParent),box(newBox),depth(newDepth){
			count=0;
		}
		~Node()
		{
			for (size_t i = 0; i < children.size(); ++i)
			{
				SAFE_DELETE(children[i]);
			}
		}
		static void swap(Node*& a, Node*& b){
			Node* c = a;
			a=b;
			b=c;
		}
	};
	Node* root;
	enum CullStrictness
	{
		SP_TREE_STRICT_CULL,
		SP_TREE_LOOSE_CULL
	};
	enum SortType
	{
		SP_TREE_SORT_NONE,
		SP_TREE_SORT_BACK_TO_FRONT,
		SP_TREE_SORT_FRONT_TO_BACK
	};

	// Sort culled list by their distance to the origin point
	static void Sort(const XMFLOAT3& origin, CulledList& objects, SortType sortType = SP_TREE_SORT_FRONT_TO_BACK);

	void AddObjects(Node* node, const vector<Cullable*>& newObjects);
	void getVisible(Frustum& frustum, CulledList& objects, SortType sortType = SP_TREE_SORT_NONE, CullStrictness type = SP_TREE_STRICT_CULL, Node* node = nullptr);
	void getVisible(AABB& frustum, CulledList& objects, SortType sortType = SP_TREE_SORT_NONE, CullStrictness type = SP_TREE_STRICT_CULL, Node* node = nullptr);
	void getVisible(SPHERE& frustum, CulledList& objects, SortType sortType = SP_TREE_SORT_NONE, CullStrictness type = SP_TREE_STRICT_CULL, Node* node = nullptr);
	void getVisible(RAY& frustum, CulledList& objects, SortType sortType = SP_TREE_SORT_NONE, CullStrictness type = SP_TREE_STRICT_CULL, Node* node = nullptr);
	void getAll(CulledList& objects, Node* node = nullptr);
	// Updates the tree. Returns null if successful, returns a new tree if the tree is resized. The old tree can be thrown away then.
	wiSPTree* updateTree(Node* node = nullptr);
	void Remove(Cullable* value, Node* node = nullptr);
};

class Octree : public wiSPTree
{
public:
	Octree(){childCount=8;}
};
class QuadTree : public wiSPTree
{
public:
	QuadTree(){childCount=4;}
};