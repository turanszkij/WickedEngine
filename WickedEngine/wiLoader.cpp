#include "wiLoader.h"
#include "wiResourceManager.h"
#include "wiHelper.h"
#include "wiMath.h"
#include "wiRenderer.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"
#include "wiRenderTarget.h"
#include "wiDepthTarget.h"
#include "wiTextureHelper.h"
#include "wiPHYSICS.h"
#include "wiArchive.h"
#include "wiBackLog.h"

#define FORSYTH_IMPLEMENTATION
#include "wiMeshOptimizer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "wiObjLoader.h"

#include <algorithm>
#include <fstream>
#include <iomanip>

using namespace std;
using namespace wiGraphicsTypes;

void LoadWiArmatures(const std::string& directory, const std::string& name, unordered_set<Armature*>& armatures)
{
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file)
	{
		Armature* armature = nullptr;
		while(!file.eof())
		{
			float trans[] = { 0,0,0,0 };
			string line="";
			file>>line;
			if(line[0]=='/' && line.substr(2,8)=="ARMATURE") 
			{
				armature = new Armature(line.substr(11, strlen(line.c_str()) - 11));
				armatures.insert(armature);
			}
			else
			{
				switch(line[0])
				{
				case 'r':
					file>>trans[0]>>trans[1]>>trans[2]>>trans[3];
					armature->rotation_rest = XMFLOAT4(trans[0],trans[1],trans[2],trans[3]);
					break;
				case 's':
					file>>trans[0]>>trans[1]>>trans[2];
					armature->scale_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					break;
				case 't':
					file>>trans[0]>>trans[1]>>trans[2];
					armature->translation_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					{
						XMMATRIX world = XMMatrixScalingFromVector(XMLoadFloat3(&armature->scale))*XMMatrixRotationQuaternion(XMLoadFloat4(&armature->rotation))*XMMatrixTranslationFromVector(XMLoadFloat3(&armature->translation));
						XMStoreFloat4x4( &armature->world_rest,world );
					}
					break;
				case 'b':
					{
						string boneName;
						file>>boneName;
						armature->boneCollection.push_back(new Bone(boneName));
					}
					break;
				case 'p':
					file>>armature->boneCollection.back()->parentName;
					break;
				case 'l':
					{
						float x=0,y=0,z=0,w=0;
						file>>x>>y>>z>>w;
						XMVECTOR quaternion = XMVectorSet(x,y,z,w);
						file>>x>>y>>z;
						XMVECTOR translation = XMVectorSet(x,y,z,0);

						XMMATRIX frame;
						frame= XMMatrixRotationQuaternion(quaternion) * XMMatrixTranslationFromVector(translation) ;

						XMStoreFloat3(&armature->boneCollection.back()->translation_rest,translation);
						XMStoreFloat4(&armature->boneCollection.back()->rotation_rest,quaternion);
						XMStoreFloat4x4(&armature->boneCollection.back()->world_rest,frame);
						XMStoreFloat4x4(&armature->boneCollection.back()->restInv,XMMatrixInverse(0,frame));
						
					}
					break;
				case 'c':
					armature->boneCollection.back()->connected=true;
					break;
				case 'h':
					file>>armature->boneCollection.back()->length;
					break;
				default: break;
				}
			}
		}
	}
	file.close();



	//CREATE FAMILY
	for(Armature* armature : armatures)
	{
		armature->CreateFamily();
	}

}
void LoadWiMaterialLibrary(const std::string& directory, const std::string& name, const std::string& texturesDir,MaterialCollection& materials)
{
	int materialI=(int)(materials.size()-1);

	Material* currentMat = NULL;
	
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file){
		while(!file.eof()){
			string line="";
			file>>line;
			if(line[0]=='/' && !strcmp(line.substr(2,8).c_str(),"MATERIAL")) {
				if (currentMat)
				{
					currentMat->ConvertToPhysicallyBasedMaterial();
					materials.insert(pair<string, Material*>(currentMat->name, currentMat));
				}
				
				currentMat = new Material(line.substr(11, strlen(line.c_str()) - 11));
				materialI++;
			}
			else{
				switch(line[0]){
				case 'd':
					file>>currentMat->diffuseColor.x;
					file>>currentMat->diffuseColor.y;
					file>>currentMat->diffuseColor.z;
					break;
				case 'X':
					currentMat->cast_shadow=false;
					break;
				case 'r':
					{
						string resourceName="";
						file>>resourceName;
						stringstream ss("");
						ss<<directory<<texturesDir<<resourceName.c_str();
						currentMat->surfaceMapName = ss.str();
						currentMat->surfaceMap = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
					}
					break;
				case 'n':
					{
						string resourceName="";
						file>>resourceName;
						stringstream ss("");
						ss<<directory<<texturesDir<<resourceName.c_str();
						currentMat->normalMapName=ss.str();
						currentMat->normalMap = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
					}
					break;
				case 't':
					{
						string resourceName="";
						file>>resourceName;
						stringstream ss("");
						ss<<directory<<texturesDir<<resourceName.c_str();
						currentMat->textureName=ss.str();
						currentMat->texture = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
					}
					file>>currentMat->premultipliedTexture;
					break;
				case 'D':
					{
						string resourceName="";
						file>>resourceName;
						stringstream ss("");
						ss<<directory<<texturesDir<<resourceName.c_str();
						currentMat->displacementMapName=ss.str();
						currentMat->displacementMap = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
					}
					break;
				case 'S':
					{
						string resourceName="";
						file>>resourceName;
						stringstream ss("");
						ss<<directory<<texturesDir<<resourceName.c_str();
						currentMat->specularMapName=ss.str();
						currentMat->specularMap = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
					}
					break;
				case 'a':
					file>>currentMat->alpha;
					break;
				case 'h':
					currentMat->shadeless=true;
					break;
				case 'R':
					file>>currentMat->refractionIndex;
					break;
				case 'e':
					file>>currentMat->enviroReflection;
					break;
				case 's':
					file>>currentMat->specular.x;
					file>>currentMat->specular.y;
					file>>currentMat->specular.z;
					file>>currentMat->specular.w;
					break;
				case 'p':
					file>>currentMat->specular_power;
					break;
				case 'k':
					currentMat->isSky=true;
					break;
				case 'm':
					file>>currentMat->movingTex.x;
					file>>currentMat->movingTex.y;
					file>>currentMat->movingTex.z;
					currentMat->framesToWaitForTexCoordOffset=currentMat->movingTex.z;
					break;
				case 'w':
					currentMat->water=true;
					break;
				case 'u':
					currentMat->subsurfaceScattering=true;
					break;
				case 'b':
					{
						string blend;
						file>>blend;
						if(!blend.compare("ADD"))
							currentMat->blendFlag=BLENDMODE_ADDITIVE;
					}
					break;
				case 'i':
					{
						file>>currentMat->emissive;
					}
					break;
				default:break;
				}
			}
		}
	}
	file.close();
	
	if (currentMat)
	{
		currentMat->ConvertToPhysicallyBasedMaterial();
		materials.insert(pair<string, Material*>(currentMat->name, currentMat));
	}

}
void LoadWiObjects(const std::string& directory, const std::string& name, unordered_set<Object*>& objects
					, unordered_set<Armature*>& armatures
				   , MeshCollection& meshes, const MaterialCollection& materials)
{
	
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file)
	{
		Object* object = nullptr;
		while(!file.eof())
		{
			float trans[] = { 0,0,0,0 };
			string line="";
			file>>line;
			if(line[0]=='/' && !strcmp(line.substr(2,6).c_str(),"OBJECT")) 
			{
				object = new Object(line.substr(9, strlen(line.c_str()) - 9));
				objects.insert(object);
			}
			else
			{
				switch(line[0])
				{
				case 'm':
					{
						string meshName="";
						file>>meshName;
						object->meshName = meshName;
						MeshCollection::iterator iter = meshes.find(meshName);
						
						if(line[1]=='b')
						{ 
							//binary load mesh in place if not present
							if(iter==meshes.end())
							{
								stringstream meshFileName("");
								meshFileName << directory << meshName << ".wimesh";
								Mesh* mesh = new Mesh();
								mesh->LoadFromFile(meshName, meshFileName.str(), materials, armatures);
								object->mesh = mesh;
								meshes.insert(pair<string, Mesh*>(meshName, mesh));
							}
							else
							{
								object->mesh=iter->second;
							}
						}
						else
						{
							if (iter != meshes.end())
							{
								object->mesh = iter->second;
							}
						}
					}
					break;
				case 'p':
					{
						file >> object->parentName;
					}
					break;
				case 'b':
					{
						file >> object->boneParent;
					}
					break;
				case 'I':
					{
						XMFLOAT3 s,t;
						XMFLOAT4 r;
						file>>t.x>>t.y>>t.z>>r.x>>r.y>>r.z>>r.w>>s.x>>s.y>>s.z;
						XMStoreFloat4x4(&object->parent_inv_rest
								, XMMatrixScalingFromVector(XMLoadFloat3(&s)) *
									XMMatrixRotationQuaternion(XMLoadFloat4(&r)) *
									XMMatrixTranslationFromVector(XMLoadFloat3(&t))
							);
					}
					break;
				case 'r':
					file>>trans[0]>>trans[1]>>trans[2]>>trans[3];
					object->Rotate(XMFLOAT4(trans[0], trans[1], trans[2],trans[3]));
					break;
				case 's':
					file>>trans[0]>>trans[1]>>trans[2];
					object->Scale(XMFLOAT3(trans[0], trans[1], trans[2]));
					break;
				case 't':
					file>>trans[0]>>trans[1]>>trans[2];
					object->Translate(XMFLOAT3(trans[0], trans[1], trans[2]));
					break;
				case 'E':
					{
						string systemName,materialName;
						bool visibleEmitter;
						float size,randfac,norfac;
						float count,life,randlife;
						float scaleX,scaleY,rot;
						file>>systemName>>visibleEmitter>>materialName>>size>>randfac>>norfac>>count>>life>>randlife;
						file>>scaleX>>scaleY>>rot;

						if (object->mesh)
						{
							object->eParticleSystems.push_back(
								new wiEmittedParticle(systemName, materialName, object, size, randfac, norfac, count, life, randlife, scaleX, scaleY, rot)
							);
						}
					}
					break;
				case 'H':
					{
						string name,mat,densityG,lenG;
						float len;
						int count;
						file>>name>>mat>>len>>count>>densityG>>lenG;
						
						object->hParticleSystems.push_back(new wiHairParticle(name, len, count, mat, object, densityG, lenG));
					}
					break;
				case 'P':
					object->rigidBody = true;
					file>>object->collisionShape>>object->mass>>
						object->friction>>object->restitution>>object->damping>>object->physicsType>>
						object->kinematic;
					break;
				case 'T':
					file >> object->transparency;
					break;
				default: break;
				}
			}
		}
	}
	file.close();

}
void LoadWiMeshes(const std::string& directory, const std::string& name, MeshCollection& meshes, 
	const unordered_set<Armature*>& armatures, const MaterialCollection& materials)
{
	int meshI=(int)(meshes.size()-1);
	Mesh* currentMesh = NULL;
	
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file){
		while(!file.eof())
		{
			float trans[] = { 0,0,0,0 };
			string line="";
			file>>line;
			if(line[0]=='/' && !line.substr(2,4).compare("MESH")) {
				currentMesh = new Mesh(line.substr(7, strlen(line.c_str()) - 7));
				meshes.insert( pair<string,Mesh*>(currentMesh->name,currentMesh) );
				meshI++;
			}
			else
			{
				switch(line[0])
				{
				case 'p':
					{
						file >> currentMesh->parent;
						for (auto& a : armatures)
						{
							if (!a->name.compare(currentMesh->parent))
							{
								currentMesh->armature = a;
								break;
							}
						}
					}
					break;
				case 'v': 
					currentMesh->vertices_FULL.push_back(Mesh::Vertex_FULL());
					file >> currentMesh->vertices_FULL.back().pos.x;
					file >> currentMesh->vertices_FULL.back().pos.y;
					file >> currentMesh->vertices_FULL.back().pos.z;
					break;
				case 'n':
					if (currentMesh->isBillboarded){
						currentMesh->vertices_FULL.back().nor.x = currentMesh->billboardAxis.x;
						currentMesh->vertices_FULL.back().nor.y = currentMesh->billboardAxis.y;
						currentMesh->vertices_FULL.back().nor.z = currentMesh->billboardAxis.z;
					}
					else{
						file >> currentMesh->vertices_FULL.back().nor.x;
						file >> currentMesh->vertices_FULL.back().nor.y;
						file >> currentMesh->vertices_FULL.back().nor.z;
					}
					break;
				case 'u':
					file >> currentMesh->vertices_FULL.back().tex.x;
					file >> currentMesh->vertices_FULL.back().tex.y;
					break;
				case 'w':
					{
						string nameB;
						float weight=0;
						int BONEINDEX=0;
						file>>nameB>>weight;
						bool gotBone=false;
						if(currentMesh->armature != nullptr){
							int j=0;
							for (auto& b : currentMesh->armature->boneCollection)
							{
								if (!b->name.compare(nameB))
								{
									BONEINDEX = j;
									break;
								}
								j++;
							}
						}
						if (gotBone) 
						{
							//ONLY PROCEED IF CORRESPONDING BONE WAS FOUND
							if (!currentMesh->vertices_FULL.back().wei.x) {
								currentMesh->vertices_FULL.back().wei.x = weight;
								currentMesh->vertices_FULL.back().ind.x = (float)BONEINDEX;
							}
							else if(!currentMesh->vertices_FULL.back().wei.y) {
								currentMesh->vertices_FULL.back().wei.y=weight;
								currentMesh->vertices_FULL.back().ind.y=(float)BONEINDEX;
							}
							else if(!currentMesh->vertices_FULL.back().wei.z) {
								currentMesh->vertices_FULL.back().wei.z=weight;
								currentMesh->vertices_FULL.back().ind.z=(float)BONEINDEX;
							}
							else if(!currentMesh->vertices_FULL.back().wei.w) {
								currentMesh->vertices_FULL.back().wei.w = weight;
								currentMesh->vertices_FULL.back().ind.w = (float)BONEINDEX;
							}
						}

						 //(+RIBBONTRAIL SETUP)(+VERTEXGROUP SETUP)

						if(nameB.find("trailbase")!=string::npos)
							currentMesh->trailInfo.base = (int)(currentMesh->vertices_FULL.size()-1);
						else if(nameB.find("trailtip")!=string::npos)
							currentMesh->trailInfo.tip = (int)(currentMesh->vertices_FULL.size()-1);
						
						bool windAffection=false;
						if(nameB.find("wind")!=string::npos)
							windAffection=true;
						bool gotvg=false;
						for (unsigned int v = 0; v<currentMesh->vertexGroups.size(); ++v)
							if(!nameB.compare(currentMesh->vertexGroups[v].name)){
								gotvg=true;
								currentMesh->vertexGroups[v].addVertex(VertexRef((int)(currentMesh->vertices_FULL.size() - 1), weight));
								if(windAffection)
									currentMesh->vertices_FULL.back().pos.w=weight;
							}
						if(!gotvg){
							currentMesh->vertexGroups.push_back(VertexGroup(nameB));
							currentMesh->vertexGroups.back().addVertex(VertexRef((int)(currentMesh->vertices_FULL.size() - 1), weight));
							if(windAffection)
								currentMesh->vertices_FULL.back().pos.w=weight;
						}
					}
					break;
				case 'i':
					{
						int count;
						file>>count;
						for(int i=0;i<count;i++){
							int index;
							file>>index;
							currentMesh->indices.push_back(index);
						}
						break;
					}
				case 'V': 
					{
						XMFLOAT3 pos;
						file >> pos.x>>pos.y>>pos.z;
						currentMesh->physicsverts.push_back(pos);
					}
					break;
				case 'I':
					{
						int count;
						file>>count;
						for(int i=0;i<count;i++){
							int index;
							file>>index;
							currentMesh->physicsindices.push_back(index);
						}
						break;
					}
				case 'm':
					{
						string mName="";
						file>>mName;
						currentMesh->materialNames.push_back(mName);
						MaterialCollection::const_iterator iter = materials.find(mName);
						if(iter!=materials.end()) {
							currentMesh->subsets.push_back(MeshSubset());
							currentMesh->renderable=true;
							currentMesh->subsets.back().material = (iter->second);
						}
					}
					break;
				case 'a':
					file>>currentMesh->vertices_FULL.back().tex.z;
					break;
				case 'B':
					for(int corner=0;corner<8;++corner){
						file>>currentMesh->aabb.corners[corner].x;
						file>>currentMesh->aabb.corners[corner].y;
						file>>currentMesh->aabb.corners[corner].z;
					}
					break;
				case 'b':
					{
						currentMesh->isBillboarded=true;
						string read = "";
						file>>read;
						transform(read.begin(), read.end(), read.begin(), toupper);
						if(read.find(toupper('y'))!=string::npos) currentMesh->billboardAxis=XMFLOAT3(0,1,0);
						else if(read.find(toupper('x'))!=string::npos) currentMesh->billboardAxis=XMFLOAT3(1,0,0);
						else if(read.find(toupper('z'))!=string::npos) currentMesh->billboardAxis=XMFLOAT3(0,0,1);
						else currentMesh->billboardAxis=XMFLOAT3(0,0,0);
					}
					break;
				case 'S':
					{
						currentMesh->softBody=true;
						string mvgi="",gvgi="",svgi="";
						file>>currentMesh->mass>>currentMesh->friction>>gvgi>>mvgi>>svgi;
						for (unsigned int v = 0; v<currentMesh->vertexGroups.size(); ++v){
							if(!strcmp(mvgi.c_str(),currentMesh->vertexGroups[v].name.c_str()))
								currentMesh->massVG=v;
							if(!strcmp(gvgi.c_str(),currentMesh->vertexGroups[v].name.c_str()))
								currentMesh->goalVG=v;
							if(!strcmp(svgi.c_str(),currentMesh->vertexGroups[v].name.c_str()))
								currentMesh->softVG=v;
						}
					}
					break;
				default: break;
				}
			}
		}
	}
	file.close();
	
	if(currentMesh)
		meshes.insert( pair<string,Mesh*>(currentMesh->name,currentMesh) );

}
void LoadWiActions(const std::string& directory, const std::string& name, unordered_set<Armature*>& armatures)
{
	Armature* armatureI=nullptr;
	Bone* boneI=nullptr;
	int firstFrame=INT_MAX;

	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file){
		while(!file.eof()){
			string line="";
			file>>line;
			if(line[0]=='/' && !strcmp(line.substr(2,8).c_str(),"ARMATURE")) {
				string armaturename = line.substr(11, strlen(line.c_str()) - 11);
				for (auto& a : armatures)
				{
					if (!a->name.compare(armaturename)) {
						armatureI = a;
						break;
					}
				}
			}
			else{
				switch(line[0]){
				case 'C':
					armatureI->actions.push_back(Action());
					file>> armatureI->actions.back().name;
					break;
				case 'A':
					file>> armatureI->actions.back().frameCount;
					break;
				case 'b':
					{
						string boneName;
						file>>boneName;
						boneI = armatureI->GetBone(boneName);
						if (boneI != nullptr)
						{
							boneI->actionFrames.resize(armatureI->actions.size());
						}
					}
					break;
				case 'r':
					{
						int f = 0;
						float x=0,y=0,z=0,w=0;
						file>>f>>x>>y>>z>>w;
						if (boneI != nullptr)
							boneI->actionFrames.back().keyframesRot.push_back(KeyFrame(f,x,y,z,w));
					}
					break;
				case 't':
					{
						int f = 0;
						float x=0,y=0,z=0;
						file>>f>>x>>y>>z;
						if (boneI != nullptr)
							boneI->actionFrames.back().keyframesPos.push_back(KeyFrame(f,x,y,z,0));
					}
					break;
				case 's':
					{
						int f = 0;
						float x=0,y=0,z=0;
						file>>f>>x>>y>>z;
						if(boneI!=nullptr)
							boneI->actionFrames.back().keyframesSca.push_back(KeyFrame(f,x,y,z,0));
					}
					break;
				default: break;
				}
			}
		}
	}
	file.close();
}
void LoadWiLights(const std::string& directory, const std::string& name, unordered_set<Light*>& lights)
{

	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file)
	{
		Light* light = nullptr;
		while(!file.eof())
		{
			string line="";
			file>>line;
			switch(line[0])
			{
			case 'P':
				{
					light = new Light();
					lights.insert(light);
					light->SetType(Light::POINT);
					file >> light->name >> light->shadow;
				}
				break;
			case 'D':
				{
					light = new Light();
					lights.insert(light);
					light->SetType(Light::DIRECTIONAL);
					file>>light->name; 
					light->shadow = true;
				}
				break;
			case 'S':
				{
					light = new Light();
					lights.insert(light);
					light->SetType(Light::SPOT);
					file>>light->name;
					file>>light->shadow>>light->enerDis.z;
				}
				break;
			case 'p':
				{
					file >> light->parentName;
				}
				break;
			case 'b':
				{
					file >> light->boneParent;
				}
				break;
			case 'I':
				{
					XMFLOAT3 s,t;
					XMFLOAT4 r;
					file>>t.x>>t.y>>t.z>>r.x>>r.y>>r.z>>r.w>>s.x>>s.y>>s.z;
					XMStoreFloat4x4(&light->parent_inv_rest
							, XMMatrixScalingFromVector(XMLoadFloat3(&s)) *
								XMMatrixRotationQuaternion(XMLoadFloat4(&r)) *
								XMMatrixTranslationFromVector(XMLoadFloat3(&t))
						);
				}
				break;
			case 't':
				{
					float x,y,z;
					file>>x>>y>>z;
					light->Translate(XMFLOAT3(x, y, z));
					break;
				}
			case 'r':
				{
					float x,y,z,w;
					file>>x>>y>>z>>w;
					light->Rotate(XMFLOAT4(x, y, z, w));
					break;
				}
			case 'c':
				{
					float r,g,b;
					file>>r>>g>>b;
					light->color=XMFLOAT4(r,g,b,0);
					break;
				}
			case 'e':
				file>>light->enerDis.x;
				break;
			case 'd':
				file>>light->enerDis.y;
				break;
			case 'n':
				light->noHalo=true;
				break;
			case 'l':
				{
					string t="";
					file>>t;
					stringstream rim("");
					rim<<directory<<"rims/"<<t;
					Texture2D* tex=nullptr;
					if ((tex = (Texture2D*)wiResourceManager::GetGlobal()->add(rim.str())) != nullptr){
						light->lensFlareRimTextures.push_back(tex);
						light->lensFlareNames.push_back(rim.str());
					}
				}
				break;
			default: break;
			}
		}

	}
	file.close();
}
void LoadWiWorldInfo(const std::string& fileName, WorldInfo& worldInfo, Wind& wind)
{
	string extension = wiHelper::GetExtensionFromFileName(fileName);

	string realName;
	if (!extension.compare("wiw"))
	{
		realName = fileName;
	}
	else if (extension.empty())
	{
		realName = fileName + ".wiw";
	}
	else
	{
		realName = fileName;
		wiHelper::RemoveExtensionFromFileName(realName);
		realName += ".wiw";
	}

	ifstream file(realName);
	if (file)
	{
		while (!file.eof())
		{
			string read = "";
			file >> read;
			switch (read[0])
			{
			case 'h':
				file >> worldInfo.horizon.x >> worldInfo.horizon.y >> worldInfo.horizon.z;
				// coming from blender, de-apply gamma correction:
				worldInfo.horizon.x = powf(worldInfo.horizon.x, 1.0f / 2.2f);
				worldInfo.horizon.y = powf(worldInfo.horizon.y, 1.0f / 2.2f);
				worldInfo.horizon.z = powf(worldInfo.horizon.z, 1.0f / 2.2f);
				break;
			case 'z':
				file >> worldInfo.zenith.x >> worldInfo.zenith.y >> worldInfo.zenith.z;
				// coming from blender, de-apply gamma correction:
				worldInfo.zenith.x = powf(worldInfo.zenith.x, 1.0f / 2.2f);
				worldInfo.zenith.y = powf(worldInfo.zenith.y, 1.0f / 2.2f);
				worldInfo.zenith.z = powf(worldInfo.zenith.z, 1.0f / 2.2f);
				break;
			case 'a':
				file >> worldInfo.ambient.x >> worldInfo.ambient.y >> worldInfo.ambient.z;
				// coming from blender, de-apply gamma correction:
				worldInfo.zenith.x = powf(worldInfo.zenith.x, 1.0f / 2.2f);
				worldInfo.zenith.y = powf(worldInfo.zenith.y, 1.0f / 2.2f);
				worldInfo.zenith.z = powf(worldInfo.zenith.z, 1.0f / 2.2f);
				break;
			case 'W':
			{
				XMFLOAT4 r;
				float s;
				file >> r.x >> r.y >> r.z >> r.w >> s;
				XMStoreFloat3(&wind.direction, XMVector3Transform(XMVectorSet(0, s, 0, 0), XMMatrixRotationQuaternion(XMLoadFloat4(&r))));
			}
			break;
			case 'm':
			{
				float s, e, h;
				file >> s >> e >> h;
				worldInfo.fogSEH = XMFLOAT3(s, e, h);
			}
			break;
			default:break;
			}
		}
	}
	file.close();
}
void LoadWiCameras(const std::string&directory, const std::string& name, std::list<Camera*>& cameras
				   ,const unordered_set<Armature*>& armatures)
{
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file)
	{
		string voidStr("");
		file>>voidStr;
		while(!file.eof()){
			string line="";
			file>>line;
			switch(line[0]){

			case 'c':
				{
					XMFLOAT3 trans;
					XMFLOAT4 rot;
					string name(""),parentA(""),parentB("");
					file>>name>>parentA>>parentB>>trans.x>>trans.y>>trans.z>>rot.x>>rot.y>>rot.z>>rot.w;
			
					cameras.push_back(new Camera(
						trans,rot
						,name)
						);

					for (auto& a : armatures)
					{
						Bone* b = a->GetBone(parentB);
						if (b != nullptr)
						{
							cameras.back()->attachTo(b);
						}
					}

				}
				break;
			case 'I':
				{
					XMFLOAT3 s,t;
					XMFLOAT4 r;
					file>>t.x>>t.y>>t.z>>r.x>>r.y>>r.z>>r.w>>s.x>>s.y>>s.z;
					XMStoreFloat4x4(&cameras.back()->parent_inv_rest
							, XMMatrixScalingFromVector(XMLoadFloat3(&s)) *
								XMMatrixRotationQuaternion(XMLoadFloat4(&r)) *
								XMMatrixTranslationFromVector(XMLoadFloat3(&t))
						);
				}
				break;
			default:break;
			}
		}
	}
	file.close();
}
void LoadWiDecals(const std::string&directory, const std::string& name, const std::string& texturesDir, unordered_set<Decal*>& decals)
{
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file)
	{
		Decal* decal = nullptr;
		string voidStr="";
		file>>voidStr;
		while(!file.eof())
		{
			string line="";
			file>>line;
			switch(line[0])
			{
			case 'd':
				{
					string name;
					XMFLOAT3 loc,scale;
					XMFLOAT4 rot;
					file>>name>>scale.x>>scale.y>>scale.z>>loc.x>>loc.y>>loc.z>>rot.x>>rot.y>>rot.z>>rot.w;
					decal = new Decal(loc, scale, rot);
					decal->name=name;
					decals.insert(decal);
				}
				break;
			case 't':
				{
					string tex="";
					file>>tex;
					stringstream ss("");
					ss<<directory<<texturesDir<<tex;
					decal->addTexture(ss.str());
				}
				break;
			case 'n':
				{
					string tex="";
					file>>tex;
					stringstream ss("");
					ss<<directory<<texturesDir<<tex;
					decal->addNormal(ss.str());
				}
				break;
			default:break;
			};
		}
	}
	file.close();
}


#pragma region SCENE
Model* _CreateWorldNode()
{
	Model* world = new Model;
	world->name = "[WickedEngine-default]{WorldNode}";
	return world;
}

Scene::Scene()
{
	models.push_back(_CreateWorldNode());
}
Scene::~Scene()
{
	for (Model* x : models)
	{
		SAFE_DELETE(x);
	}
}
void Scene::ClearWorld()
{
	for (auto& x : models)
	{
		SAFE_DELETE(x);
	}
	models.clear();

	models.push_back(_CreateWorldNode());
}
Model* Scene::GetWorldNode()
{
	return models[0];
}
void Scene::AddModel(Model* model)
{
	models.push_back(model);
	model->attachTo(models[0]);
}
void Scene::Update()
{
	models[0]->UpdateTransform();

	for (Model* x : models)
	{
		x->UpdateModel();
	}
}
#pragma endregion

#pragma region CULLABLE
Cullable::Cullable():bounds(AABB()){}
void Cullable::Serialize(wiArchive& archive)
{
	bounds.Serialize(archive);
}
#pragma endregion

#pragma region MATERIAL

vector<Material::CustomShader*> Material::customShaderPresets;

Material::~Material() {
	wiResourceManager::GetGlobal()->del(surfaceMapName);
	wiResourceManager::GetGlobal()->del(textureName);
	wiResourceManager::GetGlobal()->del(normalMapName);
	wiResourceManager::GetGlobal()->del(displacementMapName);
	wiResourceManager::GetGlobal()->del(specularMapName);
	surfaceMap = nullptr;
	texture = nullptr;
	normalMap = nullptr;
	displacementMap = nullptr;
	specularMap = nullptr;

	customShader = nullptr; // do not delete
}
void Material::init()
{
	diffuseColor = XMFLOAT3(1, 1, 1);

	surfaceMapName = "";
	surfaceMap = nullptr;

	textureName = "";
	texture = nullptr;
	premultipliedTexture = false;
	blendFlag = BLENDMODE::BLENDMODE_ALPHA;

	normalMapName = "";
	normalMap = nullptr;

	displacementMapName = "";
	displacementMap = nullptr;

	specularMapName = "";
	specularMap = nullptr;

	toonshading = water = false;
	enviroReflection = 0.0f;

	specular = XMFLOAT4(0, 0, 0, 0);
	specular_power = 50;
	movingTex = XMFLOAT3(0, 0, 0);
	framesToWaitForTexCoordOffset = 0;
	texMulAdd = XMFLOAT4(1, 1, 0, 0);
	isSky = water = shadeless = false;
	cast_shadow = true;

	// PBR props
	baseColor = XMFLOAT3(1, 1, 1);
	alpha = 1.0f;
	roughness = 0.0f;
	reflectance = 0.0f;
	metalness = 0.0f;
	refractionIndex = 0.0f;
	subsurfaceScattering = 0.0f;
	emissive = 0.0f;
	normalMapStrength = 1.0f;
	parallaxOcclusionMapping = 0.0f;

	planar_reflections = false;

	alphaRef = 1.0f; // no alpha test by default

	customShader = nullptr;

	engineStencilRef = STENCILREF::STENCILREF_DEFAULT;
	userStencilRef = 0x00;


	// constant buffer creation
	GPUBufferDesc bd;
	bd.BindFlags = BIND_CONSTANT_BUFFER;
	bd.Usage = USAGE_DEFAULT; // try to keep it in default memory because it will probably be updated non frequently!
	bd.CPUAccessFlags = 0;
	bd.ByteWidth = sizeof(MaterialCB);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &constantBuffer);
}
void Material::Update()
{
	framesToWaitForTexCoordOffset -= 1.0f;
	if (framesToWaitForTexCoordOffset <= 0)
	{
		texMulAdd.z = fmodf(texMulAdd.z + movingTex.x*wiRenderer::GetGameSpeed(), 1);
		texMulAdd.w = fmodf(texMulAdd.w + movingTex.y*wiRenderer::GetGameSpeed(), 1);
		framesToWaitForTexCoordOffset = movingTex.z*wiRenderer::GetGameSpeed();
	}

	engineStencilRef = STENCILREF_DEFAULT;
	if (subsurfaceScattering > 0)
	{
		engineStencilRef = STENCILREF_SKIN;
	}
	if (shadeless)
	{
		engineStencilRef = STENCILREF_SHADELESS;
	}
}
void Material::ConvertToPhysicallyBasedMaterial()
{
	baseColor = diffuseColor;
	roughness = max(0.01f, (1 - (float)specular_power / 128.0f));
	metalness = 0.0f;
	reflectance = (specular.x + specular.y + specular.z) / 3.0f * specular.w;
	normalMapStrength = 1.0f;
}
Texture2D* Material::GetBaseColorMap() const
{
	if (texture != nullptr)
	{
		return texture;
	}
	return wiTextureHelper::getInstance()->getWhite();
}
Texture2D* Material::GetNormalMap() const
{
	return normalMap;
	//if (normalMap != nullptr)
	//{
	//	return normalMap;
	//}
	//return wiTextureHelper::getInstance()->getNormalMapDefault();
}
Texture2D* Material::GetSurfaceMap() const
{
	if (surfaceMap != nullptr)
	{
		return surfaceMap;
	}
	return wiTextureHelper::getInstance()->getWhite();
}
Texture2D* Material::GetDisplacementMap() const
{
	if (displacementMap != nullptr)
	{
		return displacementMap;
	}
	return wiTextureHelper::getInstance()->getWhite();
}
void Material::Serialize(wiArchive& archive)
{
	if (archive.IsReadMode())
	{
		archive >> name;
		archive >> surfaceMapName;
		archive >> textureName;
		archive >> premultipliedTexture;
		int temp;
		archive >> temp;
		blendFlag = (BLENDMODE)temp;
		archive >> normalMapName;
		archive >> displacementMapName;
		archive >> specularMapName;
		archive >> water;
		archive >> movingTex;
		archive >> framesToWaitForTexCoordOffset;
		archive >> texMulAdd;
		archive >> cast_shadow;
		archive >> baseColor;
		archive >> alpha;
		archive >> roughness;
		archive >> reflectance;
		archive >> metalness;
		archive >> emissive;
		archive >> refractionIndex;
		archive >> subsurfaceScattering;
		archive >> normalMapStrength;
		if (archive.GetVersion() >= 2)
		{
			archive >> planar_reflections;
		}
		if (archive.GetVersion() >= 3)
		{
			archive >> parallaxOcclusionMapping;
		}
		if (archive.GetVersion() >= 4)
		{
			archive >> alphaRef;
		}

		string texturesDir = archive.GetSourceDirectory() + "textures/";
		if (!surfaceMapName.empty())
		{
			surfaceMapName = texturesDir + surfaceMapName;
			surfaceMap = (Texture2D*)wiResourceManager::GetGlobal()->add(surfaceMapName);
		}
		if (!textureName.empty())
		{
			textureName = texturesDir + textureName;
			texture = (Texture2D*)wiResourceManager::GetGlobal()->add(textureName);
		}
		if (!normalMapName.empty())
		{
			normalMapName = texturesDir + normalMapName;
			normalMap = (Texture2D*)wiResourceManager::GetGlobal()->add(normalMapName);
		}
		if (!displacementMapName.empty())
		{
			displacementMapName = texturesDir + displacementMapName;
			displacementMap = (Texture2D*)wiResourceManager::GetGlobal()->add(displacementMapName);
		}
		if (!specularMapName.empty())
		{
			specularMapName = texturesDir + specularMapName;
			specularMap = (Texture2D*)wiResourceManager::GetGlobal()->add(specularMapName);
		}
	}
	else
	{
		archive << name;
		archive << wiHelper::GetFileNameFromPath(surfaceMapName);
		archive << wiHelper::GetFileNameFromPath(textureName);
		archive << premultipliedTexture;
		archive << (int)blendFlag;
		archive << wiHelper::GetFileNameFromPath(normalMapName);
		archive << wiHelper::GetFileNameFromPath(displacementMapName);
		archive << wiHelper::GetFileNameFromPath(specularMapName);
		archive << water;
		archive << movingTex;
		archive << framesToWaitForTexCoordOffset;
		archive << texMulAdd;
		archive << cast_shadow;
		archive << baseColor;
		archive << alpha;
		archive << roughness;
		archive << reflectance;
		archive << metalness;
		archive << emissive;
		archive << refractionIndex;
		archive << subsurfaceScattering;
		archive << normalMapStrength;
		if (archive.GetVersion() >= 2)
		{
			archive << planar_reflections;
		}
		if (archive.GetVersion() >= 3)
		{
			archive << parallaxOcclusionMapping;
		}
		if (archive.GetVersion() >= 4)
		{
			archive << alphaRef;
		}
	}
}

wiGraphicsTypes::GPUBuffer* Material::constantBuffer_Impostor = nullptr;
void Material::CreateImpostorMaterialCB()
{
	// imposor material constantbuffer is always the same, because every param is baked into the textures
	if (constantBuffer_Impostor == nullptr)
	{
		constantBuffer_Impostor = new wiGraphicsTypes::GPUBuffer;

		MaterialCB mcb;
		ZeroMemory(&mcb, sizeof(mcb));
		mcb.baseColor = XMFLOAT4(1, 1, 1, 1);
		mcb.texMulAdd = XMFLOAT4(1, 1, 0, 0);
		mcb.normalMapStrength = 1.0f;
		mcb.roughness = 1.0f;
		mcb.reflectance = 1.0f;
		mcb.metalness = 1.0f;

		GPUBufferDesc bd;
		bd.BindFlags = BIND_CONSTANT_BUFFER;
		bd.Usage = USAGE_IMMUTABLE;
		bd.CPUAccessFlags = 0;
		bd.ByteWidth = sizeof(MaterialCB);
		SubresourceData initData;
		initData.pSysMem = &mcb;
		wiRenderer::GetDevice()->CreateBuffer(&bd, &initData, constantBuffer_Impostor);
	}
}

void MaterialCB::Create(const Material& mat)
{
	baseColor = XMFLOAT4(mat.baseColor.x, mat.baseColor.y, mat.baseColor.z, mat.alpha);
	texMulAdd = mat.texMulAdd;
	roughness = mat.roughness;
	reflectance = mat.reflectance;
	metalness = mat.metalness;
	emissive = mat.emissive;
	refractionIndex = mat.refractionIndex;
	subsurfaceScattering = mat.subsurfaceScattering;
	normalMapStrength = (mat.normalMap == nullptr ? 0 : mat.normalMapStrength);
	parallaxOcclusionMapping = mat.parallaxOcclusionMapping;
}
#pragma endregion

#pragma region MESHSUBSET

MeshSubset::MeshSubset()
{
	material = nullptr;
	indexBufferOffset = 0;
}
MeshSubset::~MeshSubset()
{
}

#pragma endregion

#pragma region VERTEXGROUP
void VertexGroup::Serialize(wiArchive& archive)
{
	if (archive.IsReadMode())
	{
		archive >> name;
		size_t vertexCount;
		archive >> vertexCount;
		for (size_t i = 0; i < vertexCount; ++i)
		{
			int first;
			float second;
			archive >> first;
			archive >> second;
			vertices.insert(pair<int, float>(first, second));
		}
	}
	else
	{
		archive << name;
		archive << vertices.size();
		for (auto& x : vertices)
		{
			archive << x.first;
			archive << x.second;
		}
	}
}
#pragma endregion

#pragma region MESH

GPUBuffer Mesh::impostorVB_POS;
GPUBuffer Mesh::impostorVB_TEX;

Mesh::Mesh(const string& name) : name(name)
{
	init();
}
Mesh::~Mesh() 
{
	SAFE_DELETE(indexBuffer);
	SAFE_DELETE(vertexBuffer_POS);
	SAFE_DELETE(vertexBuffer_TEX);
	SAFE_DELETE(vertexBuffer_BON);
	SAFE_DELETE(streamoutBuffer_POS);
	SAFE_DELETE(streamoutBuffer_PRE);
}
void Mesh::init()
{
	parent = "";
	indices.resize(0);
	renderable = false;
	doubleSided = false;
	aabb = AABB();
	trailInfo = RibbonTrail();
	armature = nullptr;
	isBillboarded = false;
	billboardAxis = XMFLOAT3(0, 0, 0);
	vertexGroups.clear();
	softBody = false;
	mass = friction = 1;
	massVG = -1;
	goalVG = -1;
	softVG = -1;
	goalPositions.clear();
	goalNormals.clear();
	renderDataComplete = false;
	calculatedAO = false;
	armatureName = "";
	impostorDistance = 100.0f;
	tessellationFactor = 0.0f;
	optimized = false;
	bufferOffset_POS = 0;
	bufferOffset_PRE = 0;
	indexFormat = wiGraphicsTypes::INDEXFORMAT_16BIT;

	SAFE_INIT(indexBuffer);
	SAFE_INIT(vertexBuffer_POS);
	SAFE_INIT(vertexBuffer_TEX);
	SAFE_INIT(vertexBuffer_BON);
	SAFE_INIT(streamoutBuffer_POS);
	SAFE_INIT(streamoutBuffer_PRE);
}

void Mesh::LoadFromFile(const std::string& newName, const std::string& fname, const MaterialCollection& materialColl, const unordered_set<Armature*>& armatures) {
	name = newName;

	BYTE* buffer;
	size_t fileSize;
	if (wiHelper::readByteData(fname, &buffer, fileSize)) {

		int offset = 0;

		int VERSION;
		memcpy(&VERSION, buffer, sizeof(int));
		offset += sizeof(int);

		if (VERSION >= 1001) {
			int doubleside;
			memcpy(&doubleside, buffer + offset, sizeof(int));
			offset += sizeof(int);
			if (doubleside) {
				doubleSided = true;
			}
		}

		int billboard;
		memcpy(&billboard, buffer + offset, sizeof(int));
		offset += sizeof(int);
		if (billboard) {
			char axis;
			memcpy(&axis, buffer + offset, 1);
			offset += 1;

			if (toupper(axis) == 'Y')
				billboardAxis = XMFLOAT3(0, 1, 0);
			else if (toupper(axis) == 'X')
				billboardAxis = XMFLOAT3(1, 0, 0);
			else if (toupper(axis) == 'Z')
				billboardAxis = XMFLOAT3(0, 0, 1);
			else
				billboardAxis = XMFLOAT3(0, 0, 0);
			isBillboarded = true;
		}

		int parented; //parentnamelength
		memcpy(&parented, buffer + offset, sizeof(int));
		offset += sizeof(int);
		if (parented) {
			char* pName = new char[parented + 1]();
			memcpy(pName, buffer + offset, parented);
			offset += parented;
			parent = pName;
			delete[] pName;

			stringstream identified_parent("");
			identified_parent << parent;
			for (Armature* a : armatures) {
				if (!a->name.compare(identified_parent.str())) {
					armatureName = identified_parent.str();
					armature = a;
				}
			}
		}

		int materialCount;
		memcpy(&materialCount, buffer + offset, sizeof(int));
		offset += sizeof(int);
		for (int i = 0; i<materialCount; ++i) {
			int matNameLen;
			memcpy(&matNameLen, buffer + offset, sizeof(int));
			offset += sizeof(int);
			char* matName = new char[matNameLen + 1]();
			memcpy(matName, buffer + offset, matNameLen);
			offset += matNameLen;

			stringstream identified_matname("");
			identified_matname << matName;
			MaterialCollection::const_iterator iter = materialColl.find(identified_matname.str());
			if (iter != materialColl.end()) {
				subsets.push_back(MeshSubset());
				subsets.back().material = iter->second;
				//materials.push_back(iter->second);
			}

			materialNames.push_back(identified_matname.str());
			delete[] matName;
		}
		int rendermesh, vertexCount;
		memcpy(&rendermesh, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&vertexCount, buffer + offset, sizeof(int));
		offset += sizeof(int);

		vertices_FULL.resize(vertexCount);

		for (int i = 0; i<vertexCount; ++i) {
			float v[8];
			memcpy(v, buffer + offset, sizeof(float) * 8);
			offset += sizeof(float) * 8;
			vertices_FULL[i].pos.x = v[0];
			vertices_FULL[i].pos.y = v[1];
			vertices_FULL[i].pos.z = v[2];
			vertices_FULL[i].pos.w = 0;
			if (!isBillboarded) {
				vertices_FULL[i].nor.x = v[3];
				vertices_FULL[i].nor.y = v[4];
				vertices_FULL[i].nor.z = v[5];
			}
			else {
				vertices_FULL[i].nor.x = billboardAxis.x;
				vertices_FULL[i].nor.y = billboardAxis.y;
				vertices_FULL[i].nor.z = billboardAxis.z;
			}
			vertices_FULL[i].tex.x = v[6];
			vertices_FULL[i].tex.y = v[7];
			int matIndex;
			memcpy(&matIndex, buffer + offset, sizeof(int));
			offset += sizeof(int);
			vertices_FULL[i].tex.z = (float)matIndex;

			int weightCount = 0;
			memcpy(&weightCount, buffer + offset, sizeof(int));
			offset += sizeof(int);
			for (int j = 0; j<weightCount; ++j) {

				int weightNameLen = 0;
				memcpy(&weightNameLen, buffer + offset, sizeof(int));
				offset += sizeof(int);
				char* weightName = new char[weightNameLen + 1](); //bone name
				memcpy(weightName, buffer + offset, weightNameLen);
				offset += weightNameLen;
				float weightValue = 0;
				memcpy(&weightValue, buffer + offset, sizeof(float));
				offset += sizeof(float);

#pragma region BONE INDEX SETUP
				string nameB = weightName;
				if (armature) {
					bool gotBone = false;
					int BONEINDEX = 0;
					int b = 0;
					while (!gotBone && b<(int)armature->boneCollection.size()) {
						if (!armature->boneCollection[b]->name.compare(nameB)) {
							gotBone = true;
							BONEINDEX = b; //GOT INDEX OF BONE OF THE WEIGHT IN THE PARENT ARMATURE
						}
						b++;
					}
					if (gotBone) { //ONLY PROCEED IF CORRESPONDING BONE WAS FOUND
						if (!vertices_FULL[i].wei.x) {
							vertices_FULL[i].wei.x = weightValue;
							vertices_FULL[i].ind.x = (float)BONEINDEX;
						}
						else if (!vertices_FULL[i].wei.y) {
							vertices_FULL[i].wei.y = weightValue;
							vertices_FULL[i].ind.y = (float)BONEINDEX;
						}
						else if (!vertices_FULL[i].wei.z) {
							vertices_FULL[i].wei.z = weightValue;
							vertices_FULL[i].ind.z = (float)BONEINDEX;
						}
						else if (!vertices_FULL[i].wei.w) {
							vertices_FULL[i].wei.w = weightValue;
							vertices_FULL[i].ind.w = (float)BONEINDEX;
						}
					}
				}

				//(+RIBBONTRAIL SETUP)(+VERTEXGROUP SETUP)

				if (nameB.find("trailbase") != string::npos)
					trailInfo.base = i;
				else if (nameB.find("trailtip") != string::npos)
					trailInfo.tip = i;

				bool windAffection = false;
				if (nameB.find("wind") != string::npos)
					windAffection = true;
				bool gotvg = false;
				for (unsigned int v = 0; v<vertexGroups.size(); ++v)
					if (!nameB.compare(vertexGroups[v].name)) {
						gotvg = true;
						vertexGroups[v].addVertex(VertexRef(i, weightValue));
						if (windAffection)
							vertices_FULL[i].pos.w = weightValue;
					}
				if (!gotvg) {
					vertexGroups.push_back(VertexGroup(nameB));
					vertexGroups.back().addVertex(VertexRef(i, weightValue));
					if (windAffection)
						vertices_FULL[i].pos.w = weightValue;
				}
#pragma endregion

				delete[] weightName;


			}

		}

		if (rendermesh) {
			int indexCount;
			memcpy(&indexCount, buffer + offset, sizeof(int));
			offset += sizeof(int);
			unsigned int* indexArray = new unsigned int[indexCount];
			memcpy(indexArray, buffer + offset, sizeof(unsigned int)*indexCount);
			offset += sizeof(unsigned int)*indexCount;
			indices.reserve(indexCount);
			for (int i = 0; i<indexCount; ++i) {
				indices.push_back(indexArray[i]);
			}
			delete[] indexArray;

			int softBody;
			memcpy(&softBody, buffer + offset, sizeof(int));
			offset += sizeof(int);
			if (softBody) {
				int softCount[2]; //ind,vert
				memcpy(softCount, buffer + offset, sizeof(int) * 2);
				offset += sizeof(int) * 2;
				unsigned int* softind = new unsigned int[softCount[0]];
				memcpy(softind, buffer + offset, sizeof(unsigned int)*softCount[0]);
				offset += sizeof(unsigned int)*softCount[0];
				float* softvert = new float[softCount[1]];
				memcpy(softvert, buffer + offset, sizeof(float)*softCount[1]);
				offset += sizeof(float)*softCount[1];

				physicsindices.reserve(softCount[0]);
				physicsverts.reserve(softCount[1] / 3);
				for (int i = 0; i<softCount[0]; ++i) {
					physicsindices.push_back(softind[i]);
				}
				for (int i = 0; i<softCount[1]; i += 3) {
					physicsverts.push_back(XMFLOAT3(softvert[i], softvert[i + 1], softvert[i + 2]));
				}

				delete[] softind;
				delete[] softvert;
			}
			else {

			}
		}
		else {

		}

		memcpy(aabb.corners, buffer + offset, sizeof(aabb.corners));
		offset += sizeof(aabb.corners);

		int isSoftbody;
		memcpy(&isSoftbody, buffer + offset, sizeof(int));
		offset += sizeof(int);
		if (isSoftbody) {
			float prop[2]; //mass,friction
			memcpy(prop, buffer + offset, sizeof(float) * 2);
			offset += sizeof(float) * 2;
			softBody = true;
			mass = prop[0];
			friction = prop[1];
			int vglenghts[3]; //goal,mass,soft
			memcpy(vglenghts, buffer + offset, sizeof(int) * 3);
			offset += sizeof(int) * 3;

			char* vgg = new char[vglenghts[0] + 1]();
			char* vgm = new char[vglenghts[1] + 1]();
			char* vgs = new char[vglenghts[2] + 1]();

			memcpy(vgg, buffer + offset, vglenghts[0]);
			offset += vglenghts[0];
			memcpy(vgm, buffer + offset, vglenghts[1]);
			offset += vglenghts[1];
			memcpy(vgs, buffer + offset, vglenghts[2]);
			offset += vglenghts[2];

			for (unsigned int v = 0; v<vertexGroups.size(); ++v) {
				if (!strcmp(vgm, vertexGroups[v].name.c_str()))
					massVG = v;
				if (!strcmp(vgg, vertexGroups[v].name.c_str()))
					goalVG = v;
				if (!strcmp(vgs, vertexGroups[v].name.c_str()))
					softVG = v;
			}

			delete[]vgg;
			delete[]vgm;
			delete[]vgs;
		}

		delete[] buffer;

		renderable = rendermesh == 0 ? false : true;
	}
}
void Mesh::Optimize()
{
	// The optimizer is crashing for many models, remove for now (todo)

	//if (optimized)
	//{
	//	return;
	//}

	//// Vertex cache optimization:
	//{
	//	ForsythVertexIndexType* _indices_in = new ForsythVertexIndexType[this->indices.size()];
	//	ForsythVertexIndexType* _indices_out = new ForsythVertexIndexType[this->indices.size()];
	//	for (size_t i = 0; i < indices.size(); ++i)
	//	{
	//		_indices_in[i] = this->indices[i];
	//	}

	//	ForsythVertexIndexType* result = forsythReorderIndices(_indices_out, _indices_in, (int)(this->indices.size() / 3), (int)(this->vertices_FULL.size()));

	//	for (size_t i = 0; i < indices.size(); ++i)
	//	{
	//		this->indices[i] = _indices_out[i];
	//	}
	//	SAFE_DELETE_ARRAY(_indices_in);
	//	SAFE_DELETE_ARRAY(_indices_out);
	//}

	//optimized = true;
}
void Mesh::CreateRenderData() 
{
	if (!renderDataComplete) 
	{
		// First, assemble vertex, index arrays:

		// In case of recreate, delete data first:
		vertices_POS.clear();
		vertices_TEX.clear();
		vertices_BON.clear();

		// De-interleave vertex arrays:
		vertices_POS.resize(vertices_FULL.size());
		vertices_TEX.resize(vertices_FULL.size());
		// do not resize vertices_BON just yet, not every mesh will need bone vertex data!
		for (size_t i = 0; i < vertices_FULL.size(); ++i)
		{
			// Normalize normals:
			float alpha = vertices_FULL[i].nor.w;
			XMVECTOR nor = XMLoadFloat4(&vertices_FULL[i].nor);
			nor = XMVector3Normalize(nor);
			XMStoreFloat4(&vertices_FULL[i].nor, nor);
			vertices_FULL[i].nor.w = alpha;

			// Normalize bone weights:
			XMFLOAT4& wei = vertices_FULL[i].wei;
			float len = wei.x + wei.y + wei.z + wei.w;
			if (len > 0)
			{
				wei.x /= len;
				wei.y /= len;
				wei.z /= len;
				wei.w /= len;

				if (vertices_BON.empty())
				{
					// Allocate full bone vertex data when we find a correct bone weight.
					vertices_BON.resize(vertices_FULL.size());
				}
				vertices_BON[i] = Vertex_BON(vertices_FULL[i]);
			}

			// Split and type conversion:
			vertices_POS[i] = Vertex_POS(vertices_FULL[i]);
			vertices_TEX[i] = Vertex_TEX(vertices_FULL[i]);
		}

		// Save original vertices. This will be input for CPU skinning / soft bodies
		vertices_Transformed_POS = vertices_POS;
		vertices_Transformed_PRE = vertices_POS; // pre <- pos!! (previous positions will have the current positions initially)

		// Map subset indices:
		for (auto& subset : subsets)
		{
			subset.subsetIndices.clear();
		}
		for (size_t i = 0; i < indices.size(); ++i)
		{
			uint32_t index = indices[i];
			const XMFLOAT4& tex = vertices_FULL[index].tex;
			unsigned int materialIndex = (unsigned int)floor(tex.z);

			assert((materialIndex < (unsigned int)subsets.size()) && "Bad subset index!");

			MeshSubset& subset = subsets[materialIndex];
			subset.subsetIndices.push_back(index);

			if (index >= 65536)
			{
				indexFormat = INDEXFORMAT_32BIT;
			}
		}


		// Goal positions, normals are controlling blending between animation and physics states for soft body rendering:
		goalPositions.clear();
		goalNormals.clear();
		if (goalVG >= 0)
		{
			goalPositions.resize(vertexGroups[goalVG].vertices.size());
			goalNormals.resize(vertexGroups[goalVG].vertices.size());
		}


		// Mapping render vertices to physics vertex representation:
		//	the physics vertices contain unique position, not duplicated by texcoord or normals
		//	this way we can map several renderable vertices to one physics vertex
		//	but the mapping function will actually be indexed by renderable vertex index for efficient retrieval.
		if (!physicsverts.empty() && physicalmapGP.empty())
		{
			for (size_t i = 0; i < vertices_POS.size(); ++i)
			{
				for (size_t j = 0; j < physicsverts.size(); ++j)
				{
					if (fabs(vertices_POS[i].pos.x - physicsverts[j].x) < FLT_EPSILON
						&&	fabs(vertices_POS[i].pos.y - physicsverts[j].y) < FLT_EPSILON
						&&	fabs(vertices_POS[i].pos.z - physicsverts[j].z) < FLT_EPSILON
						)
					{
						physicalmapGP.push_back(static_cast<int>(j));
						break;
					}
				}
			}
		}



		// Create actual GPU data:

		SAFE_DELETE(indexBuffer);
		SAFE_DELETE(vertexBuffer_POS);
		SAFE_DELETE(vertexBuffer_TEX);
		SAFE_DELETE(vertexBuffer_BON);
		SAFE_DELETE(streamoutBuffer_POS);
		SAFE_DELETE(streamoutBuffer_PRE);

		GPUBufferDesc bd;
		SubresourceData InitData;

		if (!hasDynamicVB())
		{
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			ZeroMemory(&InitData, sizeof(InitData));

			InitData.pSysMem = vertices_POS.data();
			bd.ByteWidth = (UINT)(sizeof(Vertex_POS) * vertices_POS.size());
			vertexBuffer_POS = new GPUBuffer;
			wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer_POS);
		}

		if (!vertices_BON.empty())
		{
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_IMMUTABLE;
			bd.BindFlags = BIND_SHADER_RESOURCE;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

			InitData.pSysMem = vertices_BON.data();
			bd.ByteWidth = (UINT)(sizeof(Vertex_BON) * vertices_BON.size());
			vertexBuffer_BON = new GPUBuffer;
			wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer_BON);

			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_DEFAULT;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_UNORDERED_ACCESS;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

			bd.ByteWidth = (UINT)(sizeof(Vertex_POS) * vertices_POS.size());
			streamoutBuffer_POS = new GPUBuffer;
			wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, streamoutBuffer_POS);

			bd.ByteWidth = (UINT)(sizeof(Vertex_POS) * vertices_POS.size());
			streamoutBuffer_PRE = new GPUBuffer;
			wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, streamoutBuffer_PRE);
		}

		// texture coordinate buffers are always static:
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = USAGE_IMMUTABLE;
		bd.CPUAccessFlags = 0;
		bd.BindFlags = BIND_VERTEX_BUFFER;
		bd.MiscFlags = 0;
		InitData.pSysMem = vertices_TEX.data();
		bd.ByteWidth = (UINT)(sizeof(Vertex_TEX) * vertices_TEX.size());
		vertexBuffer_TEX = new GPUBuffer;
		wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer_TEX);


		// Remap index buffer to be continuous across subsets and create gpu buffer data:
		uint32_t counter = 0;
		uint8_t stride;
		void* gpuIndexData;
		if (GetIndexFormat() == INDEXFORMAT_16BIT)
		{
			gpuIndexData = new uint16_t[indices.size()];
			stride = sizeof(uint16_t);
		}
		else
		{
			gpuIndexData = new uint32_t[indices.size()];
			stride = sizeof(uint32_t);
		}

		for (MeshSubset& subset : subsets)
		{
			if (subset.subsetIndices.empty())
			{
				continue;
			}
			subset.indexBufferOffset = counter;

			switch (GetIndexFormat())
			{
			case INDEXFORMAT_16BIT:
				for (auto& x : subset.subsetIndices)
				{
					static_cast<uint16_t*>(gpuIndexData)[counter] = static_cast<uint16_t>(x);
					counter++;
				}
				break;
			default:
				for (auto& x : subset.subsetIndices)
				{
					static_cast<uint32_t*>(gpuIndexData)[counter] = static_cast<uint32_t>(x);
					counter++;
				}
				break;
			}
		}

		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = USAGE_IMMUTABLE;
		bd.CPUAccessFlags = 0;
		bd.BindFlags = BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = stride;
		bd.Format = GetIndexFormat() == INDEXFORMAT_16BIT ? FORMAT_R16_UINT : FORMAT_R32_UINT;
		InitData.pSysMem = gpuIndexData;
		bd.ByteWidth = (UINT)(stride * indices.size());
		indexBuffer = new GPUBuffer;
		wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, indexBuffer);

		SAFE_DELETE_ARRAY(gpuIndexData);


		renderDataComplete = true;
	}

}
void Mesh::CreateImpostorVB()
{
	if (!impostorVB_POS.IsValid())
	{
		Vertex_FULL impostorVertices[6 * 6];

		float stepX = 1.f / 6.f;

		// front
		impostorVertices[0].pos = XMFLOAT4(-1, 1, 0, 0);
		impostorVertices[0].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[0].tex = XMFLOAT4(0, 0, 0, 0);

		impostorVertices[1].pos = XMFLOAT4(-1, -1, 0, 0);
		impostorVertices[1].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[1].tex = XMFLOAT4(0, 1, 0, 0);

		impostorVertices[2].pos = XMFLOAT4(1, 1, 0, 0);
		impostorVertices[2].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[2].tex = XMFLOAT4(stepX, 0, 0, 0);

		impostorVertices[3].pos = XMFLOAT4(-1, -1, 0, 0);
		impostorVertices[3].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[3].tex = XMFLOAT4(0, 1, 0, 0);

		impostorVertices[4].pos = XMFLOAT4(1, -1, 0, 0);
		impostorVertices[4].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[4].tex = XMFLOAT4(stepX, 1, 0, 0);

		impostorVertices[5].pos = XMFLOAT4(1, 1, 0, 0);
		impostorVertices[5].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[5].tex = XMFLOAT4(stepX, 0, 0, 0);

		// right
		impostorVertices[6].pos = XMFLOAT4(0, 1, -1, 0);
		impostorVertices[6].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[6].tex = XMFLOAT4(stepX, 0, 0, 0);

		impostorVertices[7].pos = XMFLOAT4(0, -1, -1, 0);
		impostorVertices[7].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[7].tex = XMFLOAT4(stepX, 1, 0, 0);

		impostorVertices[8].pos = XMFLOAT4(0, 1, 1, 0);
		impostorVertices[8].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[8].tex = XMFLOAT4(stepX*2, 0, 0, 0);

		impostorVertices[9].pos = XMFLOAT4(0, -1, -1, 0);
		impostorVertices[9].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[9].tex = XMFLOAT4(stepX, 1, 0, 0);

		impostorVertices[10].pos = XMFLOAT4(0, -1, 1, 0);
		impostorVertices[10].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[10].tex = XMFLOAT4(stepX*2, 1, 0, 0);

		impostorVertices[11].pos = XMFLOAT4(0, 1, 1, 0);
		impostorVertices[11].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[11].tex = XMFLOAT4(stepX*2, 0, 0, 0);

		// back
		impostorVertices[12].pos = XMFLOAT4(-1, 1, 0, 0);
		impostorVertices[12].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[12].tex = XMFLOAT4(stepX*3, 0, 0, 0);

		impostorVertices[13].pos = XMFLOAT4(1, 1, 0, 0);
		impostorVertices[13].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[13].tex = XMFLOAT4(stepX * 2, 0, 0, 0);

		impostorVertices[14].pos = XMFLOAT4(-1, -1, 0, 0);
		impostorVertices[14].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[14].tex = XMFLOAT4(stepX*3, 1, 0, 0);

		impostorVertices[15].pos = XMFLOAT4(-1, -1, 0, 0);
		impostorVertices[15].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[15].tex = XMFLOAT4(stepX*3, 1, 0, 0);

		impostorVertices[16].pos = XMFLOAT4(1, 1, 0, 0);
		impostorVertices[16].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[16].tex = XMFLOAT4(stepX*2, 0, 0, 0);

		impostorVertices[17].pos = XMFLOAT4(1, -1, 0, 0);
		impostorVertices[17].nor = XMFLOAT4(0, 0, -1, 1);
		impostorVertices[17].tex = XMFLOAT4(stepX*2, 1, 0, 0);

		// left
		impostorVertices[18].pos = XMFLOAT4(0, 1, -1, 0);
		impostorVertices[18].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[18].tex = XMFLOAT4(stepX*4, 0, 0, 0);

		impostorVertices[19].pos = XMFLOAT4(0, 1, 1, 0);
		impostorVertices[19].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[19].tex = XMFLOAT4(stepX * 3, 0, 0, 0);

		impostorVertices[20].pos = XMFLOAT4(0, -1, -1, 0);
		impostorVertices[20].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[20].tex = XMFLOAT4(stepX*4, 1, 0, 0);

		impostorVertices[21].pos = XMFLOAT4(0, -1, -1, 0);
		impostorVertices[21].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[21].tex = XMFLOAT4(stepX*4, 1, 0, 0);

		impostorVertices[22].pos = XMFLOAT4(0, 1, 1, 0);
		impostorVertices[22].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[22].tex = XMFLOAT4(stepX*3, 0, 0, 0);

		impostorVertices[23].pos = XMFLOAT4(0, -1, 1, 0);
		impostorVertices[23].nor = XMFLOAT4(1, 0, 0, 1);
		impostorVertices[23].tex = XMFLOAT4(stepX*3, 1, 0, 0);

		// bottom
		impostorVertices[24].pos = XMFLOAT4(-1, 0, 1, 0);
		impostorVertices[24].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[24].tex = XMFLOAT4(stepX*4, 0, 0, 0);

		impostorVertices[25].pos = XMFLOAT4(1, 0, 1, 0);
		impostorVertices[25].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[25].tex = XMFLOAT4(stepX * 5, 0, 0, 0);

		impostorVertices[26].pos = XMFLOAT4(-1, 0, -1, 0);
		impostorVertices[26].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[26].tex = XMFLOAT4(stepX*4, 1, 0, 0);

		impostorVertices[27].pos = XMFLOAT4(-1, 0, -1, 0);
		impostorVertices[27].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[27].tex = XMFLOAT4(stepX*4, 1, 0, 0);

		impostorVertices[28].pos = XMFLOAT4(1, 0, 1, 0);
		impostorVertices[28].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[28].tex = XMFLOAT4(stepX*5, 0, 0, 0);

		impostorVertices[29].pos = XMFLOAT4(1, 0, -1, 0);
		impostorVertices[29].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[29].tex = XMFLOAT4(stepX*5, 1, 0, 0);

		// top
		impostorVertices[30].pos = XMFLOAT4(-1, 0, 1, 0);
		impostorVertices[30].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[30].tex = XMFLOAT4(stepX*5, 0, 0, 0);

		impostorVertices[31].pos = XMFLOAT4(-1, 0, -1, 0);
		impostorVertices[31].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[31].tex = XMFLOAT4(stepX * 5, 1, 0, 0);

		impostorVertices[32].pos = XMFLOAT4(1, 0, 1, 0);
		impostorVertices[32].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[32].tex = XMFLOAT4(stepX*6, 0, 0, 0);

		impostorVertices[33].pos = XMFLOAT4(-1, 0, -1, 0);
		impostorVertices[33].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[33].tex = XMFLOAT4(stepX*5, 1, 0, 0);

		impostorVertices[34].pos = XMFLOAT4(1, 0, -1, 0);
		impostorVertices[34].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[34].tex = XMFLOAT4(stepX*6, 1, 0, 0);

		impostorVertices[35].pos = XMFLOAT4(1, 0, 1, 0);
		impostorVertices[35].nor = XMFLOAT4(0, 1, 0, 1);
		impostorVertices[35].tex = XMFLOAT4(stepX*6, 0, 0, 0);


		Vertex_POS impostorVertices_POS[ARRAYSIZE(impostorVertices)];
		Vertex_TEX impostorVertices_TEX[ARRAYSIZE(impostorVertices)];
		for (int i = 0; i < ARRAYSIZE(impostorVertices); ++i)
		{
			impostorVertices_POS[i] = Vertex_POS(impostorVertices[i]);
			impostorVertices_TEX[i] = Vertex_TEX(impostorVertices[i]);
		}


		GPUBufferDesc bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = USAGE_IMMUTABLE;
		bd.BindFlags = BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		SubresourceData InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = impostorVertices_POS;
		bd.ByteWidth = sizeof(impostorVertices_POS);
		wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, &impostorVB_POS);
		InitData.pSysMem = impostorVertices_TEX;
		bd.ByteWidth = sizeof(impostorVertices_TEX);
		wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, &impostorVB_TEX);
	}
}
void Mesh::ComputeNormals(bool smooth)
{
	// Start recalculating normals:

	if (smooth)
	{
		// Compute smooth surface normals:

		// 1.) Zero normals, they will be averaged later
		for (size_t i = 0; i < vertices_FULL.size() - 1; i++)
		{
			vertices_FULL[i].nor = XMFLOAT4(0, 0, 0, 0);
		}

		// 2.) Find identical vertices by POSITION, accumulate face normals
		for (size_t i = 0; i < vertices_FULL.size() - 1; i++)
		{
			Vertex_FULL& v_search = vertices_FULL[i];

			for (size_t ind = 0; ind < indices.size() / 3; ++ind)
			{
				uint32_t i0 = indices[ind * 3 + 0];
				uint32_t i1 = indices[ind * 3 + 1];
				uint32_t i2 = indices[ind * 3 + 2];

				Vertex_FULL& v0 = vertices_FULL[i0];
				Vertex_FULL& v1 = vertices_FULL[i1];
				Vertex_FULL& v2 = vertices_FULL[i2];

				bool match_pos0 =
					fabs(v_search.pos.x - v0.pos.x) < FLT_EPSILON &&
					fabs(v_search.pos.y - v0.pos.y) < FLT_EPSILON &&
					fabs(v_search.pos.z - v0.pos.z) < FLT_EPSILON;

				bool match_pos1 =
					fabs(v_search.pos.x - v1.pos.x) < FLT_EPSILON &&
					fabs(v_search.pos.y - v1.pos.y) < FLT_EPSILON &&
					fabs(v_search.pos.z - v1.pos.z) < FLT_EPSILON;

				bool match_pos2 =
					fabs(v_search.pos.x - v2.pos.x) < FLT_EPSILON &&
					fabs(v_search.pos.y - v2.pos.y) < FLT_EPSILON &&
					fabs(v_search.pos.z - v2.pos.z) < FLT_EPSILON;

				if (match_pos0 || match_pos1 || match_pos2)
				{
					XMVECTOR U = XMLoadFloat4(&v2.pos) - XMLoadFloat4(&v0.pos);
					XMVECTOR V = XMLoadFloat4(&v1.pos) - XMLoadFloat4(&v0.pos);

					XMVECTOR N = XMVector3Cross(U, V);
					N = XMVector3Normalize(N);

					XMFLOAT3 normal;
					XMStoreFloat3(&normal, N);

					v_search.nor.x += normal.x;
					v_search.nor.y += normal.y;
					v_search.nor.z += normal.z;
				}

			}
		}

		// 3.) Find unique vertices by POSITION and TEXCOORD and MATERIAL and remove duplicates
		for (size_t i = 0; i < vertices_FULL.size() - 1; i++)
		{
			const Vertex_FULL& v0 = vertices_FULL[i];

			for (size_t j = i + 1; j < vertices_FULL.size(); j++)
			{
				const Vertex_FULL& v1 = vertices_FULL[j];

				bool unique_pos =
					fabs(v0.pos.x - v1.pos.x) < FLT_EPSILON &&
					fabs(v0.pos.y - v1.pos.y) < FLT_EPSILON &&
					fabs(v0.pos.z - v1.pos.z) < FLT_EPSILON;

				bool unique_tex =
					fabs(v0.tex.x - v1.tex.x) < FLT_EPSILON &&
					fabs(v0.tex.y - v1.tex.y) < FLT_EPSILON &&
					(int)v0.tex.z == (int)v1.tex.z;

				if (unique_pos && unique_tex)
				{
					for (size_t ind = 0; ind < indices.size(); ++ind)
					{
						if (indices[ind] == j)
						{
							indices[ind] = static_cast<uint32_t>(i);
						}
						else if (indices[ind] > j && indices[ind] > 0)
						{
							indices[ind]--;
						}
					}

					vertices_FULL.erase(vertices_FULL.begin() + j);
				}

			}
		}
	}
	else
	{
		// Compute hard surface normals:

		vector<uint32_t> newIndexBuffer;
		vector<Vertex_FULL> newVertexBuffer;

		for (size_t face = 0; face < indices.size() / 3; face++)
		{
			uint32_t i0 = indices[face * 3 + 0];
			uint32_t i1 = indices[face * 3 + 1];
			uint32_t i2 = indices[face * 3 + 2];

			Vertex_FULL& v0 = vertices_FULL[i0];
			Vertex_FULL& v1 = vertices_FULL[i1];
			Vertex_FULL& v2 = vertices_FULL[i2];

			XMVECTOR U = XMLoadFloat4(&v2.pos) - XMLoadFloat4(&v0.pos);
			XMVECTOR V = XMLoadFloat4(&v1.pos) - XMLoadFloat4(&v0.pos);

			XMVECTOR N = XMVector3Cross(U, V);
			N = XMVector3Normalize(N);

			XMFLOAT3 normal;
			XMStoreFloat3(&normal, N);

			v0.nor.x = normal.x;
			v0.nor.y = normal.y;
			v0.nor.z = normal.z;

			v1.nor.x = normal.x;
			v1.nor.y = normal.y;
			v1.nor.z = normal.z;

			v2.nor.x = normal.x;
			v2.nor.y = normal.y;
			v2.nor.z = normal.z;

			newVertexBuffer.push_back(v0);
			newVertexBuffer.push_back(v1);
			newVertexBuffer.push_back(v2);

			newIndexBuffer.push_back(static_cast<uint32_t>(newIndexBuffer.size()));
			newIndexBuffer.push_back(static_cast<uint32_t>(newIndexBuffer.size()));
			newIndexBuffer.push_back(static_cast<uint32_t>(newIndexBuffer.size()));
		}

		// For hard surface normals, we created a new mesh in the previous loop through faces, so swap data:
		vertices_FULL = newVertexBuffer;
		indices = newIndexBuffer;
	}

	// force recreate:
	renderDataComplete = false;
	CreateRenderData();
}
void Mesh::FlipCulling()
{
	for (size_t face = 0; face < indices.size() / 3; face++)
	{
		uint32_t i0 = indices[face * 3 + 0];
		uint32_t i1 = indices[face * 3 + 1];
		uint32_t i2 = indices[face * 3 + 2];

		indices[face * 3 + 0] = i0;
		indices[face * 3 + 1] = i2;
		indices[face * 3 + 2] = i1;
	}

	renderDataComplete = false;
	CreateRenderData();
}
void Mesh::FlipNormals()
{
	for (size_t i = 0; i < vertices_FULL.size() - 1; i++)
	{
		Vertex_FULL& v0 = vertices_FULL[i];

		v0.nor.x *= -1;
		v0.nor.y *= -1;
		v0.nor.z *= -1;
	}

	renderDataComplete = false;
	CreateRenderData();
}

int Mesh::GetRenderTypes() const
{
	int retVal = RENDERTYPE::RENDERTYPE_VOID;
	if (renderable)
	{
		for (auto& x : subsets)
		{
			retVal |= x.material->GetRenderType();
		}
	}
	return retVal;
}
void Mesh::Serialize(wiArchive& archive)
{
	if (archive.IsReadMode())
	{
		archive >> name;
		archive >> parent;

		// vertices
		{
			size_t vertexCount;
			archive >> vertexCount;
			vertices_FULL.resize(vertexCount);
			for (size_t i = 0; i < vertexCount; ++i)
			{
				archive >> vertices_FULL[i].pos;
				archive >> vertices_FULL[i].nor;
				archive >> vertices_FULL[i].tex;
				archive >> vertices_FULL[i].ind;
				archive >> vertices_FULL[i].wei;

				if (archive.GetVersion() < 8)
				{
					vertices_FULL[i].pos.w = vertices_FULL[i].tex.w;
				}
			}
		}
		// indices
		{
			size_t indexCount;
			archive >> indexCount;
			unsigned int tempInd;
			for (size_t i = 0; i < indexCount; ++i)
			{
				archive >> tempInd;
				indices.push_back(tempInd);
			}
		}
		// physicsVerts
		{
			size_t physicsVertCount;
			archive >> physicsVertCount;
			XMFLOAT3 tempPhysicsVert;
			for (size_t i = 0; i < physicsVertCount; ++i)
			{
				archive >> tempPhysicsVert;
				physicsverts.push_back(tempPhysicsVert);
			}
		}
		// physicsindices
		{
			size_t physicsIndexCount;
			archive >> physicsIndexCount;
			unsigned int tempInd;
			for (size_t i = 0; i < physicsIndexCount; ++i)
			{
				archive >> tempInd;
				physicsindices.push_back(tempInd);
			}
		}
		// physicalmapGP
		{
			size_t physicalmapGPCount;
			archive >> physicalmapGPCount;
			int tempInd;
			for (size_t i = 0; i < physicalmapGPCount; ++i)
			{
				archive >> tempInd;
				physicalmapGP.push_back(tempInd);
			}
		}
		// subsets
		{
			size_t subsetCount;
			archive >> subsetCount;
			for (size_t i = 0; i < subsetCount; ++i)
			{
				subsets.push_back(MeshSubset());
			}
		}
		// vertexGroups
		{
			size_t groupCount;
			archive >> groupCount;
			for (size_t i = 0; i < groupCount; ++i)
			{
				VertexGroup group = VertexGroup();
				group.Serialize(archive);
				vertexGroups.push_back(group);
			}
		}
		// materialNames
		{
			size_t materialNameCount;
			archive >> materialNameCount;
			string tempString;
			for (size_t i = 0; i < materialNameCount; ++i)
			{
				archive >> tempString;
				materialNames.push_back(tempString);
			}
		}
		archive >> renderable;
		archive >> doubleSided;
		int temp;
		archive >> temp;
		//engineStencilRef = (STENCILREF)temp;
		archive >> calculatedAO;
		archive >> trailInfo.base;
		archive >> trailInfo.tip;
		archive >> isBillboarded;
		archive >> billboardAxis;
		archive >> softBody;
		archive >> mass;
		archive >> friction;
		archive >> massVG;
		archive >> goalVG;
		archive >> softVG;
		archive >> armatureName;
		aabb.Serialize(archive);

		if (archive.GetVersion() >= 7)
		{
			archive >> impostorDistance;
			archive >> tessellationFactor;
			archive >> optimized;
		}
	}
	else
	{
		archive << name;
		archive << parent;

		// vertices
		{
			archive << vertices_FULL.size();
			for (size_t i = 0; i < vertices_FULL.size(); ++i)
			{
				archive << vertices_FULL[i].pos;
				archive << vertices_FULL[i].nor;
				archive << vertices_FULL[i].tex;
				archive << vertices_FULL[i].ind;
				archive << vertices_FULL[i].wei;
			}
		}
		// indices
		{
			archive << indices.size();
			for (auto& x : indices)
			{
				archive << x;
			}
		}
		// physicsverts
		{
			archive << physicsverts.size();
			for (auto& x : physicsverts)
			{
				archive << x;
			}
		}
		// physicsindices
		{
			archive << physicsindices.size();
			for (auto& x : physicsindices)
			{
				archive << x;
			}
		}
		// physicalmapGP
		{
			archive << physicalmapGP.size();
			for (auto& x : physicalmapGP)
			{
				archive << x;
			}
		}
		// subsets
		{
			archive << subsets.size();
		}
		// vertexGroups
		{
			archive << vertexGroups.size();
			for (auto& x : vertexGroups)
			{
				x.Serialize(archive);
			}
		}
		// materialNames
		{
			archive << materialNames.size();
			for (auto& x : materialNames)
			{
				archive << x;
			}
		}
		archive << renderable;
		archive << doubleSided;
		archive << (int)0/*engineStencilRef*/;
		archive << calculatedAO;
		archive << trailInfo.base;
		archive << trailInfo.tip;
		archive << isBillboarded;
		archive << billboardAxis;
		archive << softBody;
		archive << mass;
		archive << friction;
		archive << massVG;
		archive << goalVG;
		archive << softVG;
		archive << armatureName;
		aabb.Serialize(archive);

		if (archive.GetVersion() >= 7)
		{
			archive << impostorDistance;
			archive << tessellationFactor;
			archive << optimized;
		}
	}
}
#pragma endregion

#pragma region MODEL
Model::Model()
{

}
Model::~Model()
{
	CleanUp();
}
void Model::CleanUp()
{
	for (Armature* x : armatures)
	{
		SAFE_DELETE(x);
	}
	armatures.clear();
	for (Object* x : objects)
	{
		SAFE_DELETE(x);
	}
	objects.clear();
	for (Light* x : lights)
	{
		SAFE_DELETE(x);
	}
	lights.clear();
	for (Decal* x : decals)
	{
		SAFE_DELETE(x);
	}
	decals.clear();
	for (ForceField* x : forces)
	{
		SAFE_DELETE(x);
	}
	forces.clear();
	for (EnvironmentProbe* x : environmentProbes)
	{
		SAFE_DELETE(x);
	}
	environmentProbes.clear();
}
void Model::LoadFromDisk(const std::string& fileName)
{
	string directory, name;
	wiHelper::SplitPath(fileName, directory, name);
	string extension = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(name));
	wiHelper::RemoveExtensionFromFileName(name);

	if (!extension.compare("WIMF"))
	{
		wiArchive archive(fileName, true);
		if (archive.IsOpen())
		{
			this->Serialize(archive);
		}
		else
		{
			wiHelper::messageBox("Could not open archive!", "Error!");
		}
	}
	else if (!extension.compare("OBJ"))
	{
		tinyobj::attrib_t obj_attrib;
		vector<tinyobj::shape_t> obj_shapes;
		vector<tinyobj::material_t> obj_materials;
		string obj_errors;

		bool success = tinyobj::LoadObj(&obj_attrib, &obj_shapes, &obj_materials, &obj_errors, fileName.c_str(), directory.c_str(), true);

		if (success)
		{
			this->name = name;

			// Load material library:
			vector<Material*> materialLibrary = {};
			for (auto& obj_material : obj_materials)
			{
				Material* material = new Material(obj_material.name);

				material->diffuseColor = XMFLOAT3(obj_material.diffuse[0], obj_material.diffuse[1], obj_material.diffuse[2]);
				material->textureName = obj_material.diffuse_texname;
				material->displacementMapName = obj_material.displacement_texname;
				if (material->displacementMapName.empty())
				{
					material->displacementMapName = obj_material.bump_texname;
				}
				material->emissive = max(obj_material.emission[0], max(obj_material.emission[1], obj_material.emission[2]));
				//obj_material.emissive_texname;
				material->refractionIndex = obj_material.ior;
				material->metalness = obj_material.metallic;
				//obj_material.metallic_texname;
				material->normalMapName = obj_material.normal_texname;
				material->surfaceMapName = obj_material.reflection_texname;
				material->roughness = obj_material.roughness;
				//obj_material.roughness_texname;
				material->specular_power = (int)obj_material.shininess;
				material->specular = XMFLOAT4(obj_material.specular[0], obj_material.specular[1], obj_material.specular[2], 1);
				material->specularMapName = obj_material.specular_texname;

				if (!material->surfaceMapName.empty())
				{
					material->surfaceMapName = directory + material->surfaceMapName;
					material->surfaceMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material->surfaceMapName);
				}
				if (!material->textureName.empty())
				{
					material->textureName = directory + material->textureName;
					material->texture = (Texture2D*)wiResourceManager::GetGlobal()->add(material->textureName);
				}
				if (!material->normalMapName.empty())
				{
					material->normalMapName = directory + material->normalMapName;
					material->normalMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material->normalMapName);
				}
				if (!material->displacementMapName.empty())
				{
					material->displacementMapName = directory + material->displacementMapName;
					material->displacementMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material->displacementMapName);
				}
				if (!material->specularMapName.empty())
				{
					material->specularMapName = directory + material->specularMapName;
					material->specularMap = (Texture2D*)wiResourceManager::GetGlobal()->add(material->specularMapName);
				}

				material->ConvertToPhysicallyBasedMaterial();

				materialLibrary.push_back(material); // for subset-indexing...
				this->materials.insert(make_pair(material->name, material));
			}

			if (materialLibrary.empty())
			{
				// Create default material if nothing was found:
				Material* material = new Material("OBJImport_defaultMaterial");
				materialLibrary.push_back(material);
				this->materials.insert(make_pair(material->name, material));
			}

			// Load objects, meshes:
			for (auto& shape : obj_shapes)
			{
				Object* object = new Object(shape.name);
				Mesh* mesh = new Mesh(shape.name + "_mesh");

				object->mesh = mesh;
				mesh->renderable = true;

				XMFLOAT3 min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
				XMFLOAT3 max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

				unordered_map<int, int> registered_materialIndices = {};
				unordered_map<size_t, uint32_t> uniqueVertices = {};

				for (size_t i = 0; i < shape.mesh.indices.size(); i += 3)
				{
					tinyobj::index_t reordered_indices[] = {
						shape.mesh.indices[i + 0],
						shape.mesh.indices[i + 1],
						shape.mesh.indices[i + 2],
					};

					// todo: option param would be better
					bool flipCulling = false;
					if (flipCulling)
					{
						reordered_indices[1] = shape.mesh.indices[i + 2];
						reordered_indices[2] = shape.mesh.indices[i + 1];
					}

					for (auto& index : reordered_indices)
					{
						Mesh::Vertex_FULL vert;

						vert.pos = XMFLOAT4(
							obj_attrib.vertices[index.vertex_index * 3 + 0],
							obj_attrib.vertices[index.vertex_index * 3 + 1],
							obj_attrib.vertices[index.vertex_index * 3 + 2],
							0
						);

						if (!obj_attrib.normals.empty())
						{
							vert.nor = XMFLOAT4(
								obj_attrib.normals[index.normal_index * 3 + 0],
								obj_attrib.normals[index.normal_index * 3 + 1],
								obj_attrib.normals[index.normal_index * 3 + 2],
								0
							);
						}

						if (index.texcoord_index >= 0 && !obj_attrib.texcoords.empty())
						{
							vert.tex = XMFLOAT4(
								obj_attrib.texcoords[index.texcoord_index * 2 + 0],
								1 - obj_attrib.texcoords[index.texcoord_index * 2 + 1],
								0, 0
							);
						}

						int materialIndex = max(0, shape.mesh.material_ids[i / 3]); // this indexes the material library
						if (registered_materialIndices.count(materialIndex) == 0)
						{
							registered_materialIndices[materialIndex] = (int)mesh->subsets.size();
							mesh->subsets.push_back(MeshSubset());
							Material* material = materialLibrary[materialIndex];
							mesh->subsets.back().material = material;
							mesh->materialNames.push_back(material->name);
						}
						vert.tex.z = (float)registered_materialIndices[materialIndex]; // this indexes a mesh subset

						// todo: option parameter would be better
						const bool flipZ = true;
						if (flipZ)
						{
							vert.pos.z *= -1;
							vert.nor.z *= -1;
						}

						// eliminate duplicate vertices by means of hashing:
						size_t hashes[] = {
							hash<int>{}(index.vertex_index),
							hash<int>{}(index.normal_index),
							hash<int>{}(index.texcoord_index),
							hash<int>{}(materialIndex),
						};
						size_t vertexHash = (((hashes[0] ^ (hashes[1] << 1) >> 1) ^ (hashes[2] << 1)) >> 1) ^ (hashes[3] << 1);

						if (uniqueVertices.count(vertexHash) == 0)
						{
							uniqueVertices[vertexHash] = (uint32_t)mesh->vertices_FULL.size();
							mesh->vertices_FULL.push_back(vert);
						}
						mesh->indices.push_back(uniqueVertices[vertexHash]);

						min = wiMath::Min(min, XMFLOAT3(vert.pos.x, vert.pos.y, vert.pos.z));
						max = wiMath::Max(max, XMFLOAT3(vert.pos.x, vert.pos.y, vert.pos.z));
					}
				}
				mesh->aabb.create(min, max);

				// We need to eliminate colliding mesh names, because objects can reference them by names:
				//	Note: in engine, object is decoupled from mesh, for instancing support. OBJ file have only meshes and names can collide there.
				string meshName = mesh->name;
				uint32_t unique_counter = 0;
				bool meshNameCollision = this->meshes.count(meshName) != 0;
				while (meshNameCollision)
				{
					meshName = mesh->name + to_string(unique_counter);
					meshNameCollision = this->meshes.count(meshName) != 0;
					unique_counter++;
				}
				mesh->name = meshName;

				object->meshName = mesh->name;

				this->objects.insert(object);
				this->meshes.insert(make_pair(mesh->name, mesh));
			}

			this->FinishLoading();
		}

		if (!obj_errors.empty())
		{
			wiBackLog::post(obj_errors.c_str());
		}
	}
	else
	{
		// Old Importer

		stringstream armatureFilePath(""), materialLibFilePath(""), meshesFilePath(""), objectsFilePath("")
			, actionsFilePath(""), lightsFilePath(""), decalsFilePath(""), camerasFilePath("");

		armatureFilePath << name << ".wia";
		materialLibFilePath << name << ".wim";
		meshesFilePath << name << ".wi";
		objectsFilePath << name << ".wio";
		actionsFilePath << name << ".wiact";
		lightsFilePath << name << ".wil";
		decalsFilePath << name << ".wid";
		camerasFilePath << name << ".wic";

		LoadWiArmatures(directory, armatureFilePath.str(), armatures);
		LoadWiMaterialLibrary(directory, materialLibFilePath.str(), "textures/", materials);
		LoadWiMeshes(directory, meshesFilePath.str(), meshes, armatures, materials);
		LoadWiObjects(directory, objectsFilePath.str(), objects, armatures, meshes, materials);
		LoadWiActions(directory, actionsFilePath.str(), armatures);
		LoadWiLights(directory, lightsFilePath.str(), lights);
		LoadWiDecals(directory, decalsFilePath.str(), "textures/", decals);
		LoadWiCameras(directory, camerasFilePath.str(), cameras, armatures);

		FinishLoading();
	}
}
void Model::FinishLoading()
{
	std::vector<Transform*> transforms(0);
	transforms.reserve(armatures.size() + objects.size() + lights.size() + decals.size());

	for (Armature* x : armatures)
	{
		if (x->actions.size() > 1)
		{
			// If it has actions besides the identity, activate the first by default
			x->GetPrimaryAnimation()->ChangeAction(1);
		}
		transforms.push_back(x);
		x->parentModel = this;
	}
	for (Object* x : objects) {
		transforms.push_back(x);
		x->parentModel = this;
	}
	for (Light* x : lights)
	{
		transforms.push_back(x);
		x->parentModel = this;
	}
	for (Decal* x : decals)
	{
		transforms.push_back(x);
		x->parentModel = this;
	}
	for (EnvironmentProbe* x : environmentProbes)
	{
		transforms.push_back(x);
		x->parentModel = this;
	}
	for (ForceField* x : forces)
	{
		transforms.push_back(x);
		x->parentModel = this;
	}
	for (Camera* x : cameras)
	{
		transforms.push_back(x);
		x->parentModel = this;
	}


	// Match loaded parenting information
	for (Transform* x : transforms)
	{
		if (x->parent == nullptr)
		{
			for (Transform* y : transforms)
			{
				if (x != y && !x->parentName.empty() && !x->parentName.compare(y->name))
				{
					Transform* parent = y;
					string parentName = parent->name;
					if (!x->boneParent.empty())
					{
						Armature* armature = dynamic_cast<Armature*>(y);
						if (armature != nullptr)
						{
							for (Bone* bone : armature->boneCollection)
							{
								if (!bone->name.compare(x->boneParent))
								{
									parent = bone;
									break;
								}
							}
						}
					}
					// Match parent
					XMFLOAT4X4 saved_parent_rest_inv = x->parent_inv_rest;
					x->attachTo(parent);
					x->parent_inv_rest = saved_parent_rest_inv;
					x->parentName = parentName; // this will ensure that the bone parenting is always resolved as armature->bone
					break;
				}
			}
		}

		// If it has still no parent, then attach to this model!
		if (x->parent == nullptr)
		{
			x->attachTo(this);
		}
	}


	// Set up Render data
	for (Object* x : objects)
	{
		if (x->mesh != nullptr)
		{
			// Ribbon trails
			if (x->mesh->trailInfo.base >= 0 && x->mesh->trailInfo.tip >= 0)
			{
				x->trailTex = wiTextureHelper::getInstance()->getTransparent();
				x->trailDistortTex = wiTextureHelper::getInstance()->getNormalMapDefault();
			}

			// Mesh renderdata setup
			x->mesh->Optimize();
			x->mesh->CreateRenderData();

			if (x->mesh->armature != nullptr)
			{
				x->mesh->armature->CreateBuffers();
			}
		}
	}
}
void Model::UpdateModel()
{
	for (auto& x : materials)
	{
		x.second->Update();
	}
	for (Armature* x : armatures)
	{
		x->UpdateArmature();
	}
	for (Object* x : objects)
	{
		x->UpdateObject();
	}
	for (Light*x : lights)
	{
		x->UpdateLight();
	}
	for (EnvironmentProbe* probe : environmentProbes)
	{
		probe->UpdateEnvProbe();
	}
	
	unordered_set<Decal*>::iterator iter = decals.begin();
	while (iter != decals.end())
	{
		Decal* decal = *iter;
		decal->UpdateDecal();
		if (decal->life>-2) {
			if (decal->life <= 0) {
				decal->detach();
				decals.erase(iter++);
				delete decal;
				continue;
			}
		}
		++iter;
	}
}
void Model::Add(Object* value)
{
	if (value != nullptr)
	{
		value->parentModel = this;
		objects.insert(value);
		if (value->mesh != nullptr)
		{
			meshes.insert(pair<string, Mesh*>(value->mesh->name, value->mesh));
			for (auto& x : value->mesh->subsets)
			{
				materials.insert(pair<string, Material*>(x.material->name, x.material));
			}
			this->Add(value->mesh->armature);
		}
	}
}
void Model::Add(Armature* value)
{
	if (value != nullptr)
	{
		value->parentModel = this;
		armatures.insert(value);
	}
}
void Model::Add(Light* value)
{
	if (value != nullptr)
	{
		value->parentModel = this;
		lights.insert(value);
	}
}
void Model::Add(Decal* value)
{
	if (value != nullptr)
	{
		value->parentModel = this;
		decals.insert(value);
	}
}
void Model::Add(ForceField* value)
{
	if (value != nullptr)
	{
		value->parentModel = this;
		forces.insert(value);
	}
}
void Model::Add(EnvironmentProbe* value)
{
	if (value != nullptr)
	{
		value->parentModel = this;
		environmentProbes.push_back(value);
	}
}
void Model::Add(Camera* value)
{
	if (value != nullptr)
	{
		value->parentModel = this;
		cameras.push_back(value);
	}
}
void Model::Add(Model* value)
{
	if (value != nullptr)
	{
		objects.insert(value->objects.begin(), value->objects.end());
		armatures.insert(value->armatures.begin(), value->armatures.end());
		decals.insert(value->decals.begin(), value->decals.end());
		lights.insert(value->lights.begin(), value->lights.end());
		meshes.insert(value->meshes.begin(), value->meshes.end());
		materials.insert(value->materials.begin(), value->materials.end());
		forces.insert(value->forces.begin(), value->forces.end());
		environmentProbes.insert(environmentProbes.end(), value->environmentProbes.begin(), value->environmentProbes.end());
		cameras.insert(cameras.end(), value->cameras.begin(), value->cameras.end());
	}
}
void Model::Serialize(wiArchive& archive)
{
	Transform::Serialize(archive);

	if (archive.IsReadMode())
	{
		size_t objectsCount, meshCount, materialCount, armaturesCount, lightsCount, decalsCount, forceCount, probeCount, cameraCount;

		archive >> objectsCount;
		for (size_t i = 0; i < objectsCount; ++i)
		{
			Object* x = new Object;
			x->Serialize(archive);
			objects.insert(x);
		}

		archive >> meshCount;
		for (size_t i = 0; i < meshCount; ++i)
		{
			Mesh* x = new Mesh;
			x->Serialize(archive);
			meshes.insert(pair<string, Mesh*>(x->name, x));
		}

		archive >> materialCount;
		for (size_t i = 0; i < materialCount; ++i)
		{
			Material* x = new Material;
			x->Serialize(archive);
			materials.insert(pair<string, Material*>(x->name, x));
		}

		archive >> armaturesCount;
		for (size_t i = 0; i < armaturesCount; ++i)
		{
			Armature* x = new Armature;
			x->Serialize(archive);
			armatures.insert(x);
		}

		archive >> lightsCount;
		for (size_t i = 0; i < lightsCount; ++i)
		{
			Light* x = new Light;
			x->Serialize(archive);
			lights.insert(x);
		}

		archive >> decalsCount;
		for (size_t i = 0; i < decalsCount; ++i)
		{
			Decal* x = new Decal;
			x->Serialize(archive);
			decals.insert(x);
		}

		if (archive.GetVersion() >= 10)
		{
			archive >> forceCount;
			for (size_t i = 0; i < forceCount; ++i)
			{
				ForceField* x = new ForceField;
				x->Serialize(archive);
				forces.insert(x);
			}
		}

		if (archive.GetVersion() >= 16)
		{
			archive >> probeCount;
			for (size_t i = 0; i < probeCount; ++i)
			{
				EnvironmentProbe* x = new EnvironmentProbe;
				x->Serialize(archive);
				environmentProbes.push_back(x);
			}
		}

		if (archive.GetVersion() >= 20)
		{
			archive >> cameraCount;
			for (size_t i = 0; i < cameraCount; ++i)
			{
				Camera* x = new Camera;
				x->Serialize(archive);
				cameras.push_back(x);
			}
		}

		// RESOLVE CONNECTIONS
		for (Object* x : objects)
		{
			if (x->mesh == nullptr)
			{
				// find mesh
				MeshCollection::iterator found = meshes.find(x->meshName);
				if (found != meshes.end())
				{
					x->mesh = found->second;
				}
			}
			if (x->mesh != nullptr)
			{
				// find materials for mesh subsets
				int i = 0;
				for (auto& y : x->mesh->subsets)
				{
					if (y.material == nullptr)
					{
						MaterialCollection::iterator it = materials.find(x->mesh->materialNames[i]);
						if (it != materials.end())
						{
							y.material = it->second;
						}
					}
					i++;
				}
				// find armature
				if (!x->mesh->armatureName.empty())
				{
					for (auto& y : armatures)
					{
						if (!y->name.compare(x->mesh->armatureName))
						{
							x->mesh->armature = y;
							break;
						}
					}
				}
			}
			// link particlesystems
			for (auto& y : x->eParticleSystems)
			{
				y->object = x;
				MaterialCollection::iterator it = materials.find(y->materialName);
				if (it != materials.end())
				{
					y->material = it->second;
				}
			}
			for (auto& y : x->hParticleSystems)
			{
				y->object = x;
				MaterialCollection::iterator it = materials.find(y->materialName);
				if (it != materials.end())
				{
					y->material = it->second;
					y->Generate();
				}
			}
		}

		FinishLoading();
	}
	else
	{
		archive << objects.size();
		for (auto& x : objects)
		{
			x->Serialize(archive);
		}

		archive << meshes.size();
		for (auto& x : meshes)
		{
			x.second->Serialize(archive);
		}

		archive << materials.size();
		for (auto& x : materials)
		{
			x.second->Serialize(archive);
		}

		archive << armatures.size();
		for (auto& x : armatures)
		{
			x->Serialize(archive);
		}

		archive << lights.size();
		for (auto& x : lights)
		{
			x->Serialize(archive);
		}

		archive << decals.size();
		for (auto& x : decals)
		{
			x->Serialize(archive);
		}

		if (archive.GetVersion() >= 10)
		{
			archive << forces.size();
			for (auto& x : forces)
			{
				x->Serialize(archive);
			}
		}

		if (archive.GetVersion() >= 16)
		{
			archive << environmentProbes.size();
			for (auto& x : environmentProbes)
			{
				x->Serialize(archive);
			}
		}

		if (archive.GetVersion() >= 20)
		{
			archive << cameras.size();
			for (auto& x : cameras)
			{
				x->Serialize(archive);
			}
		}
	}
}
#pragma endregion

#pragma region BONE
XMMATRIX Bone::getMatrix(int getTranslation, int getRotation, int getScale)
{
	
	return XMMatrixTranslation(0,0,length)*XMLoadFloat4x4(&world);
}
void Bone::UpdateTransform()
{
	//Transform::UpdateTransform();

	// Needs to be updated differently than regular Transforms

	for (Transform* child : children)
	{
		child->UpdateTransform();
	}
}
void Bone::Serialize(wiArchive& archive)
{
	Transform::Serialize(archive);

	if (archive.IsReadMode())
	{
		size_t childCount;
		archive >> childCount;
		string tempName;
		for (size_t j = 0; j < childCount; ++j)
		{
			archive >> tempName;
			childrenN.push_back(tempName);
		}
		archive >> restInv;
		size_t actionFramesCount;
		archive >> actionFramesCount;
		for (size_t i = 0; i < actionFramesCount; ++i)
		{
			ActionFrames aframes = ActionFrames();
			KeyFrame tempKeyFrame;
			size_t tempCount;
			archive >> tempCount;
			for (size_t i = 0; i < tempCount; ++i)
			{
				tempKeyFrame.Serialize(archive);
				aframes.keyframesRot.push_back(tempKeyFrame);
			}
			archive >> tempCount;
			for (size_t i = 0; i < tempCount; ++i)
			{
				tempKeyFrame.Serialize(archive);
				aframes.keyframesPos.push_back(tempKeyFrame);
			}
			archive >> tempCount;
			for (size_t i = 0; i < tempCount; ++i)
			{
				tempKeyFrame.Serialize(archive);
				aframes.keyframesSca.push_back(tempKeyFrame);
			}
			actionFrames.push_back(aframes);
		}
		archive >> recursivePose;
		archive >> recursiveRest;
		archive >> recursiveRestInv;
		archive >> length;
		archive >> connected;
	}
	else
	{
		archive << childrenN.size();
		for (auto& x : childrenN)
		{
			archive << x;
		}
		archive << restInv;
		archive << actionFrames.size();
		int i = 0;
		for (auto& x : actionFrames)
		{
			archive << x.keyframesRot.size();
			for (auto& y : x.keyframesRot)
			{
				y.Serialize(archive);
			}
			archive << x.keyframesPos.size();
			for (auto& y : x.keyframesPos)
			{
				y.Serialize(archive);
			}
			archive << x.keyframesSca.size();
			for (auto& y : x.keyframesSca)
			{
				y.Serialize(archive);
			}
		}
		archive << recursivePose;
		archive << recursiveRest;
		archive << recursiveRestInv;
		archive << length;
		archive << connected;
	}
}
#pragma endregion

#pragma region KEYFRAME
void KeyFrame::Serialize(wiArchive& archive)
{
	if (archive.IsReadMode())
	{
		archive >> data;
		archive >> frameI;
	}
	else
	{
		archive << data;
		archive << frameI;
	}
}
#pragma endregion

#pragma region ANIMATIONLAYER
AnimationLayer::AnimationLayer()
{
	name = "";

	activeAction = prevAction = 0;
	ResetAction();
	ResetActionPrev();

	playing = true;
	blendCurrentFrame = 0.0f;
	blendFrames = 0.0f;
	blendFact = 0.0f;
	currentFramePrevAction = 0.0f;
	weight = 1.0f;
	type = ANIMLAYER_TYPE_ADDITIVE;

	looped = true;
}

void AnimationLayer::ChangeAction(int actionIndex, float blendFrames, float weight)
{
	currentFramePrevAction = currentFrame;
	ResetAction();
	prevAction = activeAction;
	activeAction = actionIndex;
	this->blendFrames = blendFrames;
	blendFact = 0.0f;
	blendCurrentFrame = 0.0f;
	this->weight = weight;
}
void AnimationLayer::ResetAction()
{
	currentFrame = 1;
}
void AnimationLayer::ResetActionPrev()
{
	currentFramePrevAction = 1;
}
void AnimationLayer::PauseAction()
{
	playing = false;
}
void AnimationLayer::StopAction()
{
	ResetAction();
	PauseAction();
}
void AnimationLayer::PlayAction()
{
	playing = true;
}
void AnimationLayer::Serialize(wiArchive& archive)
{
	if (archive.IsReadMode())
	{
		archive >> name;
		archive >> blendFrames;
		archive >> blendFact;
		archive >> weight;
		int temp;
		archive >> temp;
		type = (ANIMATIONLAYER_TYPE)temp;
		archive >> looped;
	}
	else
	{
		archive << name;
		archive << blendFrames;
		archive << blendFact;
		archive << weight;
		archive << (int)type;
		archive << looped;
	}
}
#pragma endregion

#pragma region ARMATURE
Armature::~Armature()
{
	actions.clear();
	for (Bone* b : boneCollection)
	{
		SAFE_DELETE(b);
	}
	boneCollection.clear();
	rootbones.clear();
	for (auto& x : animationLayers)
	{
		SAFE_DELETE(x);
	}
	animationLayers.clear();
}
void Armature::UpdateTransform()
{
	Transform::UpdateTransform();

	// Calculate local animation frame:
	for (Bone* root : rootbones)
	{
		RecursiveBoneTransform(this, root, XMMatrixIdentity());
	}

	// Local animation to world space and attachment transform:
	XMMATRIX worldMatrix = getMatrix();
	for (Bone* bone : boneCollection)
	{
		XMMATRIX boneMatrix = XMLoadFloat4x4(&bone->world);
		boneMatrix = boneMatrix * worldMatrix;
		XMStoreFloat4x4(&bone->world, boneMatrix);

		bone->UpdateTransform();
	}
}
void Armature::UpdateArmature()
{
	for (auto& x : animationLayers)
	{
		if (x == nullptr)
			continue;

		AnimationLayer& anim = *x;

		// current action
		float cf = anim.currentFrame;
		int maxCf = 0;
		int activeAction = anim.activeAction;
		int prevAction = anim.prevAction;
		float frameInc = (anim.playing ? wiRenderer::GetGameSpeed() : 0.f);

		cf = anim.currentFrame += frameInc;
		maxCf = actions[activeAction].frameCount;
		if ((int)cf > maxCf)
		{
			if (anim.looped)
			{
				anim.ResetAction();
				cf = anim.currentFrame;
			}
			else
			{
				anim.currentFrame = cf = (float)maxCf;
			}
		}


		// prev action
		float cfPrevAction = anim.currentFramePrevAction;
		int maxCfPrevAction = actions[prevAction].frameCount;
		cfPrevAction = anim.currentFramePrevAction += frameInc;
		if ((int)cfPrevAction > maxCfPrevAction)
		{
			if (anim.looped)
			{
				anim.ResetActionPrev();
				cfPrevAction = anim.currentFramePrevAction;
			}
			else
			{
				anim.currentFramePrevAction = cfPrevAction = (float)maxCfPrevAction;
			}
		}

		// blending
		anim.blendCurrentFrame += frameInc;
		if (abs(anim.blendFrames) > 0)
			anim.blendFact = wiMath::Clamp(anim.blendCurrentFrame / anim.blendFrames, 0, 1);
		else
			anim.blendFact = 1;
	}
}
void Armature::RecursiveBoneTransform(Armature* armature, Bone* bone, const XMMATRIX& parentCombinedMat)
{
	Bone* parent = (Bone*)bone->parent;

	// TRANSITION BLENDING + ADDITIVE BLENDING
	XMVECTOR& finalTrans = XMVectorSet(0, 0, 0, 0);
	XMVECTOR& finalRotat = XMQuaternionIdentity();
	XMVECTOR& finalScala = XMVectorSet(1, 1, 1, 0);

	for (auto& x : armature->animationLayers)
	{
		AnimationLayer& anim = *x;

		float cf = anim.currentFrame, cfPrev = anim.currentFramePrevAction;
		int activeAction = anim.activeAction, prevAction = anim.prevAction;
		int maxCf = armature->actions[activeAction].frameCount, maxCfPrev = armature->actions[prevAction].frameCount;

		XMVECTOR& prevTrans = InterPolateKeyFrames(cfPrev, maxCfPrev, bone->actionFrames[prevAction].keyframesPos, POSITIONKEYFRAMETYPE);
		XMVECTOR& prevRotat = InterPolateKeyFrames(cfPrev, maxCfPrev, bone->actionFrames[prevAction].keyframesRot, ROTATIONKEYFRAMETYPE);
		XMVECTOR& prevScala = InterPolateKeyFrames(cfPrev, maxCfPrev, bone->actionFrames[prevAction].keyframesSca, SCALARKEYFRAMETYPE);

		XMVECTOR& currTrans = InterPolateKeyFrames(cf, maxCf, bone->actionFrames[activeAction].keyframesPos, POSITIONKEYFRAMETYPE);
		XMVECTOR& currRotat = InterPolateKeyFrames(cf, maxCf, bone->actionFrames[activeAction].keyframesRot, ROTATIONKEYFRAMETYPE);
		XMVECTOR& currScala = InterPolateKeyFrames(cf, maxCf, bone->actionFrames[activeAction].keyframesSca, SCALARKEYFRAMETYPE);

		float blendFact = anim.blendFact;

		switch (anim.type)
		{
		case AnimationLayer::ANIMLAYER_TYPE_PRIMARY:
			finalTrans = XMVectorLerp(prevTrans, currTrans, blendFact);
			finalRotat = XMQuaternionSlerp(prevRotat, currRotat, blendFact);
			finalScala = XMVectorLerp(prevScala, currScala, blendFact);
			break;
		case AnimationLayer::ANIMLAYER_TYPE_ADDITIVE:
			finalTrans = XMVectorLerp(finalTrans, XMVectorAdd(finalTrans, XMVectorLerp(prevTrans, currTrans, blendFact)), anim.weight);
			finalRotat = XMQuaternionSlerp(finalRotat, XMQuaternionMultiply(finalRotat, XMQuaternionSlerp(prevRotat, currRotat, blendFact)), anim.weight); // normalize?
			finalScala = XMVectorLerp(finalScala, XMVectorMultiply(finalScala, XMVectorLerp(prevScala, currScala, blendFact)), anim.weight);
			break;
		default:
			break;
		}
	}
	XMVectorSetW(finalTrans, 1);
	XMVectorSetW(finalScala, 1);

	bone->worldPrev = bone->world;
	bone->translationPrev = bone->translation;
	bone->rotationPrev = bone->rotation;
	XMStoreFloat3(&bone->translation, finalTrans);
	XMStoreFloat4(&bone->rotation, finalRotat);
	XMStoreFloat3(&bone->scale, finalScala);

	XMMATRIX& anim =
		XMMatrixScalingFromVector(finalScala)
		* XMMatrixRotationQuaternion(finalRotat)
		* XMMatrixTranslationFromVector(finalTrans);

	XMMATRIX& rest =
		XMLoadFloat4x4(&bone->world_rest);

	XMMATRIX& boneMat =
		anim * rest * parentCombinedMat
		;

	XMMATRIX& finalMat =
		XMLoadFloat4x4(&bone->recursiveRestInv)*
		boneMat
		;

	XMStoreFloat4x4(&bone->world, boneMat);

	XMStoreFloat4x4(&bone->boneRelativity, finalMat);

	for (unsigned int i = 0; i<bone->childrenI.size(); ++i) {
		RecursiveBoneTransform(armature, bone->childrenI[i], boneMat);
	}
}
XMVECTOR Armature::InterPolateKeyFrames(float cf, const int maxCf, const std::vector<KeyFrame>& keyframeList, KeyFrameType type)
{
	XMVECTOR result = XMVectorSet(0, 0, 0, 0);

	if (type == POSITIONKEYFRAMETYPE) result = XMVectorSet(0, 0, 0, 1);
	if (type == ROTATIONKEYFRAMETYPE) result = XMVectorSet(0, 0, 0, 1);
	if (type == SCALARKEYFRAMETYPE)   result = XMVectorSet(1, 1, 1, 1);

	//SEARCH 2 INTERPOLATABLE FRAMES
	int nearest[2] = { 0,0 };
	int first = 0, last = 0;
	if (keyframeList.size()>1) {
		first = keyframeList[0].frameI;
		last = keyframeList.back().frameI;

		if (cf <= first) { //BROKEN INTERVAL
			nearest[0] = 0;
			nearest[1] = 0;
		}
		else if (cf >= last) {
			nearest[0] = (int)(keyframeList.size() - 1);
			nearest[1] = (int)(keyframeList.size() - 1);
		}
		else { //IN BETWEEN TWO KEYFRAMES, DECIDE WHICH
			for (int k = (int)keyframeList.size() - 1; k>0; k--)
				if (keyframeList[k].frameI <= cf) {
					nearest[0] = k;
					break;
				}
			for (int k = 0; k<(int)keyframeList.size(); k++)
				if (keyframeList[k].frameI >= cf) {
					nearest[1] = k;
					break;
				}
		}

		//INTERPOLATE BETWEEN THE TWO FRAMES
		int keyframes[2] = {
			keyframeList[nearest[0]].frameI,
			keyframeList[nearest[1]].frameI
		};
		float interframe = 0;
		if (cf <= first || cf >= last) { //BROKEN INTERVAL
			float intervalBegin = (float)(maxCf - keyframes[0]);
			float intervalEnd = keyframes[1] + intervalBegin;
			float intervalLen = abs(intervalEnd - intervalBegin);
			float offsetCf = cf + intervalBegin;
			if (intervalLen) interframe = offsetCf / intervalLen;
		}
		else {
			float intervalBegin = (float)keyframes[0];
			float intervalEnd = (float)keyframes[1];
			float intervalLen = abs(intervalEnd - intervalBegin);
			float offsetCf = cf - intervalBegin;
			if (intervalLen) interframe = offsetCf / intervalLen;
		}

		if (type == ROTATIONKEYFRAMETYPE) {
			XMVECTOR quat[2] = {
				XMLoadFloat4(&keyframeList[nearest[0]].data),
				XMLoadFloat4(&keyframeList[nearest[1]].data)
			};
			result = XMQuaternionNormalize(XMQuaternionSlerp(quat[0], quat[1], interframe));
		}
		else {
			XMVECTOR tran[2] = {
				XMLoadFloat4(&keyframeList[nearest[0]].data),
				XMLoadFloat4(&keyframeList[nearest[1]].data)
			};
			result = XMVectorLerp(tran[0], tran[1], interframe);
		}
	}
	else {
		if (!keyframeList.empty())
			result = XMLoadFloat4(&keyframeList.back().data);
	}

	return result;
}


void Armature::ChangeAction(const std::string& actionName, float blendFrames, const std::string& animLayer, float weight)
{
	AnimationLayer* anim = GetPrimaryAnimation();
	if (animLayer.length() > 0)
	{
		anim = GetAnimLayer(animLayer);
	}

	if (actionName.length() > 0)
	{
		for (unsigned int i = 1; i < actions.size(); ++i)
		{
			if (!actions[i].name.compare(actionName))
			{
				anim->ChangeAction(i, blendFrames, weight);
				return;
			}
		}
	}

	// Fall back to identity action
	anim->ChangeAction(0, blendFrames, weight);
}
AnimationLayer* Armature::GetAnimLayer(const std::string& name)
{
	for (auto& x : animationLayers)
	{
		if (!x->name.compare(name))
		{
			return x;
		}
	}
	animationLayers.push_back(new AnimationLayer);
	animationLayers.back()->name = name;
	return animationLayers.back();
}
void Armature::AddAnimLayer(const std::string& name)
{
	for (auto& x : animationLayers)
	{
		if (!x->name.compare(name))
		{
			return;
		}
	}
	animationLayers.push_back(new AnimationLayer);
	animationLayers.back()->name = name;
}
void Armature::DeleteAnimLayer(const std::string& name)
{
	auto i = animationLayers.begin();
	while (i != animationLayers.end())
	{
		if ((*i)->type != AnimationLayer::ANIMLAYER_TYPE_PRIMARY && !(*i)->name.compare(name))
		{
			animationLayers.erase(i++);
		}
		else
		{
			++i;
		}
	}
}
void Armature::RecursiveRest(Bone* bone)
{
	Bone* parent = (Bone*)bone->parent;

	if (parent != nullptr) {
		XMMATRIX recRest =
			XMLoadFloat4x4(&bone->world_rest)
			*
			XMLoadFloat4x4(&parent->recursiveRest)
			;
		XMStoreFloat4x4(&bone->recursiveRest, recRest);
		XMStoreFloat4x4(&bone->recursiveRestInv, XMMatrixInverse(0, recRest));
	}
	else {
		bone->recursiveRest = bone->world_rest;
		XMStoreFloat4x4(&bone->recursiveRestInv, XMMatrixInverse(0, XMLoadFloat4x4(&bone->recursiveRest)));
	}

	for (unsigned int i = 0; i<bone->childrenI.size(); ++i) {
		RecursiveRest(bone->childrenI[i]);
	}
}
void Armature::CreateFamily()
{
	for (Bone* i : boneCollection) {
		if (i->parentName.length()>0) {
			for (Bone* j : boneCollection) {
				if (i != j) {
					if (!i->parentName.compare(j->name)) {
						i->parent = j;
						j->childrenN.push_back(i->name);
						j->childrenI.push_back(i);
						i->attachTo(j, 1, 1, 1);
					}
				}
			}
		}
		else {
			rootbones.push_back(i);
		}
	}

	for (unsigned int i = 0; i<rootbones.size(); ++i) 
	{
		RecursiveRest(rootbones[i]);
	}
}
void Armature::CreateBuffers()
{
	boneData.resize(boneCollection.size());

	GPUBufferDesc bd;
	bd.Usage = USAGE_DYNAMIC;
	bd.CPUAccessFlags = CPU_ACCESS_WRITE;

	bd.ByteWidth = sizeof(ShaderBoneType) * (UINT)boneCollection.size();
	bd.BindFlags = BIND_SHADER_RESOURCE;
	bd.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(ShaderBoneType);
	wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &boneBuffer);
}
Bone* Armature::GetBone(const std::string& name)
{
	for (auto& x : boneCollection)
	{
		if (!x->name.compare(name))
		{
			return x;
		}
	}
	return nullptr;
}
void Armature::Serialize(wiArchive& archive)
{
	Transform::Serialize(archive);

	if (archive.IsReadMode())
	{
		string voidStr;
		archive >> voidStr; // no longer needed (it was unidentified_name previously...)
		size_t boneCount;
		archive >> boneCount;
		for (size_t i = 0; i < boneCount; ++i)
		{
			Bone* bone = new Bone;
			bone->Serialize(archive);
			boneCollection.push_back(bone);
		}
		size_t animLayerCount;
		archive >> animLayerCount;
		animationLayers.clear();
		for (size_t i = 0; i < animLayerCount; ++i)
		{
			AnimationLayer* layer = new AnimationLayer;
			layer->Serialize(archive);
			animationLayers.push_back(layer);
		}
		size_t actionCount;
		archive >> actionCount;
		Action tempAction;
		actions.clear();
		for (size_t i = 0; i < actionCount; ++i)
		{
			archive >> tempAction.name;
			archive >> tempAction.frameCount;
			actions.push_back(tempAction);
		}

		CreateFamily();
	}
	else
	{
		string voidStr = "";
		archive << voidStr; // no longer needed (it was unidentified_name previously...)
		archive << boneCollection.size();
		for (auto& x : boneCollection)
		{
			x->Serialize(archive);
		}
		archive << animationLayers.size();
		for (auto& x : animationLayers)
		{
			x->Serialize(archive);
		}
		archive << actions.size();
		for (auto& x : actions)
		{
			archive << x.name;
			archive << x.frameCount;
		}
	}
}
#pragma endregion

#pragma region Decals
Decal::Decal(const XMFLOAT3& tra, const XMFLOAT3& sca, const XMFLOAT4& rot, const std::string& tex, const std::string& nor):Cullable(),Transform(){
	scale_rest=scale=sca;
	rotation_rest=rotation=rot;
	translation_rest=translation=tra;

	UpdateTransform();

	texture=normal=nullptr;
	addTexture(tex);
	addNormal(nor);

	life = -2; //persistent
	fadeStart=0;

	atlasMulAdd = XMFLOAT4(1, 1, 0, 0);

	color = XMFLOAT4(1, 1, 1, 1);
	emissive = 0;
}
Decal::~Decal() {
	wiResourceManager::GetGlobal()->del(texName);
	wiResourceManager::GetGlobal()->del(norName);
}
void Decal::addTexture(const std::string& tex){
	texName=tex;
	if(!tex.empty()){
		texture = (Texture2D*)wiResourceManager::GetGlobal()->add(tex);
	}
}
void Decal::addNormal(const std::string& nor){
	norName=nor;
	if(!nor.empty()){
		normal = (Texture2D*)wiResourceManager::GetGlobal()->add(nor);
	}
}
void Decal::UpdateTransform()
{
	Transform::UpdateTransform();

	XMMATRIX rotMat = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
	XMVECTOR eye = XMLoadFloat3(&translation);
	XMStoreFloat4x4(&world_rest, XMMatrixScalingFromVector(XMLoadFloat3(&scale))*rotMat*XMMatrixTranslationFromVector(eye));

	bounds.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(scale.x*0.25f, scale.y*0.25f, scale.z*0.25f));
	bounds = bounds.get(XMLoadFloat4x4(&world_rest));

}
void Decal::UpdateDecal()
{
	if (life>-2) {
		life -= wiRenderer::GetGameSpeed();
	}
}
float Decal::GetOpacity() const
{
	return color.w * wiMath::Clamp((life <= -2 ? 1 : life < fadeStart ? life / fadeStart : 1), 0, 1);
}
void Decal::Serialize(wiArchive& archive)
{
	Cullable::Serialize(archive);
	Transform::Serialize(archive);

	if (archive.IsReadMode())
	{
		archive >> texName;
		archive >> norName;
		archive >> life;
		archive >> fadeStart;
		if (archive.GetVersion() >= 5)
		{
			archive >> color;
			archive >> emissive;
		}

		string texturesDir = archive.GetSourceDirectory() + "textures/";
		if (!texName.empty())
		{
			texName = texturesDir + texName;
			texture = (Texture2D*)wiResourceManager::GetGlobal()->add(texName);
		}
		if (!norName.empty())
		{
			norName = texturesDir + norName;
			normal = (Texture2D*)wiResourceManager::GetGlobal()->add(norName);
		}

	}
	else
	{
		archive << wiHelper::GetFileNameFromPath(texName);
		archive << wiHelper::GetFileNameFromPath(norName);
		archive << life;
		archive << fadeStart;
		if (archive.GetVersion() >= 5)
		{
			archive << color;
			archive << emissive;
		}
	}
}
#pragma endregion

#pragma region CAMERA
void Camera::UpdateTransform()
{
	Transform::UpdateTransform();

	UpdateProps();
}

void Camera::Lerp(const Camera* a, const Camera* b, float t)
{
	Transform::Lerp(a, b, t);

	fov = wiMath::Lerp(a->fov, b->fov, t);

	UpdateProps();
	UpdateProjection();
}
void Camera::CatmullRom(const Camera* a, const Camera* b, const Camera* c, const Camera* d, float t)
{
	Transform::CatmullRom(a, b, c, d, t);

	XMVECTOR FOV = XMVectorCatmullRom(
		XMVectorSet(a->fov, 0, 0, 0),
		XMVectorSet(b->fov, 0, 0, 0),
		XMVectorSet(c->fov, 0, 0, 0),
		XMVectorSet(d->fov, 0, 0, 0),
		t
	);
	fov = XMVectorGetX(FOV);

	UpdateProps();
	UpdateProjection();
}

void Camera::Serialize(wiArchive& archive)
{
	Transform::Serialize(archive);

	if (archive.IsReadMode())
	{
		archive >> At;
		archive >> Up;
		archive >> width;
		archive >> height;
		archive >> zNearP;
		archive >> zFarP;
		archive >> fov;
	}
	else
	{
		archive << At;
		archive << Up;
		archive << width;
		archive << height;
		archive << zNearP;
		archive << zFarP;
		archive << fov;
	}
}
#pragma endregion

#pragma region OBJECT
Object::Object(const std::string& name) :Transform()
{
	this->name = name;
	init();

}
Object::Object(const Object& other):Cullable(other),Transform(other)
{
	init();

	meshName = other.meshName;
	mesh = other.mesh;
	name = other.name;
	mass = other.mass;
	collisionShape = other.collisionShape;
	friction = other.friction;
	restitution = other.restitution;
	damping = other.damping;
	physicsType = other.physicsType;
	transparency = other.transparency;
	color = other.color;

	parentModel = other.parentModel;

	for (auto&x : other.eParticleSystems)
	{
		eParticleSystems.push_back(new wiEmittedParticle(*x));
		eParticleSystems.back()->object = this;
	}
	for (auto&x : other.hParticleSystems)
	{
		hParticleSystems.push_back(new wiHairParticle(*x));
		hParticleSystems.back()->object = this;
	}
}
Object::~Object() 
{
	for (auto&x : eParticleSystems)
	{
		SAFE_DELETE(x);
	}
	for (auto&x : hParticleSystems)
	{
		SAFE_DELETE(x);
	}
}
void Object::init()
{
	mesh = nullptr;
	trail.resize(0);
	renderable = true;
	eParticleSystems.resize(0);
	hParticleSystems.resize(0);
	rigidBody = kinematic = false;
	collisionShape = "BOX";
	mass = friction = restitution = damping = 1.0f;
	physicsType = "ACTIVE";
	physicsObjectID = -1;
	transparency = 0.0f;
	color = XMFLOAT3(1, 1, 1);
	trailDistortTex = nullptr;
	trailTex = nullptr;
	occlusionHistory = 0xFFFFFFFF;
	occlusionQueryID = -1;
}
void Object::EmitTrail(const XMFLOAT3& col, float fadeSpeed) {
	if (mesh != nullptr)
	{
		int base = mesh->trailInfo.base;
		int tip = mesh->trailInfo.tip;


		//int x = trail.size();

		if (base >= 0 && tip >= 0) {
			XMFLOAT4 baseP, tipP;
			XMFLOAT4 newCol = XMFLOAT4(col.x, col.y, col.z, 1);
			baseP = wiRenderer::TransformVertex(mesh, base).pos;
			tipP = wiRenderer::TransformVertex(mesh, tip).pos;

			trail.push_back(RibbonVertex(XMFLOAT3(baseP.x, baseP.y, baseP.z), XMFLOAT2(0,0), XMFLOAT4(0, 0, 0, 1),fadeSpeed));
			trail.push_back(RibbonVertex(XMFLOAT3(tipP.x, tipP.y, tipP.z), XMFLOAT2(0,0), newCol,fadeSpeed));
		}
	}
}
void Object::FadeTrail() {
	for (unsigned int j = 0; j<trail.size(); ++j) {
		const float fade = trail[j].fade;
		if (trail[j].col.x>0) trail[j].col.x = trail[j].col.x - fade*(wiRenderer::GetGameSpeed() + wiRenderer::GetGameSpeed()*(1 - j % 2) * 2);
		else trail[j].col.x = 0;
		if (trail[j].col.y>0) trail[j].col.y = trail[j].col.y - fade*(wiRenderer::GetGameSpeed() + wiRenderer::GetGameSpeed()*(1 - j % 2) * 2);
		else trail[j].col.y = 0;
		if (trail[j].col.z>0) trail[j].col.z = trail[j].col.z - fade*(wiRenderer::GetGameSpeed() + wiRenderer::GetGameSpeed()*(1 - j % 2) * 2);
		else trail[j].col.z = 0;
		if (trail[j].col.w>0)
			trail[j].col.w -= fade*wiRenderer::GetGameSpeed();
		else
			trail[j].col.w = 0;

#if 0
		// Collapse trail... perhaps will be needed
		if (j % 2 == 0)
		{
			trail[j].pos = wiMath::Lerp(trail[j].pos, trail[j + 1].pos, trail[j].fade * wiRenderer::GetGameSpeed());
			trail[j + 1].pos = wiMath::Lerp(trail[j + 1].pos, trail[j].pos, trail[j + 1].fade * wiRenderer::GetGameSpeed());
		}
#endif
	}
	while (!trail.empty() && trail.front().col.w <= 0)
	{
		trail.pop_front();
	}
}
void Object::UpdateTransform()
{
	Transform::UpdateTransform();

}
void Object::UpdateObject()
{
	XMMATRIX world = getMatrix();

	if (mesh->isBillboarded) {
		XMMATRIX bbMat = XMMatrixIdentity();
		if (mesh->billboardAxis.x || mesh->billboardAxis.y || mesh->billboardAxis.z) {
			float angle = 0;
			angle = (float)atan2(translation.x - wiRenderer::getCamera()->translation.x, translation.z - wiRenderer::getCamera()->translation.z) * (180.0f / XM_PI);
			bbMat = XMMatrixRotationAxis(XMLoadFloat3(&mesh->billboardAxis), angle * 0.0174532925f);
		}
		else
			bbMat = XMMatrixInverse(0, XMMatrixLookAtLH(XMVectorSet(0, 0, 0, 0), XMVectorSubtract(XMLoadFloat3(&translation), wiRenderer::getCamera()->GetEye()), XMVectorSet(0, 1, 0, 0)));

		XMMATRIX w = XMMatrixScalingFromVector(XMLoadFloat3(&scale)) *
			bbMat *
			XMMatrixRotationQuaternion(XMLoadFloat4(&rotation)) *
			XMMatrixTranslationFromVector(XMLoadFloat3(&translation)
				);
		XMStoreFloat4x4(&this->world, w);
	}

	if (mesh->softBody)
		bounds = mesh->aabb;
	else if (!mesh->isBillboarded && mesh->renderable) {
		bounds = mesh->aabb.get(world);
	}
	else if (mesh->renderable)
		bounds.createFromHalfWidth(translation, scale);

	if (!trail.empty())
	{
		wiRenderer::objectsWithTrails.insert(this);
		FadeTrail();
	}

	for (wiEmittedParticle* x : eParticleSystems)
	{
		wiRenderer::emitterSystems.insert(x);
	}
}
bool Object::IsCastingShadow() const
{
	for (auto& x : mesh->subsets)
	{
		if (x.material->IsCastingShadow())
		{
			return true;
		}
	}
	return false;
}
bool Object::IsReflector() const
{
	for (auto& x : mesh->subsets)
	{
		if (x.material->HasPlanarReflection())
		{
			return true;
		}
	}
	return false;
}
int Object::GetRenderTypes() const
{
	int retVal = RENDERTYPE::RENDERTYPE_VOID;
	if (renderable)
	{
		for (auto& x : mesh->subsets)
		{
			retVal |= x.material->GetRenderType();
		}
	}
	return retVal;
}
bool Object::IsOccluded() const
{
	// Perform a conservative occlusion test:
	// If it is visible in any frames in the history, it is determined visible in this frame
	// But if all queries failed in the history, it is occluded.
	// If it pops up for a frame after occluded, it is visible again for some frames
	return ((occlusionQueryID >= 0) && (occlusionHistory & 0xFFFFFFFF) == 0);
}
XMMATRIX Object::GetOBB() const
{
	if (mesh == nullptr)
	{
		return XMMatrixIdentity();
	}
	return mesh->aabb.getAsBoxMatrix()*XMLoadFloat4x4(&world);
}
void Object::Serialize(wiArchive& archive)
{
	Cullable::Serialize(archive);

	// backwards-compatibility: serialize [streamable] properties
	if (archive.IsReadMode())
	{
		string tempStr;
		bool tempBool;
		archive >> tempStr;
		archive >> meshName; // this is still needed!
		archive >> tempStr;
		archive >> tempBool;
	}
	else
	{
		string tempStr = "";
		bool tempBool = false;
		archive << tempStr;
		archive << meshName; // this is still needed!
		archive << tempStr;
		archive << tempBool;
	}

	Transform::Serialize(archive);

	if (archive.IsReadMode())
	{
		int unused;
		archive >> unused;
		archive >> transparency;
		archive >> color;
		archive >> rigidBody;
		archive >> kinematic;
		archive >> collisionShape;
		archive >> physicsType;
		archive >> mass;
		archive >> friction;
		archive >> restitution;
		archive >> damping;

		size_t emitterPSCount;
		archive >> emitterPSCount;
		for (size_t i = 0; i < emitterPSCount; ++i)
		{
			wiEmittedParticle* e = new wiEmittedParticle;
			e->Serialize(archive);
			eParticleSystems.push_back(e);
		}
		size_t hairPSCount;
		archive >> hairPSCount;
		for (size_t i = 0; i < hairPSCount; ++i)
		{
			wiHairParticle* h = new wiHairParticle;
			h->Serialize(archive);
			hParticleSystems.push_back(h);
		}

		if (archive.GetVersion() >= 13)
		{
			archive >> renderable;
		}
		if (archive.GetVersion() >= 19)
		{
			archive >> cascadeMask;
		}

	}
	else
	{
		archive << (int)0;
		archive << transparency;
		archive << color;
		archive << rigidBody;
		archive << kinematic;
		archive << collisionShape;
		archive << physicsType;
		archive << mass;
		archive << friction;
		archive << restitution;
		archive << damping;

		archive << eParticleSystems.size();
		for (auto& x : eParticleSystems)
		{
			x->Serialize(archive);
		}
		archive << hParticleSystems.size();
		for (auto& x : hParticleSystems)
		{
			x->Serialize(archive);
		}

		if (archive.GetVersion() >= 13)
		{
			archive << renderable;
		}
		if (archive.GetVersion() >= 19)
		{
			archive << cascadeMask;
		}

	}
}
#pragma endregion

#pragma region LIGHT
Texture2D* Light::shadowMapArray_2D = nullptr;
Texture2D* Light::shadowMapArray_Cube = nullptr;
Texture2D* Light::shadowMapArray_Transparent = nullptr;
Light::Light():Transform() {
	color = XMFLOAT4(0, 0, 0, 0);
	enerDis = XMFLOAT4(0, 0, 0, 0);
	SetType(LightType::POINT);
	shadow = false;
	noHalo = false;
	volumetrics = false;
	lensFlareRimTextures.resize(0);
	lensFlareNames.resize(0);
	shadowMap_index = -1;
	entityArray_index = 0;
	radius = 1.0f;
	width = 1.0f;
	height = 1.0f;
}
Light::~Light() {
	for (string x : lensFlareNames)
		wiResourceManager::GetGlobal()->del(x);
}
XMFLOAT3 Light::GetDirection() const
{
	XMMATRIX rot = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
	XMFLOAT3 retVal;
	XMStoreFloat3(&retVal, XMVector3Normalize(-XMVector3Transform(XMVectorSet(0, -1, 0, 1), rot)));
	return retVal;
}
float Light::GetRange() const
{
	switch (type)
	{
	case Light::DIRECTIONAL:
		return 1000000;
	case Light::POINT:
	case Light::SPOT:
		return enerDis.y;
	case Light::SPHERE:
	case Light::DISC:
		return radius * 100; // todo
	case Light::RECTANGLE:
		return max(width, height) * 100; // todo
	case Light::TUBE:
		return max(width, radius) * 100; // todo
	}
	return 0;
}
void Light::UpdateTransform()
{
	Transform::UpdateTransform();
}

void Light::UpdateLight()
{
	switch (type)
	{
		case Light::DIRECTIONAL:
		{
			if (shadow)
			{
				XMFLOAT2 screen = XMFLOAT2((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
				Camera* camera = wiRenderer::getCamera();
				float nearPlane = camera->zNearP;
				float farPlane = camera->zFarP;
				XMMATRIX view = camera->GetView();
				XMMATRIX projection = camera->GetRealProjection();
				XMMATRIX world = XMMatrixIdentity();

				// Set up three shadow cascades (far - mid - near):
				const float referenceFrustumDepth = 800.0f;									// this was the frustum depth used for reference
				const float currentFrustumDepth = farPlane - nearPlane;						// current frustum depth
				const float lerp0 = referenceFrustumDepth / currentFrustumDepth * 0.5f;		// third slice distance from cam (percentage)
				const float lerp1 = referenceFrustumDepth / currentFrustumDepth * 0.12f;	// second slice distance from cam (percentage)
				const float lerp2 = referenceFrustumDepth / currentFrustumDepth * 0.016f;	// first slice distance from cam (percentage)

				// Create shadow cascades for main camera:
				if (shadowCam_dirLight.empty())
				{
					shadowCam_dirLight.resize(3);
				}

				// Place the shadow cascades inside the viewport:
				if (!shadowCam_dirLight.empty()) 
				{
					// frustum top left - near
					XMVECTOR a0 = XMVector3Unproject(XMVectorSet(0, 0, 0, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);
					// frustum top left - far
					XMVECTOR a1 = XMVector3Unproject(XMVectorSet(0, 0, 1, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);
					// frustum bottom right - near
					XMVECTOR b0 = XMVector3Unproject(XMVectorSet(screen.x, screen.y, 0, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);
					// frustum bottom right - far
					XMVECTOR b1 = XMVector3Unproject(XMVectorSet(screen.x, screen.y, 1, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);

					// calculate cascade projection sizes:
					float size0 = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0, b1, lerp0), XMVectorLerp(a0, a1, lerp0))));
					float size1 = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0, b1, lerp1), XMVectorLerp(a0, a1, lerp1))));
					float size2 = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0, b1, lerp2), XMVectorLerp(a0, a1, lerp2))));

					XMVECTOR rotDefault = XMQuaternionIdentity();

					// create shadow cascade projections:
					shadowCam_dirLight[0] = SHCAM(size0, rotDefault, -farPlane * 0.5f, farPlane * 0.5f);
					shadowCam_dirLight[1] = SHCAM(size1, rotDefault, -farPlane * 0.5f, farPlane * 0.5f);
					shadowCam_dirLight[2] = SHCAM(size2, rotDefault, -farPlane * 0.5f, farPlane * 0.5f);

					// frustum center - near
					XMVECTOR c = XMVector3Unproject(XMVectorSet(screen.x * 0.5f, screen.y * 0.5f, 0, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);
					// frustum center - far
					XMVECTOR d = XMVector3Unproject(XMVectorSet(screen.x * 0.5f, screen.y * 0.5f, 1, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);

					// Avoid shadowmap texel swimming by aligning them to a discrete grid:
					float f0 = shadowCam_dirLight[0].size / (float)wiRenderer::SHADOWRES_2D;
					float f1 = shadowCam_dirLight[1].size / (float)wiRenderer::SHADOWRES_2D;
					float f2 = shadowCam_dirLight[2].size / (float)wiRenderer::SHADOWRES_2D;
					XMVECTOR e0 = XMVectorFloor(XMVectorLerp(c, d, lerp0) / f0) * f0;
					XMVECTOR e1 = XMVectorFloor(XMVectorLerp(c, d, lerp1) / f1) * f1;
					XMVECTOR e2 = XMVectorFloor(XMVectorLerp(c, d, lerp2) / f2) * f2;

					XMMATRIX rrr = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));

					shadowCam_dirLight[0].Update(rrr, e0);
					shadowCam_dirLight[1].Update(rrr, e1);
					shadowCam_dirLight[2].Update(rrr, e2);
				}
			}

			bounds.createFromHalfWidth(wiRenderer::getCamera()->translation, XMFLOAT3(10000, 10000, 10000));
		}
		break;
		case Light::SPOT:
		{
			if (shadow)
			{
				if (shadowCam_spotLight.empty())
				{
					shadowCam_spotLight.push_back(SHCAM(XMFLOAT4(0, 0, 0, 1), 0.1f, 1000.0f, enerDis.z));
				}
				shadowCam_spotLight[0].Update(XMLoadFloat4x4(&world));
				shadowCam_spotLight[0].farplane = enerDis.y;
				shadowCam_spotLight[0].Create_Perspective(enerDis.z);
			}

			bounds.createFromHalfWidth(translation, XMFLOAT3(enerDis.y, enerDis.y, enerDis.y));
		}
		break;
		case Light::POINT:
		case Light::SPHERE:
		case Light::DISC:
		case Light::RECTANGLE:
		case Light::TUBE:
		{
			if (shadow)
			{
				if (shadowCam_pointLight.empty())
				{
					shadowCam_pointLight.push_back(SHCAM(XMFLOAT4(0.5f, -0.5f, -0.5f, -0.5f), 0.1f, 1000.0f, XM_PIDIV2)); //+x
					shadowCam_pointLight.push_back(SHCAM(XMFLOAT4(0.5f, 0.5f, 0.5f, -0.5f), 0.1f, 1000.0f, XM_PIDIV2)); //-x

					shadowCam_pointLight.push_back(SHCAM(XMFLOAT4(1, 0, 0, -0), 0.1f, 1000.0f, XM_PIDIV2)); //+y
					shadowCam_pointLight.push_back(SHCAM(XMFLOAT4(0, 0, 0, -1), 0.1f, 1000.0f, XM_PIDIV2)); //-y

					shadowCam_pointLight.push_back(SHCAM(XMFLOAT4(0.707f, 0, 0, -0.707f), 0.1f, 1000.0f, XM_PIDIV2)); //+z
					shadowCam_pointLight.push_back(SHCAM(XMFLOAT4(0, 0.707f, 0.707f, 0), 0.1f, 1000.0f, XM_PIDIV2)); //-z
				}
				for (unsigned int i = 0; i < shadowCam_pointLight.size(); ++i) {
					shadowCam_pointLight[i].Update(XMLoadFloat3(&translation));
					shadowCam_pointLight[i].farplane = max(1.0f, enerDis.y);
					shadowCam_pointLight[i].Create_Perspective(XM_PIDIV2);
				}
			}

			if (type == Light::POINT)
			{
				bounds.createFromHalfWidth(translation, XMFLOAT3(enerDis.y, enerDis.y, enerDis.y));
			}
			else
			{
				// area lights have no bounds like directional lights
				bounds.createFromHalfWidth(wiRenderer::getCamera()->translation, XMFLOAT3(10000, 10000, 10000));
			}
		}
		break;
	}
}
void Light::SetType(LightType type)
{
	this->type = type;
	switch (type)
	{
	case Light::DIRECTIONAL:
	case Light::SPOT:
		shadowBias = 0.0001f;
		break;
	case Light::POINT:
	case Light::SPHERE:
	case Light::DISC:
	case Light::RECTANGLE:
	case Light::TUBE:
	case Light::LIGHTTYPE_COUNT:
		shadowBias = 0.1f;
		break;
	}
	UpdateLight();
}
void Light::Serialize(wiArchive& archive)
{
	Cullable::Serialize(archive);
	Transform::Serialize(archive);

	if (archive.IsReadMode())
	{
		archive >> color;
		archive >> enerDis;
		archive >> noHalo;
		archive >> shadow;
		archive >> shadowBias;
		int temp;
		archive >> temp;
		type = (LightType)temp;
		if (archive.GetVersion() < 6 && type == POINT)
		{
			// the shadow bias for point lights was faulty
			shadowBias = 0.1f;
		}
		size_t lensFlareCount;
		archive >> lensFlareCount;
		string rim;
		for (size_t i = 0; i < lensFlareCount; ++i)
		{
			archive >> rim;
			Texture2D* tex;
			rim = archive.GetSourceDirectory() + "rims/" + rim;
			if (!rim.empty() && (tex = (Texture2D*)wiResourceManager::GetGlobal()->add(rim)) != nullptr) {
				lensFlareRimTextures.push_back(tex);
				lensFlareNames.push_back(rim);
			}
		}

		if (archive.GetVersion() >= 6)
		{
			archive >> radius;
			archive >> width;
			archive >> height;
		}
		if (archive.GetVersion() >= 17)
		{
			archive >> volumetrics;
		}
	}
	else
	{
		archive << color;
		archive << enerDis;
		archive << noHalo;
		archive << shadow;
		archive << shadowBias;
		archive << (int)type;
		archive << lensFlareNames.size();
		for (auto& x : lensFlareNames)
		{
			archive << wiHelper::GetFileNameFromPath(x);
		}

		if (archive.GetVersion() >= 6)
		{
			archive << radius;
			archive << width;
			archive << height;
		}
		if (archive.GetVersion() >= 17)
		{
			archive << volumetrics;
		}
	}
}
#pragma endregion

#pragma region FORCEFIELD
void ForceField::Serialize(wiArchive& archive)
{
	Transform::Serialize(archive);

	if (archive.IsReadMode())
	{
		archive >> type;
		archive >> gravity;
		archive >> range;
	}
	else
	{
		archive << type;
		archive << gravity;
		archive << range;
	}
}
#pragma endregion

#pragma region ENVIRONMENTPROBE
void EnvironmentProbe::UpdateEnvProbe()
{
	bounds.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
	bounds = bounds.get(world);
}
void EnvironmentProbe::Serialize(wiArchive& archive)
{
	Cullable::Serialize(archive);
	Transform::Serialize(archive);

	if (archive.IsReadMode())
	{
		archive >> realTime;
	}
	else
	{
		archive << realTime;
	}
}
#pragma endregion
