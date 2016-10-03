#pragma once
#include "CommonInclude.h"
#include "wiLoader.h"

class Frustum;

// TODO: Profile!
#define SORT_SPTREE_CULL

#ifdef SORT_SPTREE_CULL
typedef set<Cullable*, Cullable> CulledList;
#else
typedef unordered_set<Cullable*> CulledList;
#endif

typedef unordered_set<Object*> CulledObjectList;
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
		list<Cullable*> objects;

		Node(Node* newParent, const AABB& newBox = AABB(), int newDepth = 0):parent(newParent),box(newBox),depth(newDepth){
			count=0;
			objects.resize(0);
		}
		static void swap(Node*& a, Node*& b){
			Node* c = a;
			a=b;
			b=c;
		}
		void Release(){
			if(this){
				for(unsigned int i=0;i<children.size();++i){
					if(children[i])
						children[i]->Release();
					children[i]=NULL;
				}
				children.clear();
				objects.clear();
				delete this;
			}
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
		SP_TREE_SORT_BACK_TO_FRONT,
		SP_TREE_SORT_FRONT_TO_BACK
	};

	void AddObjects(Node* node, const vector<Cullable*>& newObjects);
	static void getVisible(Node* node, Frustum& frustum, CulledList& objects, SortType sort = SP_TREE_SORT_FRONT_TO_BACK, CullStrictness type = SP_TREE_STRICT_CULL);
	static void getVisible(Node* node, AABB& frustum, CulledList& objects, SortType sort = SP_TREE_SORT_FRONT_TO_BACK, CullStrictness type = SP_TREE_STRICT_CULL);
	static void getVisible(Node* node, SPHERE& frustum, CulledList& objects, SortType sort = SP_TREE_SORT_FRONT_TO_BACK, CullStrictness type = SP_TREE_STRICT_CULL);
	static void getVisible(Node* node, RAY& frustum, CulledList& objects, SortType sort = SP_TREE_SORT_FRONT_TO_BACK, CullStrictness type = SP_TREE_STRICT_CULL);
	static void getAll(Node* node, CulledList& objects);
	wiSPTree* updateTree(Node* node);
	void Remove(Cullable* value, Node* node = nullptr);

	void CleanUp();
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