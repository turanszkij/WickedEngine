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

using namespace wiGraphicsTypes;

void LoadWiArmatures(const string& directory, const string& name, const string& identifier, vector<Armature*>& armatures)
{
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file){
		while(!file.eof()){
			float trans[] = { 0,0,0,0 };
			string line="";
			file>>line;
			if(line[0]=='/' && line.substr(2,8)=="ARMATURE") {
				armatures.push_back(new Armature(line.substr(11,strlen(line.c_str())-11),identifier) );
			}
			else{
				switch(line[0]){
				case 'r':
					file>>trans[0]>>trans[1]>>trans[2]>>trans[3];
					armatures.back()->rotation_rest = XMFLOAT4(trans[0],trans[1],trans[2],trans[3]);
					break;
				case 's':
					file>>trans[0]>>trans[1]>>trans[2];
					armatures.back()->scale_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					break;
				case 't':
					file>>trans[0]>>trans[1]>>trans[2];
					armatures.back()->translation_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					{
						XMMATRIX world = XMMatrixScalingFromVector(XMLoadFloat3(&armatures.back()->scale))*XMMatrixRotationQuaternion(XMLoadFloat4(&armatures.back()->rotation))*XMMatrixTranslationFromVector(XMLoadFloat3(&armatures.back()->translation));
						XMStoreFloat4x4( &armatures.back()->world_rest,world );
					}
					break;
				case 'b':
					{
						string boneName;
						file>>boneName;
						armatures.back()->boneCollection.push_back(new Bone(boneName));
						//stringstream ss("");
						//ss<<armatures.back()->name<<"_"<<boneName;
						//armatures.back()->boneCollection.push_back(new Bone(ss.str()));
						//transforms.insert(pair<string,Transform*>(armatures.back()->boneCollection.back()->name,armatures.back()->boneCollection.back()));
					}
					break;
				case 'p':
					file>>armatures.back()->boneCollection.back()->parentName;
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

						XMStoreFloat3(&armatures.back()->boneCollection.back()->translation_rest,translation);
						XMStoreFloat4(&armatures.back()->boneCollection.back()->rotation_rest,quaternion);
						XMStoreFloat4x4(&armatures.back()->boneCollection.back()->world_rest,frame);
						XMStoreFloat4x4(&armatures.back()->boneCollection.back()->restInv,XMMatrixInverse(0,frame));
						
						/*XMStoreFloat3( &armatures.back()->boneCollection.back().position,translationInverse );
						XMStoreFloat4( &armatures.back()->boneCollection.back().rotation,quaternionInverse );*/

						/*XMVECTOR sca,rot,tra;
						XMMatrixDecompose(&sca,&rot,&tra,XMMatrixInverse(0,XMMatrixTranspose(frame))*XMLoadFloat4x4(&armatures.back()->world));
						XMStoreFloat3( &armatures.back()->boneCollection.back().position,tra );
						XMStoreFloat4( &armatures.back()->boneCollection.back().rotation,rot );*/
						
					}
					break;
				case 'c':
					armatures.back()->boneCollection.back()->connected=true;
					break;
				case 'h':
					file>>armatures.back()->boneCollection.back()->length;
					break;
				default: break;
				}
			}
		}
	}
	file.close();



	//CREATE FAMILY
	for(Armature* armature : armatures){
		for(Bone* i : armature->boneCollection){
			if(i->parentName.length()>0){
				for(Bone* j : armature->boneCollection){
					if(i!=j){
						if(!i->parentName.compare(j->name)){
							i->parent=j;
							j->childrenN.push_back(i->name);
							j->childrenI.push_back(i);
							i->attachTo(j,1,1,1);
						}
					}
				}
			}
			else{
				armature->rootbones.push_back(i);
			}
		}

		for (unsigned int i = 0; i<armature->rootbones.size(); ++i){
			RecursiveRest(armature,armature->rootbones[i]);
		}
	}

}
void RecursiveRest(Armature* armature, Bone* bone){
	Bone* parent = (Bone*)bone->parent;

	if(parent!=nullptr){
		XMMATRIX recRest = 
			XMLoadFloat4x4(&bone->world_rest)
			*
			XMLoadFloat4x4(&parent->recursiveRest)
			//*
			//XMLoadFloat4x4(&armature->boneCollection[boneI].rest)
			;
		XMStoreFloat4x4( &bone->recursiveRest, recRest );
		XMStoreFloat4x4( &bone->recursiveRestInv, XMMatrixInverse(0,recRest) );
	}
	else{
		bone->recursiveRest = bone->world_rest ;
		XMStoreFloat4x4( &bone->recursiveRestInv, XMMatrixInverse(0,XMLoadFloat4x4(&bone->recursiveRest)) );
	}

	for (unsigned int i = 0; i<bone->childrenI.size(); ++i){
		RecursiveRest(armature,bone->childrenI[i]);
	}
}
void LoadWiMaterialLibrary(const string& directory, const string& name, const string& identifier, const string& texturesDir,MaterialCollection& materials)
{
	int materialI=materials.size()-1;

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
				
				stringstream identified_name("");
				identified_name<<line.substr(11,strlen(line.c_str())-11)<<identifier;
				currentMat = new Material(identified_name.str());
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
						currentMat->refMapName=ss.str();
						currentMat->refMap = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
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
void LoadWiObjects(const string& directory, const string& name, const string& identifier, vector<Object*>& objects
					, vector<Armature*>& armatures
				   , MeshCollection& meshes, const MaterialCollection& materials)
{
	int objectI=objects.size()-1;
	
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file){
		while(!file.eof()){
			float trans[] = { 0,0,0,0 };
			string line="";
			file>>line;
			if(line[0]=='/' && !strcmp(line.substr(2,6).c_str(),"OBJECT")) {
				stringstream identified_name("");
				identified_name<<line.substr(9,strlen(line.c_str())-9)<<identifier;
				objects.push_back(new Object(identified_name.str()));
				objectI++;
			}
			else{
				switch(line[0]){
				case 'm':
					{
						string meshName="";
						file>>meshName;
						stringstream identified_mesh("");
						identified_mesh<<meshName<<identifier;
						MeshCollection::iterator iter = meshes.find(identified_mesh.str());
						
						if(line[1]=='b'){ //binary load mesh in place if not present

							if(iter==meshes.end()){
								stringstream meshFileName("");
								meshFileName<<directory<<meshName<<".wimesh";
								Mesh* mesh = new Mesh();
								mesh->LoadFromFile(identified_mesh.str(),meshFileName.str(),materials,armatures,identifier);
								objects.back()->mesh=mesh;
								meshes.insert(pair<string,Mesh*>(identified_mesh.str(),mesh));
							}
							else{
								objects.back()->mesh=iter->second;
							}
						}
						else{
							if(iter!=meshes.end()) {
								objects.back()->mesh=iter->second;
								//objects.back()->mesh->usedBy.push_back(objects.size()-1);
							}
						}
					}
					break;
				case 'p':
					{
						string parentName="";
						file>>parentName;
						stringstream identified_parentName("");
						identified_parentName<<parentName<<identifier;
						objects.back()->parentName = identified_parentName.str();
						//for(Armature* a : armatures){
						//	if(!a->name.compare(identified_parentName.str())){
						//		objects.back()->parentName=identified_parentName.str();
						//		objects.back()->parent=a;
						//		objects.back()->attachTo(a,1,1,1);
						//		objects.back()->armatureDeform=true;
						//	}
						//}
					}
					break;
				case 'b':
					{
						string bone="";
						file>>bone;
						objects.back()->boneParent = bone;
						//if(objects.back()->parent!=nullptr){
						//	for(Bone* b : ((Armature*)objects.back()->parent)->boneCollection){
						//		if(!bone.compare(b->name)){
						//			objects.back()->parent=b;
						//			objects.back()->armatureDeform=false;
						//			break;
						//		}
						//	}
						//}
					}
					break;
				case 'I':
					{
						XMFLOAT3 s,t;
						XMFLOAT4 r;
						file>>t.x>>t.y>>t.z>>r.x>>r.y>>r.z>>r.w>>s.x>>s.y>>s.z;
						XMStoreFloat4x4(&objects.back()->parent_inv_rest
								, XMMatrixScalingFromVector(XMLoadFloat3(&s)) *
									XMMatrixRotationQuaternion(XMLoadFloat4(&r)) *
									XMMatrixTranslationFromVector(XMLoadFloat3(&t))
							);
					}
					break;
				case 'r':
					file>>trans[0]>>trans[1]>>trans[2]>>trans[3];
					objects.back()->Rotate(XMFLOAT4(trans[0], trans[1], trans[2],trans[3]));
					//objects.back()->rotation_rest = XMFLOAT4(trans[0],trans[1],trans[2],trans[3]);
					break;
				case 's':
					file>>trans[0]>>trans[1]>>trans[2];
					objects.back()->Scale(XMFLOAT3(trans[0], trans[1], trans[2]));
					//objects.back()->scale_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					break;
				case 't':
					file>>trans[0]>>trans[1]>>trans[2];
					objects.back()->Translate(XMFLOAT3(trans[0], trans[1], trans[2]));
					//objects.back()->translation_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					//XMStoreFloat4x4( &objects.back()->world_rest, XMMatrixScalingFromVector(XMLoadFloat3(&objects.back()->scale_rest))
					//											*XMMatrixRotationQuaternion(XMLoadFloat4(&objects.back()->rotation_rest))
					//											*XMMatrixTranslationFromVector(XMLoadFloat3(&objects.back()->translation_rest))
					//										);
					//objects.back()->world=objects.back()->world_rest;
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
						
						if(visibleEmitter) objects.back()->emitterType=Object::EMITTER_VISIBLE;
						else if(objects.back()->emitterType ==Object::NO_EMITTER) objects.back()->emitterType =Object::EMITTER_INVISIBLE;

						if(wiRenderer::EMITTERSENABLED){
							stringstream identified_materialName("");
							identified_materialName<<materialName<<identifier;
							stringstream identified_systemName("");
							identified_systemName<<systemName<<identifier;
							if(objects.back()->mesh){
								objects.back()->eParticleSystems.push_back( 
									new wiEmittedParticle(identified_systemName.str(),identified_materialName.str(),objects.back(),size,randfac,norfac,count,life,randlife,scaleX,scaleY,rot) 
									);
							}
						}
					}
					break;
				case 'H':
					{
						string name,mat,densityG,lenG;
						float len;
						int count;
						file>>name>>mat>>len>>count>>densityG>>lenG;
						
						if(wiRenderer::HAIRPARTICLEENABLED){
							stringstream identified_materialName("");
							identified_materialName<<mat<<identifier;
							objects.back()->hParticleSystems.push_back(new wiHairParticle(name,len,count,identified_materialName.str(),objects.back(),densityG,lenG) );
						}
					}
					break;
				case 'P':
					objects.back()->rigidBody = true;
					file>>objects.back()->collisionShape>>objects.back()->mass>>
						objects.back()->friction>>objects.back()->restitution>>objects.back()->damping>>objects.back()->physicsType>>
						objects.back()->kinematic;
					break;
				case 'T':
					file >> objects.back()->transparency;
					break;
				default: break;
				}
			}
		}
	}
	file.close();

	//for (unsigned int i = 0; i<objects.size(); i++){
	//	if(objects[i]->mesh){
	//		if(objects[i]->mesh->trailInfo.base>=0 && objects[i]->mesh->trailInfo.tip>=0){
	//			//objects[i]->trail.resize(MAX_RIBBONTRAILS);
	//			GPUBufferDesc bd;
	//			ZeroMemory( &bd, sizeof(bd) );
	//			bd.Usage = USAGE_DYNAMIC;
	//			bd.ByteWidth = sizeof( RibbonVertex ) * 1000;
	//			bd.BindFlags = BIND_VERTEX_BUFFER;
	//			bd.CPUAccessFlags = CPU_ACCESS_WRITE;
	//			wiRenderer::GetDevice()->CreateBuffer( &bd, NULL, &objects[i]->trailBuff );
	//			objects[i]->trailTex = wiTextureHelper::getInstance()->getTransparent();
	//			objects[i]->trailDistortTex = wiTextureHelper::getInstance()->getNormalMapDefault();
	//		}
	//	}
	//}

	//for (MeshCollection::iterator iter = meshes.begin(); iter != meshes.end(); ++iter){
	//	Mesh* iMesh = iter->second;

	//	iMesh->CreateVertexArrays();
	//	iMesh->Optimize();
	//	iMesh->CreateBuffers();
	//}

}
void LoadWiMeshes(const string& directory, const string& name, const string& identifier, MeshCollection& meshes, 
	const vector<Armature*>& armatures, const MaterialCollection& materials)
{
	int meshI=meshes.size()-1;
	Mesh* currentMesh = NULL;
	
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file){
		while(!file.eof()){
			float trans[] = { 0,0,0,0 };
			string line="";
			file>>line;
			if(line[0]=='/' && !line.substr(2,4).compare("MESH")) {
				stringstream identified_name("");
				identified_name<<line.substr(7,strlen(line.c_str())-7)<<identifier;
				currentMesh = new Mesh(identified_name.str());
				meshes.insert( pair<string,Mesh*>(currentMesh->name,currentMesh) );
				meshI++;
			}
			else{
				switch(line[0]){
				case 'p':
					{
						string parentArmature="";
						file>>parentArmature;

						stringstream identified_parentArmature("");
						identified_parentArmature<<parentArmature<<identifier;
						currentMesh->parent=identified_parentArmature.str();
						for (unsigned int i = 0; i<armatures.size(); ++i)
							if(!strcmp(armatures[i]->name.c_str(),currentMesh->parent.c_str())){
								currentMesh->armature=armatures[i];
							}
					}
					break;
				case 'v': 
					currentMesh->vertices.push_back(SkinnedVertex());
					file >> currentMesh->vertices.back().pos.x;
					file >> currentMesh->vertices.back().pos.y;
					file >> currentMesh->vertices.back().pos.z;
					break;
				case 'n':
					if (currentMesh->isBillboarded){
						currentMesh->vertices.back().nor.x = currentMesh->billboardAxis.x;
						currentMesh->vertices.back().nor.y = currentMesh->billboardAxis.y;
						currentMesh->vertices.back().nor.z = currentMesh->billboardAxis.z;
					}
					else{
						file >> currentMesh->vertices.back().nor.x;
						file >> currentMesh->vertices.back().nor.y;
						file >> currentMesh->vertices.back().nor.z;
					}
					break;
				case 'u':
					file >> currentMesh->vertices.back().tex.x;
					file >> currentMesh->vertices.back().tex.y;
					//texCoordFill++;
					break;
				case 'w':
					{
						string nameB;
						float weight=0;
						int BONEINDEX=0;
						file>>nameB>>weight;
						bool gotArmature=false;
						bool gotBone=false;
						int i=0;
						while(!gotArmature && i<(int)armatures.size()){  //SEARCH FOR PARENT ARMATURE
							if(!strcmp(armatures[i]->name.c_str(),currentMesh->parent.c_str()))
								gotArmature=true;
							else i++;
						}
						if(gotArmature){
							int j=0;
							while(!gotBone && j<(int)armatures[i]->boneCollection.size()){
								if(!armatures[i]->boneCollection[j]->name.compare(nameB)){
									gotBone=true;
									BONEINDEX=j; //GOT INDEX OF BONE OF THE WEIGHT IN THE PARENT ARMATURE
								}
								j++;
							}
						}
						if(gotBone){ //ONLY PROCEED IF CORRESPONDING BONE WAS FOUND
							if(!currentMesh->vertices.back().wei.x) {
								currentMesh->vertices.back().wei.x=weight;
								currentMesh->vertices.back().bon.x=(float)BONEINDEX;
							}
							else if(!currentMesh->vertices.back().wei.y) {
								currentMesh->vertices.back().wei.y=weight;
								currentMesh->vertices.back().bon.y=(float)BONEINDEX;
							}
							else if(!currentMesh->vertices.back().wei.z) {
								currentMesh->vertices.back().wei.z=weight;
								currentMesh->vertices.back().bon.z=(float)BONEINDEX;
							}
							else if(!currentMesh->vertices.back().wei.w) {
								currentMesh->vertices.back().wei.w=weight;
								currentMesh->vertices.back().bon.w=(float)BONEINDEX;
							}
						}

						 //(+RIBBONTRAIL SETUP)(+VERTEXGROUP SETUP)

						if(nameB.find("trailbase")!=string::npos)
							currentMesh->trailInfo.base = currentMesh->vertices.size()-1;
						else if(nameB.find("trailtip")!=string::npos)
							currentMesh->trailInfo.tip = currentMesh->vertices.size()-1;
						
						bool windAffection=false;
						if(nameB.find("wind")!=string::npos)
							windAffection=true;
						bool gotvg=false;
						for (unsigned int v = 0; v<currentMesh->vertexGroups.size(); ++v)
							if(!nameB.compare(currentMesh->vertexGroups[v].name)){
								gotvg=true;
								currentMesh->vertexGroups[v].addVertex( VertexRef(currentMesh->vertices.size()-1,weight) );
								if(windAffection)
									currentMesh->vertices.back().tex.w=weight;
							}
						if(!gotvg){
							currentMesh->vertexGroups.push_back(VertexGroup(nameB));
							currentMesh->vertexGroups.back().addVertex( VertexRef(currentMesh->vertices.size()-1,weight) );
							if(windAffection)
								currentMesh->vertices.back().tex.w=weight;
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
						stringstream identified_material("");
						identified_material<<mName<<identifier;
						currentMesh->materialNames.push_back(identified_material.str());
						MaterialCollection::const_iterator iter = materials.find(identified_material.str());
						if(iter!=materials.end()) {
							currentMesh->renderable=true;
							currentMesh->materials.push_back(iter->second);
							currentMesh->materialIndices.push_back(currentMesh->materials.size()); //CONNECT meshes WITH MATERIALS
						}
					}
					break;
				case 'a':
					file>>currentMesh->vertices.back().tex.z;
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
void LoadWiActions(const string& directory, const string& name, const string& identifier, vector<Armature*>& armatures)
{
	int armatureI=0;
	int boneI=0;
	int firstFrame=INT_MAX;

	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file){
		while(!file.eof()){
			string line="";
			file>>line;
			if(line[0]=='/' && !strcmp(line.substr(2,8).c_str(),"ARMATURE")) {
				stringstream identified_name("");
				identified_name<<line.substr(11,strlen(line.c_str())-11)<<identifier;
				string armaturename = identified_name.str() ;
				for (unsigned int i = 0; i<armatures.size(); i++)
					if(!armatures[i]->name.compare(armaturename)){
						armatureI=i;
						break;
					}
			}
			else{
				switch(line[0]){
				case 'C':
					armatures[armatureI]->actions.push_back(Action());
					file>>armatures[armatureI]->actions.back().name;
					break;
				case 'A':
					file>>armatures[armatureI]->actions.back().frameCount;
					break;
				case 'b':
					{
						string boneName;
						file>>boneName;
						for (unsigned int i = 0; i<armatures[armatureI]->boneCollection.size(); i++)
							if(!armatures[armatureI]->boneCollection[i]->name.compare(boneName)){
								boneI=i;
								break;
							} //GOT BONE INDEX
						armatures[armatureI]->boneCollection[boneI]->actionFrames.resize(armatures[armatureI]->actions.size());
					}
					break;
				case 'r':
					{
						int f = 0;
						float x=0,y=0,z=0,w=0;
						file>>f>>x>>y>>z>>w;
						armatures[armatureI]->boneCollection[boneI]->actionFrames.back().keyframesRot.push_back(KeyFrame(f,x,y,z,w));
					}
					break;
				case 't':
					{
						int f = 0;
						float x=0,y=0,z=0;
						file>>f>>x>>y>>z;
						armatures[armatureI]->boneCollection[boneI]->actionFrames.back().keyframesPos.push_back(KeyFrame(f,x,y,z,0));
					}
					break;
				case 's':
					{
						int f = 0;
						float x=0,y=0,z=0;
						file>>f>>x>>y>>z;
						armatures[armatureI]->boneCollection[boneI]->actionFrames.back().keyframesSca.push_back(KeyFrame(f,x,y,z,0));
					}
					break;
				default: break;
				}
			}
		}
	}
	file.close();
}
void LoadWiLights(const string& directory, const string& name, const string& identifier, vector<Light*>& lights)
{

	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file){
		while(!file.eof()){
			string line="";
			file>>line;
			switch(line[0]){
			case 'P':
				{
					lights.push_back(new Light());
					lights.back()->type=Light::POINT;
					string lname = "";
					file>>lname>> lights.back()->shadow;
					stringstream identified_name("");
					identified_name<<lname<<identifier;
					lights.back()->name=identified_name.str();
					lights.back()->shadowBias = 0.00001f;
					
				}
				break;
			case 'D':
				{
					lights.push_back(new Light());
					lights.back()->type=Light::DIRECTIONAL;
					file>>lights.back()->name; 
					lights.back()->shadow = true;
					lights.back()->shadowMaps_dirLight.resize(3);
					lights.back()->shadowBias = 9.99995464e-005f;
					//for (int i = 0; i < 3; ++i)
					//{
					//	lights.back()->shadowMaps_dirLight[i].Initialize(
					//		wiRenderer::SHADOWMAPRES, wiRenderer::SHADOWMAPRES
					//		, 0, true
					//		);
					//}
				}
				break;
			case 'S':
				{
					lights.push_back(new Light());
					lights.back()->type=Light::SPOT;
					file>>lights.back()->name;
					file>>lights.back()->shadow>>lights.back()->enerDis.z;
					lights.back()->shadowBias = 0.00001f;
				}
				break;
			case 'p':
				{
					string parentName="";
					file>>parentName;
					
					stringstream identified_parentName("");
					identified_parentName<<parentName<<identifier;
					lights.back()->parentName=identified_parentName.str();
					//for(map<string,Transform*>::iterator it=transforms.begin();it!=transforms.end();++it){
					//	if(!it->second->name.compare(lights.back()->parentName)){
					//		lights.back()->parent=it->second;
					//		lights.back()->attachTo(it->second,1,1,1);
					//		break;
					//	}
					//}
				}
				break;
			case 'b':
				{
					string parentBone="";
					file>>parentBone;
					lights.back()->boneParent = parentBone;

					//for(Bone* b : ((Armature*)lights.back()->parent)->boneCollection){
					//	if(!b->name.compare(parentBone)){
					//		lights.back()->parent=b;
					//		lights.back()->attachTo(b,1,1,1);
					//	}
					//}
				}
				break;
			case 'I':
				{
					XMFLOAT3 s,t;
					XMFLOAT4 r;
					file>>t.x>>t.y>>t.z>>r.x>>r.y>>r.z>>r.w>>s.x>>s.y>>s.z;
					XMStoreFloat4x4(&lights.back()->parent_inv_rest
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
					lights.back()->Translate(XMFLOAT3(x, y, z));
					//lights.back()->translation_rest=XMFLOAT3(x,y,z);
					break;
				}
			case 'r':
				{
					float x,y,z,w;
					file>>x>>y>>z>>w;
					lights.back()->Rotate(XMFLOAT4(x, y, z, w));
					//lights.back()->rotation_rest=XMFLOAT4(x,y,z,w);
					break;
				}
			case 'c':
				{
					float r,g,b;
					file>>r>>g>>b;
					lights.back()->color=XMFLOAT4(r,g,b,0);
					break;
				}
			case 'e':
				file>>lights.back()->enerDis.x;
				break;
			case 'd':
				file>>lights.back()->enerDis.y;
				//lights.back()->enerDis.y *= XMVectorGetX( world.r[0] )*0.1f;
				break;
			case 'n':
				lights.back()->noHalo=true;
				break;
			case 'l':
				{
					string t="";
					file>>t;
					stringstream rim("");
					rim<<directory<<"rims/"<<t;
					Texture2D* tex=nullptr;
					if ((tex = (Texture2D*)wiResourceManager::GetGlobal()->add(rim.str())) != nullptr){
						lights.back()->lensFlareRimTextures.push_back(tex);
						lights.back()->lensFlareNames.push_back(rim.str());
					}
				}
				break;
			default: break;
			}
		}

		//for(MeshCollection::iterator iter=lightGwiRenderer.begin(); iter!=lightGwiRenderer.end(); ++iter){
		//	Mesh* iMesh = iter->second;
		//	GPUBufferDesc bd;
		//	ZeroMemory( &bd, sizeof(bd) );
		//	bd.Usage = USAGE_DYNAMIC;
		//	bd.ByteWidth = sizeof( Instance )*iMesh->usedBy.size();
		//	bd.BindFlags = BIND_VERTEX_BUFFER;
		//	bd.CPUAccessFlags = CPU_ACCESS_WRITE;
		//	wiRenderer::GetDevice()->CreateBuffer( &bd, 0, &iMesh->meshInstanceBuffer );
		//}
	}
	file.close();
}
void LoadWiHitSpheres(const string& directory, const string& name, const string& identifier, vector<HitSphere*>& spheres
					  ,const vector<Armature*>& armatures)
{
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file)
	{
		string voidStr="";
		file>>voidStr;
		while(!file.eof()){
			string line="";
			file>>line;
			switch(line[0]){
			case 'H':
				{
					string name;
					float scale;
					XMFLOAT3 loc;
					string parentStr;
					string prop;
					file>>name>>scale>>loc.x>>loc.y>>loc.z>>parentStr>>prop;
			
					stringstream identified_parent(""),identified_name("");
					identified_parent<<parentStr<<identifier;
					identified_name<<name<<identifier;
					//Armature* parentA=nullptr;
					//Transform* parent=nullptr;
					//for(Armature* a:armatures){
					//	if(parentArmature.compare(a->name)){
					//		for(Bone* b:a->boneCollection){
					//			if(!parentBone.compare(b->name)){
					//				parentA=a;
					//				parent=b;
					//			}
					//		}
					//	}
					//}
					//spheres.push_back(new HitSphere(identified_name.str(),scale,loc,parentA,parent,prop));

					// PARENTING DISABLED ON REFACTOR! CHECK!
					//Transform* parent = nullptr;
					//if(transforms.find(identified_parent.str())!=transforms.end())
					//{
					//	parent = transforms[identified_parent.str()];
					//	spheres.push_back(new HitSphere(identified_name.str(),scale,loc,parent,prop));
					//	spheres.back()->attachTo(parent,1,1,1);
					//	transforms.insert(pair<string,Transform*>(spheres.back()->name,spheres.back()));
					//}

				}
				break;
			case 'I':
				{
					XMFLOAT3 s,t;
					XMFLOAT4 r;
					file>>t.x>>t.y>>t.z>>r.x>>r.y>>r.z>>r.w>>s.x>>s.y>>s.z;
					XMStoreFloat4x4(&spheres.back()->parent_inv_rest
							, XMMatrixScalingFromVector(XMLoadFloat3(&s)) *
								XMMatrixRotationQuaternion(XMLoadFloat4(&r)) *
								XMMatrixTranslationFromVector(XMLoadFloat3(&t))
						);
				}
				break;
			case 'b':
				{
					string parentBone = "";
					file>>parentBone;
					Armature* parentA = (Armature*)spheres.back()->parent;
					if(parentA!=nullptr){
						for(Bone* b:parentA->boneCollection){
							if(!parentBone.compare(b->name)){
								spheres.back()->attachTo(b,1,1,1);
							}
						}
					}
				}
				break;
			default: break;
			};
		}
	}
	file.close();


	////SET UP SPHERE INDEXERS
	//for(int i=0;i<spheres.size();i++){
	//	for(int j=0;j<armatures.size();j++){
	//		if(!armatures[j]->name.compare(spheres[i]->pA)){
	//			spheres[i]->parentArmature=armatures[j];
	//			for(int k=0;k<armatures[j]->boneCollection.size();k++)
	//				if(!armatures[j]->boneCollection[k]->name.compare(spheres[i]->pB)){
	//					spheres[i]->parentBone=k;
	//					break;
	//				}
	//			break;
	//		}
	//	}
	//}
}
void LoadWiWorldInfo(const string&directory, const string& name, WorldInfo& worldInfo, Wind& wind){
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file){
		while(!file.eof()){
			string read = "";
			file>>read;
			switch(read[0]){
			case 'h':
				file>>worldInfo.horizon.x>>worldInfo.horizon.y>>worldInfo.horizon.z;
				break;
			case 'z':
				file>>worldInfo.zenith.x>>worldInfo.zenith.y>>worldInfo.zenith.z;
				break;
			case 'a':
				file>>worldInfo.ambient.x>>worldInfo.ambient.y>>worldInfo.ambient.z;
				break;
			case 'W':
				{
					XMFLOAT4 r;
					float s;
					file>>r.x>>r.y>>r.z>>r.w>>s;
					XMStoreFloat3(&wind.direction, XMVector3Transform( XMVectorSet(0,s,0,0),XMMatrixRotationQuaternion(XMLoadFloat4(&r)) ));
				}
				break;
			case 'm':
				{
					float s,e,h;
					file>>s>>e>>h;
					worldInfo.fogSEH=XMFLOAT3(s,e,h);
				}
				break;
			default:break;
			}
		}
	}
	file.close();
}
void LoadWiCameras(const string&directory, const string& name, const string& identifier, vector<Camera>& cameras
				   ,const vector<Armature*>& armatures){
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

			
					stringstream identified_parentArmature("");
					identified_parentArmature<<parentA<<identifier;
			
					cameras.push_back(Camera(
						trans,rot
						,name)
						);

					for (unsigned int i = 0; i<armatures.size(); ++i){
						if(!armatures[i]->name.compare(identified_parentArmature.str())){
							for (unsigned int j = 0; j<armatures[i]->boneCollection.size(); ++j){
								if(!armatures[i]->boneCollection[j]->name.compare(parentB.c_str()))
									cameras.back().attachTo(armatures[i]->boneCollection[j]);
							}
						}
					}

				}
				break;
			case 'I':
				{
					XMFLOAT3 s,t;
					XMFLOAT4 r;
					file>>t.x>>t.y>>t.z>>r.x>>r.y>>r.z>>r.w>>s.x>>s.y>>s.z;
					XMStoreFloat4x4(&cameras.back().parent_inv_rest
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
void LoadWiDecals(const string&directory, const string& name, const string& texturesDir, list<Decal*>& decals){
	stringstream filename("");
	filename<<directory<<name;

	ifstream file(filename.str().c_str());
	if(file)
	{
		string voidStr="";
		file>>voidStr;
		while(!file.eof()){
			string line="";
			file>>line;
			switch(line[0]){
			case 'd':
				{
					string name;
					XMFLOAT3 loc,scale;
					XMFLOAT4 rot;
					file>>name>>scale.x>>scale.y>>scale.z>>loc.x>>loc.y>>loc.z>>rot.x>>rot.y>>rot.z>>rot.w;
					Decal* decal = new Decal();
					decal->name=name;
					decal->translation_rest=loc;
					decal->scale_rest=scale;
					decal->rotation_rest=rot;
					decals.push_back(new Decal(loc,scale,rot));
				}
				break;
			case 't':
				{
					string tex="";
					file>>tex;
					stringstream ss("");
					ss<<directory<<texturesDir<<tex;
					decals.back()->addTexture(ss.str());
				}
				break;
			case 'n':
				{
					string tex="";
					file>>tex;
					stringstream ss("");
					ss<<directory<<texturesDir<<tex;
					decals.back()->addNormal(ss.str());
				}
				break;
			default:break;
			};
		}
	}
	file.close();
}


//void LoadFromDisk(const string& dir, const string& name, const string& identifier
//				  , vector<Armature*>& armatures
//				  , MaterialCollection& materials
//				  , vector<Object*>& objects
//				  , MeshCollection& meshes
//				  , vector<Light*>& lights
//				  , vector<HitSphere*>& spheres
//				  , WorldInfo& worldInfo, Wind& wind
//				  , vector<Camera>& cameras
//				  , map<string,Transform*>& transforms
//				  , list<Decal*>& decals
//				  )
//{
//	MaterialCollection		l_materials;
//	vector<Armature*>		l_armatures;
//	vector<Object*>			l_objects;
//	MeshCollection			l_meshes;
//	vector<Light*>			l_lights;
//	vector<HitSphere*>		l_spheres;
//	WorldInfo				l_worldInfo = worldInfo;
//	Wind					l_wind = wind;
//	vector<Camera>			l_cameras;
//	map<string,Transform*>  l_transforms;
//	list<Decal*>			l_decals;
//
//	stringstream directory(""),armatureFilePath(""),materialLibFilePath(""),meshesFilePath(""),objectsFilePath("")
//		,actionsFilePath(""),lightsFilePath(""),worldInfoFilePath(""),enviroMapFilePath(""),hitSpheresFilePath("")
//		,camerasFilePath(""),decalsFilePath("");
//
//	directory<<dir;
//	armatureFilePath<<name<<".wia";
//	materialLibFilePath<<name<<".wim";
//	meshesFilePath<<name<<".wi";
//	objectsFilePath<<name<<".wio";
//	actionsFilePath<<name<<".wiact";
//	lightsFilePath<<name<<".wil";
//	worldInfoFilePath<<name<<".wiw";
//	hitSpheresFilePath<<name<<".wih";
//	camerasFilePath<<name<<".wic";
//	decalsFilePath<<name<<".wid";
//
//	LoadWiArmatures(directory.str(), armatureFilePath.str(),identifier,l_armatures,l_transforms);
//	LoadWiMaterialLibrary(directory.str(), materialLibFilePath.str(),identifier, "textures/", l_materials);
//	LoadWiMeshes(directory.str(), meshesFilePath.str(),identifier,meshes,l_armatures,l_materials);
//	LoadWiObjects(directory.str(), objectsFilePath.str(),identifier,l_objects,l_armatures,meshes,l_transforms,l_materials);
//	LoadWiActions(directory.str(), actionsFilePath.str(),identifier,l_armatures);
//	LoadWiLights(directory.str(), lightsFilePath.str(),identifier, l_lights, l_armatures,l_transforms);
//	LoadWiHitSpheres(directory.str(), hitSpheresFilePath.str(),identifier,spheres,l_armatures,l_transforms);
//	LoadWiCameras(directory.str(), camerasFilePath.str(),identifier,l_cameras,l_armatures,l_transforms);
//	LoadWiWorldInfo(directory.str(), worldInfoFilePath.str(),l_worldInfo,l_wind);
//	LoadWiDecals(directory.str(), decalsFilePath.str(), "textures/", l_decals);
//
//	wiRenderer::graphicsMutex.lock();
//	{
//		armatures.insert(armatures.end(),l_armatures.begin(),l_armatures.end());
//		objects.insert(objects.end(),l_objects.begin(),l_objects.end());
//		lights.insert(lights.end(),l_lights.begin(),l_lights.end());
//		spheres.insert(spheres.end(),l_spheres.begin(),l_spheres.end());
//		cameras.insert(cameras.end(),l_cameras.begin(),l_cameras.end());
//
//		meshes.insert(l_meshes.begin(),l_meshes.end());
//		materials.insert(l_materials.begin(),l_materials.end());
//
//		worldInfo=l_worldInfo;
//		wind=l_wind;
//
//		transforms.insert(l_transforms.begin(),l_transforms.end());
//
//		decals.insert(decals.end(),l_decals.begin(),l_decals.end());
//	}
//	wiRenderer::graphicsMutex.unlock();
//}


void GenerateSPTree(wiSPTree*& tree, vector<Cullable*>& objects, int type){
	if(type==SPTREE_GENERATE_QUADTREE)
		tree = new QuadTree();
	else if(type==SPTREE_GENERATE_OCTREE)
		tree = new Octree();
	tree->initialize(objects);
}

#pragma region SCENE
Scene::Scene()
{
	models.push_back(new Model);
	models.back()->name = "[WickedEngine-default]{WorldNode}";
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
	Model* world = GetWorldNode();
	for (unsigned int i = 1; i < models.size();++i)
	{
		SAFE_DELETE(models[i]);
	}
	models.clear();
	models.push_back(world);

	for (unsigned int i = 0; i < environmentProbes.size(); ++i)
	{
		SAFE_DELETE(environmentProbes[i]);
	}
	environmentProbes.clear();
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
Cullable::Cullable():bounds(AABB())/*,lastSquaredDistMulThousand(0)*/{}
#pragma endregion

#pragma region STREAMABLE
Streamable::Streamable():directory(""),meshfile(""),materialfile(""),loaded(false){}
#pragma endregion

#pragma region MATERIAL
Material::~Material() {
	wiResourceManager::GetGlobal()->del(refMapName);
	wiResourceManager::GetGlobal()->del(textureName);
	wiResourceManager::GetGlobal()->del(normalMapName);
	wiResourceManager::GetGlobal()->del(displacementMapName);
	wiResourceManager::GetGlobal()->del(specularMapName);
	refMap = nullptr;
	texture = nullptr;
	normalMap = nullptr;
	displacementMap = nullptr;
	specularMap = nullptr;
}
void Material::ConvertToPhysicallyBasedMaterial()
{
	baseColor = diffuseColor;
	roughness = (1 - (float)specular_power / 128.0f);
	metalness = 0.0f;
	reflectance = (specular.x + specular.y + specular.z) / 3.0f * specular.w;
	normalMapStrength = 1.0f;
}
Texture2D* Material::GetBaseColorMap()
{
	if (texture != nullptr)
	{
		return texture;
	}
	return wiTextureHelper::getInstance()->getWhite();
}
Texture2D* Material::GetNormalMap()
{
	if (normalMap != nullptr)
	{
		return normalMap;
	}
	return wiTextureHelper::getInstance()->getNormalMapDefault();
}
Texture2D* Material::GetRoughnessMap()
{
	return wiTextureHelper::getInstance()->getColor(wiColor::Gray);
}
Texture2D* Material::GetMetalnessMap()
{
	return wiTextureHelper::getInstance()->getWhite();
}
Texture2D* Material::GetReflectanceMap()
{
	if (refMap != nullptr)
	{
		return refMap;
	}
	return wiTextureHelper::getInstance()->getWhite();
}
Texture2D* Material::GetDisplacementMap()
{
	if (displacementMap != nullptr)
	{
		return displacementMap;
	}
	return wiTextureHelper::getInstance()->getWhite();
}
#pragma endregion

#pragma region MESH

GPUBuffer Mesh::meshInstanceBuffer;

void Mesh::LoadFromFile(const string& newName, const string& fname
	, const MaterialCollection& materialColl, vector<Armature*> armatures, const string& identifier) {
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
			identified_parent << parent << identifier;
			for (Armature* a : armatures) {
				if (!a->name.compare(identified_parent.str())) {
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
			identified_matname << matName << identifier;
			MaterialCollection::const_iterator iter = materialColl.find(identified_matname.str());
			if (iter != materialColl.end()) {
				materials.push_back(iter->second);
			}

			materialNames.push_back(identified_matname.str());
			delete[] matName;
		}
		int rendermesh, vertexCount;
		memcpy(&rendermesh, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&vertexCount, buffer + offset, sizeof(int));
		offset += sizeof(int);

		vertices.reserve(vertexCount);
		for (int i = 0; i<vertexCount; ++i) {
			SkinnedVertex vert = SkinnedVertex();
			float v[8];
			memcpy(v, buffer + offset, sizeof(float) * 8);
			offset += sizeof(float) * 8;
			vert.pos.x = v[0];
			vert.pos.y = v[1];
			vert.pos.z = v[2];
			if (!isBillboarded) {
				vert.nor.x = v[3];
				vert.nor.y = v[4];
				vert.nor.z = v[5];
			}
			else {
				vert.nor.x = billboardAxis.x;
				vert.nor.y = billboardAxis.y;
				vert.nor.z = billboardAxis.z;
			}
			vert.tex.x = v[6];
			vert.tex.y = v[7];
			int matIndex;
			memcpy(&matIndex, buffer + offset, sizeof(int));
			offset += sizeof(int);
			vert.tex.z = (float)matIndex;

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
						if (!vert.wei.x) {
							vert.wei.x = weightValue;
							vert.bon.x = (float)BONEINDEX;
						}
						else if (!vert.wei.y) {
							vert.wei.y = weightValue;
							vert.bon.y = (float)BONEINDEX;
						}
						else if (!vert.wei.z) {
							vert.wei.z = weightValue;
							vert.bon.z = (float)BONEINDEX;
						}
						else if (!vert.wei.w) {
							vert.wei.w = weightValue;
							vert.bon.w = (float)BONEINDEX;
						}
					}
				}

				//(+RIBBONTRAIL SETUP)(+VERTEXGROUP SETUP)

				if (nameB.find("trailbase") != string::npos)
					trailInfo.base = vertices.size();
				else if (nameB.find("trailtip") != string::npos)
					trailInfo.tip = vertices.size();

				bool windAffection = false;
				if (nameB.find("wind") != string::npos)
					windAffection = true;
				bool gotvg = false;
				for (unsigned int v = 0; v<vertexGroups.size(); ++v)
					if (!nameB.compare(vertexGroups[v].name)) {
						gotvg = true;
						vertexGroups[v].addVertex(VertexRef(vertices.size(), weightValue));
						if (windAffection)
							vert.tex.w = weightValue;
					}
				if (!gotvg) {
					vertexGroups.push_back(VertexGroup(nameB));
					vertexGroups.back().addVertex(VertexRef(vertices.size(), weightValue));
					if (windAffection)
						vert.tex.w = weightValue;
				}
#pragma endregion

				delete[] weightName;


			}


			vertices.push_back(vert);
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

		//CreateVertexArrays();

		//Optimize();

		//CreateBuffers();
	}
}
void Mesh::Optimize()
{
	//TODO
}
void Mesh::CreateBuffers(Object* object) {
	if (!buffersComplete) {

		GPUBufferDesc bd;
		if (!meshInstanceBuffer.IsValid())
		{
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_DYNAMIC;
			bd.ByteWidth = sizeof(Instance) * 2;
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = CPU_ACCESS_WRITE;
			wiRenderer::GetDevice()->CreateBuffer(&bd, 0, &meshInstanceBuffer);
		}


		if (goalVG >= 0) {
			goalPositions.resize(vertexGroups[goalVG].vertices.size());
			goalNormals.resize(vertexGroups[goalVG].vertices.size());
		}



		ZeroMemory(&bd, sizeof(bd));
#ifdef USE_GPU_SKINNING
		bd.Usage = (softBody ? USAGE_DYNAMIC : USAGE_IMMUTABLE);
		bd.CPUAccessFlags = (softBody ? CPU_ACCESS_WRITE : 0);
		if (object->isArmatureDeformed() && !softBody)
			bd.ByteWidth = sizeof(SkinnedVertex) * vertices.size();
		else
			bd.ByteWidth = sizeof(Vertex) * vertices.size();
#else
		bd.Usage = ((softBody || object->isArmatureDeformed()) ? USAGE_DYNAMIC : USAGE_IMMUTABLE);
		bd.CPUAccessFlags = ((softBody || object->isArmatureDeformed()) ? CPU_ACCESS_WRITE : 0);
		bd.ByteWidth = sizeof(Vertex) * vertices.size();
#endif
		bd.BindFlags = BIND_VERTEX_BUFFER;
		SubresourceData InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		if (object->isArmatureDeformed() && !softBody)
			InitData.pSysMem = vertices.data();
		else
			InitData.pSysMem = skinnedVertices.data();
		wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, &meshVertBuff);


		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(unsigned int) * indices.size();
		bd.BindFlags = BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = indices.data();
		wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, &meshIndexBuff);

		if (renderable)
		{

			if (object->isArmatureDeformed() && !softBody) {
				ZeroMemory(&bd, sizeof(bd));
				bd.Usage = USAGE_DEFAULT;
				bd.ByteWidth = sizeof(Vertex) * vertices.size();
				bd.BindFlags = BIND_STREAM_OUTPUT | BIND_VERTEX_BUFFER;
				bd.CPUAccessFlags = 0;
				bd.StructureByteStride = 0;
				wiRenderer::GetDevice()->CreateBuffer(&bd, NULL, &sOutBuffer);
			}

			//PHYSICALMAPPING
			if (!physicsverts.empty())
			{
				for (unsigned int i = 0; i < vertices.size(); ++i) {
					for (unsigned int j = 0; j < physicsverts.size(); ++j) {
						if (fabs(vertices[i].pos.x - physicsverts[j].x) < FLT_EPSILON
							&&	fabs(vertices[i].pos.y - physicsverts[j].y) < FLT_EPSILON
							&&	fabs(vertices[i].pos.z - physicsverts[j].z) < FLT_EPSILON
							)
						{
							physicalmapGP.push_back(j);
							break;
						}
					}
				}
			}
		}


		buffersComplete = true;
	}

}
void Mesh::CreateVertexArrays()
{
	if (skinnedVertices.empty())
	{
		skinnedVertices.resize(vertices.size());
		for (unsigned int i = 0; i<vertices.size(); ++i) {
			skinnedVertices[i].pos = vertices[i].pos;
			skinnedVertices[i].nor = vertices[i].nor;
			skinnedVertices[i].tex = vertices[i].tex;
		}
	}
}

vector<Instance> meshInstances[GRAPHICSTHREAD_COUNT];
void Mesh::AddRenderableInstance(const Instance& instance, int numerator, GRAPHICSTHREAD threadID)
{
	if (numerator >= (int)meshInstances[threadID].size())
	{
		meshInstances[threadID].resize((meshInstances[threadID].size() + 1) * 2);
	}
	meshInstances[threadID][numerator] = instance;
}
void Mesh::UpdateRenderableInstances(int count, GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->UpdateBuffer(&meshInstanceBuffer, meshInstances[threadID].data(), threadID, sizeof(Instance)*count);
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
	for (Object* x : objects)
	{
		for (wiEmittedParticle* y : x->eParticleSystems)
		{
			SAFE_DELETE(y);
		}
		for (wiHairParticle* y : x->hParticleSystems)
		{
			SAFE_DELETE(y);
		}
		SAFE_DELETE(x);
	}
	for (Light* x : lights)
	{
		SAFE_DELETE(x);
	}
	for (Decal* x : decals)
	{
		SAFE_DELETE(x);
	}
}
void Model::LoadFromDisk(const string& dir, const string& name, const string& identifier)
{
	stringstream directory(""), armatureFilePath(""), materialLibFilePath(""), meshesFilePath(""), objectsFilePath("")
		, actionsFilePath(""), lightsFilePath(""), decalsFilePath("");

	directory << dir;
	armatureFilePath << name << ".wia";
	materialLibFilePath << name << ".wim";
	meshesFilePath << name << ".wi";
	objectsFilePath << name << ".wio";
	actionsFilePath << name << ".wiact";
	lightsFilePath << name << ".wil";
	decalsFilePath << name << ".wid";

	LoadWiArmatures(directory.str(), armatureFilePath.str(), identifier, armatures);
	LoadWiMaterialLibrary(directory.str(), materialLibFilePath.str(), identifier, "textures/", materials);
	LoadWiMeshes(directory.str(), meshesFilePath.str(), identifier, meshes, armatures, materials);
	LoadWiObjects(directory.str(), objectsFilePath.str(), identifier, objects, armatures, meshes, materials);
	LoadWiActions(directory.str(), actionsFilePath.str(), identifier, armatures);
	LoadWiLights(directory.str(), lightsFilePath.str(), identifier, lights);
	LoadWiDecals(directory.str(), decalsFilePath.str(), "textures/", decals);

	vector<Transform*> transforms(0);
	transforms.reserve(armatures.size() + objects.size() + lights.size() + decals.size());

	for (Armature* x : armatures)
	{
		if (x->actions.size() > 1)
		{
			// If it has actions besides the identity, activate the first by default
			x->GetPrimaryAnimation()->ChangeAction(1);
		}
		transforms.push_back(x);
	}
	for (Object* x : objects) {
		transforms.push_back(x);
		for (wiEmittedParticle* e : x->eParticleSystems)
		{
			if (e->light != nullptr)
			{
				lights.push_back(e->light);
			}
		}
	}
	for (Light* x : lights)
	{
		x->SetUp();
		transforms.push_back(x);
	}
	for (Decal* x : decals)
	{
		transforms.push_back(x);
	}


	// Match loaded parenting information
	for (Transform* x : transforms)
	{
		for (Transform* y : transforms)
		{
			if (x != y && x->parentName.length() > 0 && !x->parentName.compare(y->name))
			{
				// Match parent
				XMFLOAT4X4 saved_parent_rest_inv = x->parent_inv_rest;
				x->attachTo(y);
				x->parent_inv_rest = saved_parent_rest_inv;
				break;
			}
		}

		if (x->parent != nullptr && x->parentName.length() > 0 && x->boneParent.length() > 0)
		{
			// Match Bone parent
			Armature* armature = dynamic_cast<Armature*>(x->parent);
			if (armature != nullptr)
			{
				// Only if the current parent is an Armature!
				for (Bone* b : armature->boneCollection) {
					if (!b->name.compare(x->boneParent))
					{
						XMFLOAT4X4 saved_parent_rest_inv = x->parent_inv_rest;
						x->attachTo(b);
						x->parent_inv_rest = saved_parent_rest_inv;
						break;
					}
				}
			}
		}

		if (x->parent == nullptr)
		{
			x->attachTo(this);
		}
	}


	// Set up Render data
	for (Object* x : objects)
	{
		if (x->mesh) 
		{
			// Ribbon trails
			if (x->mesh->trailInfo.base >= 0 && x->mesh->trailInfo.tip >= 0) {
				GPUBufferDesc bd;
				ZeroMemory(&bd, sizeof(bd));
				bd.Usage = USAGE_DYNAMIC;
				bd.ByteWidth = sizeof(RibbonVertex) * 1000;
				bd.BindFlags = BIND_VERTEX_BUFFER;
				bd.CPUAccessFlags = CPU_ACCESS_WRITE;
				wiRenderer::GetDevice()->CreateBuffer(&bd, NULL, &x->trailBuff);
				x->trailTex = wiTextureHelper::getInstance()->getTransparent();
				x->trailDistortTex = wiTextureHelper::getInstance()->getNormalMapDefault();
			}

			// Mesh renderdata setup
			x->mesh->CreateVertexArrays();
			x->mesh->Optimize();
			x->mesh->CreateBuffers(x);
		}
	}

}
void Model::UpdateModel()
{
	for (MaterialCollection::iterator iter = materials.begin(); iter != materials.end(); ++iter) {
		Material* iMat = iter->second;
		iMat->framesToWaitForTexCoordOffset -= 1.0f;
		if (iMat->framesToWaitForTexCoordOffset <= 0) {
			iMat->texMulAdd.z += iMat->movingTex.x*wiRenderer::GetGameSpeed();
			iMat->texMulAdd.w += iMat->movingTex.y*wiRenderer::GetGameSpeed();
			iMat->framesToWaitForTexCoordOffset = iMat->movingTex.z*wiRenderer::GetGameSpeed();
		}
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
	
	list<Decal*>::iterator iter = decals.begin();
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
#pragma endregion

#pragma region AABB
AABB::AABB(){
	for(int i=0;i<8;++i) corners[i]=XMFLOAT3(0,0,0);
}
AABB::AABB(const XMFLOAT3& min, const XMFLOAT3& max){
	create(min,max);
}
void AABB::createFromHalfWidth(const XMFLOAT3& center, const XMFLOAT3& halfwidth){
	XMFLOAT3 min = XMFLOAT3(center.x-halfwidth.x,center.y-halfwidth.y,center.z-halfwidth.z);
	XMFLOAT3 max = XMFLOAT3(center.x+halfwidth.x,center.y+halfwidth.y,center.z+halfwidth.z);
	create(min,max);
}
void AABB::create(const XMFLOAT3& min, const XMFLOAT3& max){
	corners[0]=min;
	corners[1]=XMFLOAT3(min.x,max.y,min.z);
	corners[2]=XMFLOAT3(min.x,max.y,max.z);
	corners[3]=XMFLOAT3(min.x,min.y,max.z);
	corners[4]=XMFLOAT3(max.x,min.y,min.z);
	corners[5]=XMFLOAT3(max.x,max.y,min.z);
	corners[6]=max;
	corners[7]=XMFLOAT3(max.x,min.y,max.z);
}
AABB AABB::get(const XMMATRIX& mat){
	AABB ret;
	XMFLOAT3 min,max;
	for(int i=0;i<8;++i){
		XMVECTOR point = XMVector3Transform( XMLoadFloat3(&corners[i]), mat );
		XMStoreFloat3(&ret.corners[i],point);
	}
	min=ret.corners[0];
	max=ret.corners[6];
	for(int i=0;i<8;++i){
		XMFLOAT3& p=ret.corners[i];

		if(p.x<min.x) min.x=p.x;
		if(p.y<min.y) min.y=p.y;
		if(p.z<min.z) min.z=p.z;

		if(p.x>max.x) max.x=p.x;
		if(p.y>max.y) max.y=p.y;
		if(p.z>max.z) max.z=p.z;
	}

	ret.create(min,max);

	return ret;
}
AABB AABB::get(const XMFLOAT4X4& mat){
	return get(XMLoadFloat4x4(&mat));
}
XMFLOAT3 AABB::getMin()const {return corners[0];}
XMFLOAT3 AABB::getMax() const{return corners[6];}
XMFLOAT3 AABB::getCenter() const{
	XMFLOAT3 min=getMin(),max=getMax();
	return XMFLOAT3((min.x+max.x)*0.5f,(min.y+max.y)*0.5f,(min.z+max.z)*0.5f);
}
XMFLOAT3 AABB::getHalfWidth() const{
	XMFLOAT3 max=getMax(),center=getCenter();
	return XMFLOAT3(abs(max.x-center.x),abs(max.y-center.y),abs(max.z-center.z));
}
float AABB::getArea() const{
	XMFLOAT3 _min = getMin();
	XMFLOAT3 _max = getMax();
	return (_max.x-_min.x)*(_max.y-_min.y)*(_max.z-_min.z);
}
float AABB::getRadius() const{
	XMFLOAT3& abc=getHalfWidth();
	return max(max(abc.x,abc.y),abc.z);
}
AABB::INTERSECTION_TYPE AABB::intersects(const AABB& b) const{

	XMFLOAT3 aMin = getMin(), aMax=getMax();
    XMFLOAT3 bMin = b.getMin(), bMax=b.getMax();

    if( bMin.x >= aMin.x && bMax.x <= aMax.x &&
        bMin.y >= aMin.y && bMax.y <= aMax.y &&
        bMin.z >= aMin.z && bMax.z <= aMax.z )
    {
        return INSIDE;
    }
    
    if( aMax.x < bMin.x || aMin.x > bMax.x )
        return OUTSIDE;
    if( aMax.y < bMin.y || aMin.y > bMax.y )
        return OUTSIDE;
    if( aMax.z < bMin.z || aMin.z > bMax.z )
        return OUTSIDE;
        
    return INTERSECTS;
}
bool AABB::intersects(const XMFLOAT3& p) const{
	XMFLOAT3 max=getMax();
	XMFLOAT3 min=getMin();
	if (p.x>max.x) return false;
	if (p.x<min.x) return false;
	if (p.y>max.y) return false;
	if (p.y<min.y) return false;
	if (p.z>max.z) return false;
	if (p.z<min.z) return false;
	return true;
}
bool AABB::intersects(const RAY& ray) const{
	if(intersects(ray.origin))
		return true;

	XMFLOAT3 MIN = getMin();
	XMFLOAT3 MAX = getMax();

	float tx1 = (MIN.x - ray.origin.x)*ray.direction_inverse.x;
	float tx2 = (MAX.x - ray.origin.x)*ray.direction_inverse.x;
 	
	float tmin = min(tx1, tx2);
	float tmax = max(tx1, tx2);
 	
	float ty1 = (MIN.y - ray.origin.y)*ray.direction_inverse.y;
	float ty2 = (MAX.y - ray.origin.y)*ray.direction_inverse.y;
 
	tmin = max(tmin, min(ty1, ty2));
	tmax = min(tmax, max(ty1, ty2));
	
	float tz1 = (MIN.z - ray.origin.z)*ray.direction_inverse.z;
	float tz2 = (MAX.z - ray.origin.z)*ray.direction_inverse.z;
 
	tmin = max(tmin, min(tz1, tz2));
	tmax = min(tmax, max(tz1, tz2));
 
	return tmax >= tmin;
}
AABB AABB::operator* (float a)
{
	XMFLOAT3 min = getMin();
	XMFLOAT3 max = getMax();
	min.x*=a;
	min.y*=a;
	min.z*=a;
	max.x*=a;
	max.y*=a;
	max.z*=a;
	return AABB(min,max);
}
#pragma endregion

#pragma region SPHERE
bool SPHERE::intersects(const AABB& b){
	XMFLOAT3 min = b.getMin();
	XMFLOAT3 max = b.getMax();
	XMFLOAT3 closestPointInAabb = wiMath::Min(wiMath::Max(center, min), max);
	double distanceSquared = wiMath::Distance(closestPointInAabb,center);
	return distanceSquared < radius;
}
bool SPHERE::intersects(const SPHERE& b){
	return wiMath::Distance(center,b.center)<=radius+b.radius;
}
#pragma endregion

#pragma region RAY
bool RAY::intersects(const AABB& box) const{
	return box.intersects(*this);
}
#pragma endregion

#pragma region HITSPHERE
GPUBuffer HitSphere::vertexBuffer;
void HitSphere::SetUpStatic()
{
	const int numVert = (RESOLUTION+1)*2;
	vector<XMFLOAT3A> verts(0);

	for(int i=0;i<=RESOLUTION;++i){
		float alpha = (float)i/(float)RESOLUTION*2*3.14159265359f;
		verts.push_back(XMFLOAT3A(XMFLOAT3A(sin(alpha),cos(alpha),0)));
		verts.push_back(XMFLOAT3A(XMFLOAT3A(0,0,0)));
	}

	GPUBufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof( XMFLOAT3A )*verts.size();
	bd.BindFlags = BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	SubresourceData InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = verts.data();
	wiRenderer::GetDevice()->CreateBuffer( &bd, &InitData, &vertexBuffer );
}
void HitSphere::CleanUpStatic()
{
	
}
void HitSphere::UpdateTransform()
{
	Transform::UpdateTransform();
	
	//getMatrix();
	center = translation;
	radius = radius_saved*scale.x;
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

	// calculate frame
	for (Bone* root : rootbones) 
	{
		RecursiveBoneTransform(this, root, getMatrix());
	}

}
void Armature::UpdateArmature()
{

	for (auto& x : animationLayers)
	{
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

	bone->boneRelativityPrev = bone->boneRelativity;
	XMStoreFloat4x4(&bone->boneRelativity, finalMat);

	for (unsigned int i = 0; i<bone->childrenI.size(); ++i) {
		RecursiveBoneTransform(armature, bone->childrenI[i], boneMat);
	}

	// Because bones are not updated in the regular Transform fashion, a separate update needs to be called
	bone->UpdateTransform();
}
XMVECTOR Armature::InterPolateKeyFrames(float cf, const int maxCf, const vector<KeyFrame>& keyframeList, KeyFrameType type)
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
			nearest[0] = keyframeList.size() - 1;
			nearest[1] = keyframeList.size() - 1;
		}
		else { //IN BETWEEN TWO KEYFRAMES, DECIDE WHICH
			for (int k = keyframeList.size() - 1; k>0; k--)
				if (keyframeList[k].frameI <= cf) {
					nearest[0] = k;
					break;
				}
			for (unsigned int k = 0; k<keyframeList.size(); k++)
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

void Armature::ChangeAction(const string& actionName, float blendFrames, const string& animLayer, float weight)
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
AnimationLayer* Armature::GetAnimLayer(const string& name)
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
void Armature::AddAnimLayer(const string& name)
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
void Armature::DeleteAnimLayer(const string& name)
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
#pragma endregion

#pragma region Decals
Decal::Decal(const XMFLOAT3& tra, const XMFLOAT3& sca, const XMFLOAT4& rot, const string& tex, const string& nor):Cullable(),Transform(){
	scale_rest=scale=sca;
	rotation_rest=rotation=rot;
	translation_rest=translation=tra;

	UpdateTransform();

	texture=normal=nullptr;
	addTexture(tex);
	addNormal(nor);

	life = -2; //persistent
	fadeStart=0;
}
Decal::~Decal() {
	wiResourceManager::GetGlobal()->del(texName);
	wiResourceManager::GetGlobal()->del(norName);
}
void Decal::addTexture(const string& tex){
	texName=tex;
	if(!tex.empty()){
		texture = (Texture2D*)wiResourceManager::GetGlobal()->add(tex);
	}
}
void Decal::addNormal(const string& nor){
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
	XMVECTOR frontV = XMVector3Transform(XMVectorSet(0, 0, 1, 0), rotMat);
	XMStoreFloat3(&front, frontV);
	XMVECTOR at = XMVectorAdd(eye, frontV);
	XMVECTOR up = XMVector3Transform(XMVectorSet(0, 1, 0, 0), rotMat);
	XMStoreFloat4x4(&view, XMMatrixLookAtLH(eye, at, up));
	XMStoreFloat4x4(&projection, XMMatrixOrthographicLH(scale.x, scale.y, -scale.z * 0.5f, scale.z * 0.5f));
	XMStoreFloat4x4(&world_rest, XMMatrixScalingFromVector(XMLoadFloat3(&scale))*rotMat*XMMatrixTranslationFromVector(eye));

	bounds.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(scale.x, scale.y, scale.z));
	bounds = bounds.get(XMLoadFloat4x4(&world_rest));

}
void Decal::UpdateDecal()
{
	if (life>-2) {
		life -= wiRenderer::GetGameSpeed();
	}
}
#pragma endregion

#pragma region CAMERA
void Camera::UpdateTransform()
{
	Transform::UpdateTransform();

	//getMatrix();
	UpdateProps();
}
#pragma endregion

#pragma region OBJECT
Object::~Object() {
}
void Object::EmitTrail(const XMFLOAT3& col, float fadeSpeed) {
	if (mesh != nullptr)
	{
		int base = mesh->trailInfo.base;
		int tip = mesh->trailInfo.tip;


		int x = trail.size();

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
		wiRenderer::objectsWithTrails.push_back(this);
		FadeTrail();
	}

	for (wiEmittedParticle* x : eParticleSystems)
	{
		x->Update(wiRenderer::GetGameSpeed());
		wiRenderer::emitterSystems.push_back(x);
	}
}
#pragma endregion

#pragma region LIGHT
vector<wiRenderTarget> Light::shadowMaps_pointLight;
vector<wiRenderTarget> Light::shadowMaps_spotLight;
Light::~Light() {
	shadowCam.clear();
	//shadowMap.clear();
	shadowMaps_dirLight.clear();
	lensFlareRimTextures.clear();
	for (string x : lensFlareNames)
		wiResourceManager::GetGlobal()->del(x);
	lensFlareNames.clear();
}
void Light::SetUp()
{
	if (!shadowCam.empty())
		return;

	if (type == Light::DIRECTIONAL) {

		float lerp = 0.5f;
		float lerp1 = 0.12f;
		float lerp2 = 0.016f;
		XMVECTOR a0, a, b0, b;
		a0 = XMVector3Unproject(XMVectorSet(0, (float)wiRenderer::GetDevice()->GetScreenHeight(), 0, 1), 0, 0, (float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
		a = XMVector3Unproject(XMVectorSet(0, (float)wiRenderer::GetDevice()->GetScreenHeight(), 1, 1), 0, 0, (float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
		b0 = XMVector3Unproject(XMVectorSet((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0, 1), 0, 0, (float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
		b = XMVector3Unproject(XMVectorSet((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 1, 1), 0, 0, (float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
		float size = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0, b, lerp), XMVectorLerp(a0, a, lerp))));
		float size1 = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0, b, lerp1), XMVectorLerp(a0, a, lerp1))));
		float size2 = XMVectorGetX(XMVector3Length(XMVectorSubtract(XMVectorLerp(b0, b, lerp2), XMVectorLerp(a0, a, lerp2))));
		XMVECTOR rot = XMQuaternionIdentity();

		shadowCam.push_back(SHCAM(size, rot, 0, wiRenderer::getCamera()->zFarP));
		shadowCam.push_back(SHCAM(size1, rot, 0, wiRenderer::getCamera()->zFarP));
		shadowCam.push_back(SHCAM(size2, rot, 0, wiRenderer::getCamera()->zFarP));

	}
	else if (type == Light::SPOT && shadow) {
		shadowCam.push_back(SHCAM(XMFLOAT4(0, 0, 0, 1), wiRenderer::getCamera()->zNearP, enerDis.y, enerDis.z));
	}
	else if (type == Light::POINT && shadow) {
		shadowCam.push_back(SHCAM(XMFLOAT4(0.5f, -0.5f, -0.5f, -0.5f), wiRenderer::getCamera()->zNearP, enerDis.y, XM_PI / 2.0f)); //+x
		shadowCam.push_back(SHCAM(XMFLOAT4(0.5f, 0.5f, 0.5f, -0.5f), wiRenderer::getCamera()->zNearP, enerDis.y, XM_PI / 2.0f)); //-x

		shadowCam.push_back(SHCAM(XMFLOAT4(1, 0, 0, -0), wiRenderer::getCamera()->zNearP, enerDis.y, XM_PI / 2.0f)); //+y
		shadowCam.push_back(SHCAM(XMFLOAT4(0, 0, 0, -1), wiRenderer::getCamera()->zNearP, enerDis.y, XM_PI / 2.0f)); //-y

		shadowCam.push_back(SHCAM(XMFLOAT4(0.707f, 0, 0, -0.707f), wiRenderer::getCamera()->zNearP, enerDis.y, XM_PI / 2.0f)); //+z
		shadowCam.push_back(SHCAM(XMFLOAT4(0, 0.707f, 0.707f, 0), wiRenderer::getCamera()->zNearP, enerDis.y, XM_PI / 2.0f)); //-z
	}
}
void Light::UpdateTransform()
{
	Transform::UpdateTransform();
}
void Light::UpdateLight()
{
	//Shadows
	if (type == Light::DIRECTIONAL) {

		float lerp = 0.5f;//third slice distance from cam (percentage)
		float lerp1 = 0.12f;//second slice distance from cam (percentage)
		float lerp2 = 0.016f;//first slice distance from cam (percentage)
		XMVECTOR c, d, e, e1, e2;
		c = XMVector3Unproject(XMVectorSet((float)wiRenderer::GetDevice()->GetScreenWidth() * 0.5f, (float)wiRenderer::GetDevice()->GetScreenHeight() * 0.5f, 1, 1), 0, 0, (float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());
		d = XMVector3Unproject(XMVectorSet((float)wiRenderer::GetDevice()->GetScreenWidth() * 0.5f, (float)wiRenderer::GetDevice()->GetScreenHeight() * 0.5f, 0, 1), 0, 0, (float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 0.1f, 1.0f, wiRenderer::getCamera()->GetProjection(), wiRenderer::getCamera()->GetView(), XMMatrixIdentity());

		if (!shadowCam.empty()) {

			float f = shadowCam[0].size / (float)wiRenderer::SHADOWMAPRES;
			e = XMVectorFloor(XMVectorLerp(d, c, lerp) / f)*f;
			f = shadowCam[1].size / (float)wiRenderer::SHADOWMAPRES;
			e1 = XMVectorFloor(XMVectorLerp(d, c, lerp1) / f)*f;
			f = shadowCam[2].size / (float)wiRenderer::SHADOWMAPRES;
			e2 = XMVectorFloor(XMVectorLerp(d, c, lerp2) / f)*f;

			XMMATRIX rrr = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation_rest));
			shadowCam[0].Update(rrr*XMMatrixTranslationFromVector(e));
			if (shadowCam.size()>1) {
				shadowCam[1].Update(rrr*XMMatrixTranslationFromVector(e1));
				if (shadowCam.size()>2)
					shadowCam[2].Update(rrr*XMMatrixTranslationFromVector(e2));
			}
		}

		bounds.createFromHalfWidth(wiRenderer::getCamera()->translation, XMFLOAT3(10000, 10000, 10000));
	}
	else if (type == Light::SPOT) {
		if (!shadowCam.empty()) {
			shadowCam[0].Update(XMLoadFloat4x4(&world));
		}

		bounds.createFromHalfWidth(translation, XMFLOAT3(enerDis.y, enerDis.y, enerDis.y));
	}
	else if (type == Light::POINT) {
		for (unsigned int i = 0; i<shadowCam.size(); ++i) {
			shadowCam[i].Update(XMLoadFloat3(&translation));
		}

		bounds.createFromHalfWidth(translation, XMFLOAT3(enerDis.y, enerDis.y, enerDis.y));
	}
}
#pragma endregion