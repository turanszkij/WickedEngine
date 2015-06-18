#pragma once
#include "WickedEngine.h"
#include "WickedLoader.h"

class Frustum;
//
//struct cullable_comparator {
//    bool operator() (const Cullable* a, const Cullable* b) const{
//		return a->lastSquaredDistMulThousand<=b->lastSquaredDistMulThousand;
//    }
//};
//typedef set<Cullable*,cullable_comparator> CulledList;
typedef set<Cullable*> CulledList;

typedef set<Object*> CulledObjectList;
typedef unordered_map<Mesh*,CulledObjectList> CulledCollection;


class SPTree
{
protected:
	int childCount;
	SPTree();
public:
	~SPTree();
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
#define SP_TREE_STRICT_CULL 0
#define SP_TREE_LOOSE_CULL 1
	void AddObjects(Node* node, const vector<Cullable*>& newObjects);
	static void getVisible(Node* node, Frustum& frustum, CulledList& objects, int type = SP_TREE_STRICT_CULL);
	static void getVisible(Node* node, AABB& frustum, CulledList& objects, int type = SP_TREE_STRICT_CULL);
	static void getVisible(Node* node, SPHERE& frustum, CulledList& objects, int type = SP_TREE_STRICT_CULL);
	static void getVisible(Node* node, RAY& frustum, CulledList& objects, int type = SP_TREE_STRICT_CULL);
	static void getAll(Node* node, CulledList& objects);
	SPTree* updateTree(Node* node);

	void CleanUp();
};

class Octree : public SPTree
{
public:
	Octree(){childCount=8;}
};
class QuadTree : public SPTree
{
public:
	QuadTree(){childCount=4;}
};