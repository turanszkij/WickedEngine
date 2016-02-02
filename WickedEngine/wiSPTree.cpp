#include "wiSPTree.h"
#include "wiMath.h"
#include "wiLoader.h"
#include "wiFrustum.h"

#define SP_TREE_MAX_DEPTH 5
#define SP_TREE_OBJECT_PER_NODE 6
#define SP_TREE_BOX_CONTAIN


wiSPTree::wiSPTree()
{
	root=NULL;
}

wiSPTree::~wiSPTree()
{
	//CleanUp();
}

void wiSPTree::initialize(const vector<Cullable*>& objects){
	initialize(objects,XMFLOAT3(D3D11_FLOAT32_MAX,D3D11_FLOAT32_MAX,D3D11_FLOAT32_MAX)
		,XMFLOAT3(-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX,-D3D11_FLOAT32_MAX));
}
void wiSPTree::initialize(const vector<Cullable*>& objects, const XMFLOAT3& newMin, const XMFLOAT3& newMax){
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

void wiSPTree::CleanUp()
{
	if(root) 
		root->Release();
	root=NULL;
	if(this) 
		delete this;
}


void wiSPTree::AddObjects(Node* node, const vector<Cullable*>& newObjects){
	for(Cullable* object : newObjects){
		/*bool default_mesh = false;
		bool water_mesh = false;
		bool transparent_mesh = false;

		if(object->mesh->renderable)
			for(Material* mat : object->mesh->materials){
				if(!mat->water && !mat->isSky && !mat->transparent)
					default_mesh=true;
				if(mat->water && !mat->isSky)
					water_mesh=true;
				if(!mat->water && !mat->isSky && mat->transparent)
					transparent_mesh=true;
			}*/
		
		node->objects.push_back(object);
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

			vector<Cullable*> o(0);
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

void wiSPTree::getVisible(Node* node, Frustum& frustum, CulledList& objects, SortType sort, CullStrictness type){
	if(!node) return;
	int contain_type = frustum.CheckBox(node->box.corners);
	if(!contain_type) 
		return;
	else{
		for(Cullable* object : node->objects)
			if(
				type==SP_TREE_LOOSE_CULL || 
				(type==SP_TREE_STRICT_CULL &&
					contain_type==BOX_FRUSTUM_INSIDE ||
					(contain_type==BOX_FRUSTUM_INTERSECTS && frustum.CheckBox(object->bounds.corners))
				)
			)
			{

#ifdef SORT_SPTREE_CULL
				object->lastSquaredDistMulThousand=(long)(wiMath::Distance(object->bounds.getCenter(),frustum.getCamPos())*1000);
				if (sort == SP_TREE_SORT_PAINTER)
					object->lastSquaredDistMulThousand *= -1;
#endif

				objects.insert(object);
			}
		if(node->count){
			for (unsigned int i = 0; i<node->children.size(); ++i)
				getVisible(node->children[i],frustum,objects,sort,type);
		}
	}
}
void wiSPTree::getVisible(Node* node, AABB& frustum, CulledList& objects, SortType sort, CullStrictness type){
	if(!node) return;
	int contain_type = frustum.intersects(node->box);
	if(!contain_type) 
		return;
	else{
		for(Cullable* object : node->objects)
			if(
				type==SP_TREE_LOOSE_CULL || 
				(type==SP_TREE_STRICT_CULL &&
					contain_type==AABB::INSIDE ||
					(contain_type==INTERSECTS && frustum.intersects(object->bounds))
				)
			){

#ifdef SORT_SPTREE_CULL
				object->lastSquaredDistMulThousand=(long)(wiMath::Distance(object->bounds.getCenter(),frustum.getCenter())*1000);
				if (sort == SP_TREE_SORT_PAINTER)
					object->lastSquaredDistMulThousand *= -1;
#endif

				objects.insert(object);
			}
		if(node->count){
			for (unsigned int i = 0; i<node->children.size(); ++i)
				getVisible(node->children[i],frustum,objects, sort,type);
		}
	}
}
void wiSPTree::getVisible(Node* node, SPHERE& frustum, CulledList& objects, SortType sort, CullStrictness type){
	if(!node) return;
	int contain_type = frustum.intersects(node->box);
	if(!contain_type) return;
	else {
		for(Cullable* object : node->objects)
			if(frustum.intersects(object->bounds)){

#ifdef SORT_SPTREE_CULL
				object->lastSquaredDistMulThousand=(long)(wiMath::Distance(object->bounds.getCenter(),frustum.center)*1000);
				if (sort == SP_TREE_SORT_PAINTER)
					object->lastSquaredDistMulThousand *= -1;
#endif

				objects.insert(object);
			}
		if(node->count){
			for (unsigned int i = 0; i<node->children.size(); ++i)
				getVisible(node->children[i],frustum,objects, sort,type);
		}
	}
}
void wiSPTree::getVisible(Node* node, RAY& frustum, CulledList& objects, SortType sort, CullStrictness type){
	if(!node) return;
	int contain_type = frustum.intersects(node->box);
	if(!contain_type) return;
	else {
		for(Cullable* object : node->objects)
			if(frustum.intersects(object->bounds)){

#ifdef SORT_SPTREE_CULL
				object->lastSquaredDistMulThousand=(long)(wiMath::Distance(object->bounds.getCenter(),frustum.origin)*1000);
				if (sort == SP_TREE_SORT_PAINTER)
					object->lastSquaredDistMulThousand *= -1;
#endif

				objects.insert(object);
			}
		if(node->count){
			for (unsigned int i = 0; i<node->children.size(); ++i)
				getVisible(node->children[i],frustum,objects, sort,type);
		}
	}
}
void wiSPTree::getAll(Node* node, CulledList& objects){
	if(node != nullptr){
		objects.insert(node->objects.begin(),node->objects.end());
		if(node->count){
			for (unsigned int i = 0; i<node->children.size(); ++i)
				getAll(node->children[i],objects);
		}
	}
}

wiSPTree* wiSPTree::updateTree(Node* node){
	if(this && node)
	{

		vector<Cullable*> bad(0);
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
				getVisible(node,node->box,culledItems);
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
