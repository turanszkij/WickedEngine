#include "WickedLoader.h"
#include "ResourceManager.h"
#include "WickedHelper.h"
#include "WickedMath.h"
#include "Renderer.h"
#include "EmittedParticle.h"
#include "HairParticle.h"
#include "RenderTarget.h"
#include "DepthTarget.h"


void Mesh::LoadFromFile(const string& newName, const string& fname
						, const MaterialCollection& materialColl, vector<Armature*> armatures, const string& identifier){
	name=newName;

	BYTE* buffer;
	size_t fileSize;
	if (wiHelper::readByteData(fname, &buffer, fileSize)){

		int offset=0;

		int VERSION;
		memcpy(&VERSION,buffer,sizeof(int));
		offset+=sizeof(int);

		if(VERSION>=1001){
			int doubleside;
			memcpy(&doubleside,buffer+offset,sizeof(int));
			offset+=sizeof(int);
			if(doubleside){
				doubleSided=true;
			}
		}

		int billboard;
		memcpy(&billboard,buffer+offset,sizeof(int));
		offset+=sizeof(int);
		if(billboard){
			char axis;
			memcpy(&axis,buffer+offset,1);
			offset+=1;

			if(toupper(axis)=='Y') 
				billboardAxis=XMFLOAT3(0,1,0);
			else if(toupper(axis)=='X') 
				billboardAxis=XMFLOAT3(1,0,0);
			else if(toupper(axis)=='Z') 
				billboardAxis=XMFLOAT3(0,0,1);
			else 
				billboardAxis=XMFLOAT3(0,0,0);
			isBillboarded=true;
		}

		int parented; //parentnamelength
		memcpy(&parented,buffer+offset,sizeof(int));
		offset+=sizeof(int);
		if(parented){
			char* pName = new char[parented+1]();
			memcpy(pName,buffer+offset,parented);
			offset+=parented;
			parent=pName;
			delete[] pName;

			stringstream identified_parent("");
			identified_parent<<parent<<identifier;
			for(Armature* a : armatures){
				if(!a->name.compare(identified_parent.str())){
					armature=a;
				}
			}
		}

		int materialCount;
		memcpy(&materialCount,buffer+offset,sizeof(int));
		offset+=sizeof(int);
		for(int i=0;i<materialCount;++i){
			int matNameLen;
			memcpy(&matNameLen,buffer+offset,sizeof(int));
			offset+=sizeof(int);
			char* matName = new char[matNameLen+1]();
			memcpy(matName,buffer+offset,matNameLen);
			offset+=matNameLen;

			stringstream identified_matname("");
			identified_matname<<matName<<identifier;
			MaterialCollection::const_iterator iter = materialColl.find(identified_matname.str());
			if(iter!=materialColl.end()){
				materials.push_back(iter->second);
			}
			
			materialNames.push_back(identified_matname.str());
			delete[] matName;
		}
		int rendermesh,vertexCount;
		memcpy(&rendermesh,buffer+offset,sizeof(int));
		offset+=sizeof(int);
		memcpy(&vertexCount,buffer+offset,sizeof(int));
		offset+=sizeof(int);

		vertices.reserve(vertexCount);
		for(int i=0;i<vertexCount;++i){
			SkinnedVertex vert = SkinnedVertex();
			float v[8];
			memcpy(v,buffer+offset,sizeof(float)*8);
			offset+=sizeof(float)*8;
			vert.pos.x=v[0];
			vert.pos.y=v[1];
			vert.pos.z=v[2];
			if(!isBillboarded){
				vert.nor.x=v[3];
				vert.nor.y=v[4];
				vert.nor.z=v[5];
			}
			else{
				vert.nor=billboardAxis;
			}
			vert.tex.x=v[6];
			vert.tex.y=v[7];
			int matIndex;
			memcpy(&matIndex,buffer+offset,sizeof(int));
			offset+=sizeof(int);
			vert.tex.z=matIndex;
			
			int weightCount=0;
			memcpy(&weightCount,buffer+offset,sizeof(int));
			offset+=sizeof(int);
			for(int j=0;j<weightCount;++j){
				
				int weightNameLen=0;
				memcpy(&weightNameLen,buffer+offset,sizeof(int));
				offset+=sizeof(int);
				char* weightName = new char[weightNameLen+1](); //bone name
				memcpy(weightName,buffer+offset,weightNameLen);
				offset+=weightNameLen;
				float weightValue=0;
				memcpy(&weightValue,buffer+offset,sizeof(float));
				offset+=sizeof(float);
				
#pragma region BONE INDEX SETUP
				string nameB = weightName;
				if(armature){
					bool gotBone=false;
					int BONEINDEX=0;
					int b=0;
					while(!gotBone && b<armature->boneCollection.size()){
						if(!armature->boneCollection[b]->name.compare(nameB)){
							gotBone=true;
							BONEINDEX=b; //GOT INDEX OF BONE OF THE WEIGHT IN THE PARENT ARMATURE
						}
						b++;
					}
					if(gotBone){ //ONLY PROCEED IF CORRESPONDING BONE WAS FOUND
						if(!vert.wei.x) {
							vert.wei.x=weightValue;
							vert.bon.x=BONEINDEX;
						}
						else if(!vert.wei.y) {
							vert.wei.y=weightValue;
							vert.bon.y=BONEINDEX;
						}
						else if(!vert.wei.z) {
							vert.wei.z=weightValue;
							vert.bon.z=BONEINDEX;
						}
						else if(!vert.wei.w) {
							vert.wei.w=weightValue;
							vert.bon.w=BONEINDEX;
						}
					}
				}

					//(+RIBBONTRAIL SETUP)(+VERTEXGROUP SETUP)

				if(nameB.find("trailbase")!=string::npos)
					trailInfo.base = vertices.size();
				else if(nameB.find("trailtip")!=string::npos)
					trailInfo.tip = vertices.size();
						
				bool windAffection=false;
				if(nameB.find("wind")!=string::npos)
					windAffection=true;
				bool gotvg=false;
				for(int v=0;v<vertexGroups.size();++v)
					if(!nameB.compare(vertexGroups[v].name)){
						gotvg=true;
						vertexGroups[v].addVertex( VertexRef(vertices.size(),weightValue) );
						if(windAffection)
							vert.tex.w=weightValue;
					}
				if(!gotvg){
					vertexGroups.push_back(VertexGroup(nameB));
					vertexGroups.back().addVertex( VertexRef(vertices.size(),weightValue) );
					if(windAffection)
						vert.tex.w=weightValue;
				}
#pragma endregion

				delete[] weightName;

				
			}

			
			vertices.push_back(vert);
		}

		if(rendermesh){
			int indexCount;
			memcpy(&indexCount,buffer+offset,sizeof(int));
			offset+=sizeof(int);
			unsigned int* indexArray = new unsigned int[indexCount];
			memcpy(indexArray,buffer+offset,sizeof(unsigned int)*indexCount);
			offset+=sizeof(unsigned int)*indexCount;
			indices.reserve(indexCount);
			for(int i=0;i<indexCount;++i){
				indices.push_back(indexArray[i]);
			}
			delete[] indexArray;

			int softBody;
			memcpy(&softBody,buffer+offset,sizeof(int));
			offset+=sizeof(int);
			if(softBody){
				int softCount[2]; //ind,vert
				memcpy(softCount,buffer+offset,sizeof(int)*2);
				offset+=sizeof(int)*2;
				unsigned int* softind = new unsigned int[softCount[0]];
				memcpy(softind,buffer+offset,sizeof(unsigned int)*softCount[0]);
				offset+=sizeof(unsigned int)*softCount[0];
				float* softvert = new float[softCount[1]];
				memcpy(softvert,buffer+offset,sizeof(float)*softCount[1]);
				offset+=sizeof(float)*softCount[1];

				physicsindices.reserve(softCount[0]);
				physicsverts.reserve(softCount[1]/3);
				for(int i=0;i<softCount[0];++i){
					physicsindices.push_back(softind[i]);
				}
				for(int i=0;i<softCount[1];i+=3){
					physicsverts.push_back(XMFLOAT3(softvert[i],softvert[i+1],softvert[i+2]));
				}

				delete[] softind;
				delete[] softvert;
			}
			else{

			}
		}
		else{

		}

		memcpy(aabb.corners,buffer+offset,sizeof(aabb.corners));
		offset+=sizeof(aabb.corners);

		int isSoftbody;
		memcpy(&isSoftbody,buffer+offset,sizeof(int));
		offset+=sizeof(int);
		if(isSoftbody){
			float prop[2]; //mass,friction
			memcpy(prop,buffer+offset,sizeof(float)*2);
			offset+=sizeof(float)*2;
			softBody=true;
			mass=prop[0];
			friction=prop[1];
			int vglenghts[3]; //goal,mass,soft
			memcpy(vglenghts,buffer+offset,sizeof(int)*3);
			offset+=sizeof(int)*3;

			char* vgg = new char[vglenghts[0]+1]();
			char* vgm = new char[vglenghts[1]+1]();
			char* vgs = new char[vglenghts[2]+1]();
			
			memcpy(vgg,buffer+offset,vglenghts[0]);
			offset+=vglenghts[0];
			memcpy(vgm,buffer+offset,vglenghts[1]);
			offset+=vglenghts[1];
			memcpy(vgs,buffer+offset,vglenghts[2]);
			offset+=vglenghts[2];

			for(int v=0;v<vertexGroups.size();++v){
				if(!strcmp(vgm,vertexGroups[v].name.c_str()))
					massVG=v;
				if(!strcmp(vgg,vertexGroups[v].name.c_str()))
					goalVG=v;
				if(!strcmp(vgs,vertexGroups[v].name.c_str()))
					softVG=v;
			}

			delete[]vgg;
			delete[]vgm;
			delete[]vgs;
		}

		delete[] buffer;

		renderable=(bool)rendermesh;

		CreateBuffers();
	}
}
void Mesh::CreateBuffers(){
	if(!buffersComplete){

		usedBy.resize(1);

		for(int i=0;i<5;++i){
			instances[i].clear();
			instances[i].resize(usedBy.size());
		}

		if(meshInstanceBuffer!=nullptr)
			return;

		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof( Instance )*usedBy.size();
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &meshInstanceBuffer );

		//bool armatureDeformedMesh=false;
		//for(int u : usedBy){
		//	if(objects[u]->armatureDeform)
		//		armatureDeformedMesh=true;
		//}
		//if(!armatureDeformedMesh)
		//	armature=nullptr;


		if(goalVG>=0){
			goalPositions.resize(vertexGroups[goalVG].vertices.size());
			goalNormals.resize(vertexGroups[goalVG].vertices.size());
		}
		skinnedVertices.resize(vertices.size());
		for(int i=0;i<vertices.size();++i){
			skinnedVertices[i].pos=vertices[i].pos;
			skinnedVertices[i].nor=vertices[i].nor;
			skinnedVertices[i].tex=vertices[i].tex;
		}


			ZeroMemory( &bd, sizeof(bd) );
	#ifdef USE_GPU_SKINNING
			bd.Usage = (softBody?D3D11_USAGE_DYNAMIC:D3D11_USAGE_IMMUTABLE);
			bd.CPUAccessFlags = (softBody?D3D11_CPU_ACCESS_WRITE:0);
			if(hasArmature() && !softBody)
				bd.ByteWidth = sizeof( SkinnedVertex ) * vertices.size();
			else
				bd.ByteWidth = sizeof( Vertex ) * vertices.size();
	#else
			bd.Usage = ((softBody || hasArmature())?D3D11_USAGE_DYNAMIC:D3D11_USAGE_IMMUTABLE);
			bd.CPUAccessFlags = ((softBody || hasArmature())?D3D11_CPU_ACCESS_WRITE:0);
			bd.ByteWidth = sizeof( Vertex ) * vertices.size();
	#endif
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			D3D11_SUBRESOURCE_DATA InitData;
			ZeroMemory( &InitData, sizeof(InitData) );
			if(hasArmature() && !softBody)
				InitData.pSysMem = vertices.data();
			else
				InitData.pSysMem = skinnedVertices.data();
			wiRenderer::graphicsDevice->CreateBuffer( &bd, &InitData, &meshVertBuff );
		
			
			ZeroMemory( &bd, sizeof(bd) );
			bd.Usage = D3D11_USAGE_IMMUTABLE;
			bd.ByteWidth = sizeof( unsigned int ) * indices.size();
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;
			ZeroMemory( &InitData, sizeof(InitData) );
			InitData.pSysMem = indices.data();
			wiRenderer::graphicsDevice->CreateBuffer( &bd, &InitData, &meshIndexBuff );
			
		if(renderable)
		{
		
			ZeroMemory( &bd, sizeof(bd) );
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.ByteWidth = sizeof(BoneShaderBuffer);
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &boneBuffer );

			if(hasArmature() && !softBody){
				ZeroMemory( &bd, sizeof(bd) );
				bd.Usage = D3D11_USAGE_DEFAULT;
				bd.ByteWidth = sizeof(Vertex) * vertices.size();
				bd.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER;
				bd.CPUAccessFlags = 0;
				bd.StructureByteStride=0;
				wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &sOutBuffer );
			}

			//PHYSICALMAPPING
			for(int i=0;i<vertices.size();++i){
				for(int j=0;j<physicsverts.size();++j){
					if(		fabs( vertices[i].pos.x-physicsverts[j].x ) < DBL_EPSILON
						&&	fabs( vertices[i].pos.y-physicsverts[j].y ) < DBL_EPSILON
						&&	fabs( vertices[i].pos.z-physicsverts[j].z ) < DBL_EPSILON
						)
					{
						physicalmapGP.push_back(j);
						break;
					}
				}
			}
		}


		buffersComplete=true;
	}

}
void Mesh::AddInstance(int count){
	usedBy.resize(usedBy.size()+count);
	for(int i=0;i<5;++i){
		instances[i].clear();
		instances[i].resize(usedBy.size());
	}
	wiRenderer::ResizeBuffer<Instance>(meshInstanceBuffer,usedBy.size()*2);
}

void LoadWiArmatures(const string& directory, const string& name, const string& identifier, vector<Armature*>& armatures
					 , map<string,Transform*>& transforms)
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
		//for(int i=0;i<armature->boneCollection.size();i++){
		//	string parent = armature->boneCollection[i]->parentName;
		//	if(parent.length()>0){
		//		for(int j=0;j<armature->boneCollection.size();j++)
		//			if(!armature->boneCollection[j].name.compare(parent)){
		//				armature->boneCollection[i].parentI=j;
		//				armature->boneCollection[j].children.push_back(armature->boneCollection[i].name);
		//				armature->boneCollection[j].childrenI.push_back(i);
		//			}
		//	}
		//	else{
		//		armature->rootbones.push_back(i);
		//	}
		//}

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

		for(int i=0;i<armature->rootbones.size();++i){
			RecursiveRest(armature,armature->rootbones[i]);
		}

		transforms.insert(pair<string,Transform*>(armature->name,armature));
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

	for(int i=0;i<bone->childrenI.size();++i){
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
				if(currentMat)
					materials.insert(pair<string,Material*>(currentMat->name,currentMat));
				
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
						currentMat->refMap=(ID3D11ShaderResourceView*)wiResourceManager::add(ss.str());
					}
					if(currentMat->refMap!=0)
						currentMat->hasRefMap = true;
					break;
				case 'n':
					{
						string resourceName="";
						file>>resourceName;
						stringstream ss("");
						ss<<directory<<texturesDir<<resourceName.c_str();
						currentMat->normalMapName=ss.str();
						currentMat->normalMap=(ID3D11ShaderResourceView*)wiResourceManager::add(ss.str());
					}
					if(currentMat->normalMap!=0)
						currentMat->hasNormalMap = true;
					break;
				case 't':
					{
						string resourceName="";
						file>>resourceName;
						stringstream ss("");
						ss<<directory<<texturesDir<<resourceName.c_str();
						currentMat->textureName=ss.str();
						currentMat->texture=(ID3D11ShaderResourceView*)wiResourceManager::add(ss.str());
					}
					if(currentMat->texture!=0)
						currentMat->hasTexture=true;
					file>>currentMat->premultipliedTexture;
					break;
				case 'D':
					{
						string resourceName="";
						file>>resourceName;
						stringstream ss("");
						ss<<directory<<texturesDir<<resourceName.c_str();
						currentMat->displacementMapName=ss.str();
						currentMat->displacementMap=(ID3D11ShaderResourceView*)wiResourceManager::add(ss.str());
					}
					if(currentMat->displacementMap!=0)
						currentMat->hasDisplacementMap=true;
					break;
				case 'S':
					{
						string resourceName="";
						file>>resourceName;
						stringstream ss("");
						ss<<directory<<texturesDir<<resourceName.c_str();
						currentMat->specularMapName=ss.str();
						currentMat->specularMap=(ID3D11ShaderResourceView*)wiResourceManager::add(ss.str());
					}
					if(currentMat->specularMap!=0)
						currentMat->hasSpecularMap=true;
					break;
				case 'a':
					currentMat->transparent=true;
					file>>currentMat->alpha;
					break;
				case 'h':
					currentMat->shadeless=true;
					break;
				case 'R':
					file>>currentMat->refraction_index;
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
					currentMat->subsurface_scattering=true;
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
						file>>currentMat->emit;
					}
					break;
				default:break;
				}
			}
		}
	}
	file.close();
	
	if(currentMat)
		materials.insert(pair<string,Material*>(currentMat->name,currentMat));

}
void LoadWiObjects(const string& directory, const string& name, const string& identifier, vector<Object*>& objects_norm
				   , vector<Object*>& objects_trans, vector<Object*>& objects_water, vector<Armature*>& armatures
				   , MeshCollection& meshes, map<string,Transform*>& transforms, const MaterialCollection& materials)
{
	vector<Object*> objects(0);
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
				transforms.insert(pair<string,Transform*>(objects.back()->name,objects.back()));
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
								objects.back()->mesh->AddInstance(1);
							}
						}
						else{
							if(iter!=meshes.end()) {
								objects.back()->mesh=iter->second;
								objects.back()->mesh->usedBy.push_back(objects.size()-1);
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
						for(Armature* a : armatures){
							if(!a->name.compare(identified_parentName.str())){
								objects.back()->parentName=identified_parentName.str();
								objects.back()->parent=a;
								objects.back()->attachTo(a,1,1,1);
								objects.back()->armatureDeform=true;
							}
						}
					}
					break;
				case 'b':
					{
						string bone="";
						file>>bone;
						if(objects.back()->parent!=nullptr){
							for(Bone* b : ((Armature*)objects.back()->parent)->boneCollection){
								if(!bone.compare(b->name)){
									objects.back()->parent=b;
									objects.back()->armatureDeform=false;
									break;
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
						XMStoreFloat4x4(&objects.back()->parent_inv_rest
								, XMMatrixScalingFromVector(XMLoadFloat3(&s)) *
									XMMatrixRotationQuaternion(XMLoadFloat4(&r)) *
									XMMatrixTranslationFromVector(XMLoadFloat3(&t))
							);
					}
					break;
				case 'r':
					file>>trans[0]>>trans[1]>>trans[2]>>trans[3];
					objects.back()->rotation_rest = XMFLOAT4(trans[0],trans[1],trans[2],trans[3]);
					break;
				case 's':
					file>>trans[0]>>trans[1]>>trans[2];
					objects.back()->scale_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					break;
				case 't':
					file>>trans[0]>>trans[1]>>trans[2];
					objects.back()->translation_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					XMStoreFloat4x4( &objects.back()->world_rest, XMMatrixScalingFromVector(XMLoadFloat3(&objects.back()->scale_rest))
																*XMMatrixRotationQuaternion(XMLoadFloat4(&objects.back()->rotation_rest))
																*XMMatrixTranslationFromVector(XMLoadFloat3(&objects.back()->translation_rest))
															);
					objects.back()->world=objects.back()->world_rest;
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
						
						if(visibleEmitter) objects.back()->particleEmitter=Object::EMITTER_VISIBLE;
						else if(objects.back()->particleEmitter==Object::NO_EMITTER) objects.back()->particleEmitter=Object::EMITTER_INVISIBLE;

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
							objects.back()->hwiParticleSystems.push_back(new wiHairParticle(name,len,count,identified_materialName.str(),objects.back(),densityG,lenG) );
						}
					}
					break;
				case 'P':
					objects.back()->rigidBody = true;
					file>>objects.back()->collisionShape>>objects.back()->mass>>
						objects.back()->friction>>objects.back()->restitution>>objects.back()->damping>>objects.back()->physicsType>>
						objects.back()->kinematic;
					break;
				default: break;
				}
			}
		}
	}
	file.close();

	for(int i=0;i<objects.size();i++){
		if(objects[i]->mesh){
			if(objects[i]->mesh->trailInfo.base>=0 && objects[i]->mesh->trailInfo.tip>=0){
					//objects[i]->trail.resize(MAX_RIBBONTRAILS);
					D3D11_BUFFER_DESC bd;
					ZeroMemory( &bd, sizeof(bd) );
					bd.Usage = D3D11_USAGE_DYNAMIC;
					bd.ByteWidth = sizeof( RibbonVertex ) * MAX_RIBBONTRAILS;
					bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
					bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &objects[i]->trailBuff );
			}
		
			/*string parent = objects[i]->parent;
			bool childofArmature=true;
			if(parent.length()){
				for(int j=0;j<objects.size();j++)
					if(objects[j]->name == parent){
						objects[j]->children.push_back(objects[i]->name);
						objects[j]->childrenI.push_back(i);
						childofArmature=false;
					}
			}*/

			bool default_mesh = false;
			bool water_mesh = false;
			bool transparent_mesh = false;

			//if(objects[i]->mesh->renderable)
				for(Material* mat : objects[i]->mesh->materials){
					if(!mat->water && !mat->isSky && !mat->transparent)
						default_mesh=true;
					if(mat->water && !mat->isSky)
						water_mesh=true;
					if(!mat->water && !mat->isSky && mat->transparent)
						transparent_mesh=true;
				}
		
			if(default_mesh)
				objects_norm.push_back(objects[i]);
			if(water_mesh)
				objects_water.push_back(objects[i]);
			if(transparent_mesh)
				objects_trans.push_back(objects[i]);

			if(!default_mesh && !water_mesh && !transparent_mesh)
				objects_norm.push_back(objects[i]);
		}
	}

	for(MeshCollection::iterator iter=meshes.begin(); iter!=meshes.end(); ++iter){
		Mesh* iMesh = iter->second;

		if(iMesh->buffersComplete)
			continue;
		
		for(int i=0;i<5;++i){
			iMesh->instances[i].clear();
			iMesh->instances[i].resize(iMesh->usedBy.size());
		}

		if(iMesh->meshInstanceBuffer!=nullptr)
			continue;

		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof( Instance )*iMesh->usedBy.size();
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &iMesh->meshInstanceBuffer );

		bool armatureDeformedMesh=false;
		for(int u : iMesh->usedBy){
			if(objects[u]->armatureDeform)
				armatureDeformedMesh=true;
		}
		if(!armatureDeformedMesh)
			iMesh->armature=nullptr;


		if(iMesh->goalVG>=0){
			iMesh->goalPositions.resize(iMesh->vertexGroups[iMesh->goalVG].vertices.size());
			iMesh->goalNormals.resize(iMesh->vertexGroups[iMesh->goalVG].vertices.size());
		}
		iMesh->skinnedVertices.resize(iMesh->vertices.size());
		for(int i=0;i<iMesh->vertices.size();++i){
			iMesh->skinnedVertices[i].pos=iMesh->vertices[i].pos;
			iMesh->skinnedVertices[i].nor=iMesh->vertices[i].nor;
			iMesh->skinnedVertices[i].tex=iMesh->vertices[i].tex;
		}


			ZeroMemory( &bd, sizeof(bd) );
	#ifdef USE_GPU_SKINNING
			bd.Usage = (iMesh->softBody?D3D11_USAGE_DYNAMIC:D3D11_USAGE_IMMUTABLE);
			bd.CPUAccessFlags = (iMesh->softBody?D3D11_CPU_ACCESS_WRITE:0);
			if(iMesh->hasArmature() && !iMesh->softBody)
				bd.ByteWidth = sizeof( SkinnedVertex ) * iMesh->vertices.size();
			else
				bd.ByteWidth = sizeof( Vertex ) * iMesh->vertices.size();
	#else
			bd.Usage = ((iMesh->softBody || iMesh->hasArmature())?D3D11_USAGE_DYNAMIC:D3D11_USAGE_IMMUTABLE);
			bd.CPUAccessFlags = ((iMesh->softBody || iMesh->hasArmature())?D3D11_CPU_ACCESS_WRITE:0);
			bd.ByteWidth = sizeof( Vertex ) * iMesh->vertices.size();
	#endif
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			D3D11_SUBRESOURCE_DATA InitData;
			ZeroMemory( &InitData, sizeof(InitData) );
			if(iMesh->hasArmature() && !iMesh->softBody)
				InitData.pSysMem = iMesh->vertices.data();
			else
				InitData.pSysMem = iMesh->skinnedVertices.data();
			wiRenderer::graphicsDevice->CreateBuffer( &bd, &InitData, &iMesh->meshVertBuff );
		
			
			ZeroMemory( &bd, sizeof(bd) );
			bd.Usage = D3D11_USAGE_IMMUTABLE;
			bd.ByteWidth = sizeof( unsigned int ) * iMesh->indices.size();
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;
			ZeroMemory( &InitData, sizeof(InitData) );
			InitData.pSysMem = iMesh->indices.data();
			wiRenderer::graphicsDevice->CreateBuffer( &bd, &InitData, &iMesh->meshIndexBuff );
			
		if(iMesh->renderable)
		{
		
			ZeroMemory( &bd, sizeof(bd) );
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.ByteWidth = sizeof(BoneShaderBuffer);
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &iMesh->boneBuffer );

			if(iMesh->hasArmature() && !iMesh->softBody){
				ZeroMemory( &bd, sizeof(bd) );
				bd.Usage = D3D11_USAGE_DEFAULT;
				bd.ByteWidth = sizeof(Vertex) * iMesh->vertices.size();
				bd.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER;
				bd.CPUAccessFlags = 0;
				bd.StructureByteStride=0;
				wiRenderer::graphicsDevice->CreateBuffer( &bd, NULL, &iMesh->sOutBuffer );
			}

			//PHYSICALMAPPING
			for(int i=0;i<iMesh->vertices.size();++i){
				for(int j=0;j<iMesh->physicsverts.size();++j){
					if(		fabs( iMesh->vertices[i].pos.x-iMesh->physicsverts[j].x ) < DBL_EPSILON
						&&	fabs( iMesh->vertices[i].pos.y-iMesh->physicsverts[j].y ) < DBL_EPSILON
						&&	fabs( iMesh->vertices[i].pos.z-iMesh->physicsverts[j].z ) < DBL_EPSILON
						)
					{
						iMesh->physicalmapGP.push_back(j);
						break;
					}
				}
			}

		}
	}
	objects.clear();
}
void LoadWiMeshes(const string& directory, const string& name, const string& identifier, MeshCollection& meshes, const vector<Armature*>& armatures, const MaterialCollection& materials)
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
						for(int i=0;i<armatures.size();++i)
							if(!strcmp(armatures[i]->name.c_str(),currentMesh->parent.c_str())){
								currentMesh->armatureIndex=i;
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
					if(currentMesh->isBillboarded){
						currentMesh->vertices.back().nor=currentMesh->billboardAxis;
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
						while(!gotArmature && i<armatures.size()){  //SEARCH FOR PARENT ARMATURE
							if(!strcmp(armatures[i]->name.c_str(),currentMesh->parent.c_str()))
								gotArmature=true;
							else i++;
						}
						if(gotArmature){
							int j=0;
							while(!gotBone && j<armatures[i]->boneCollection.size()){
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
								currentMesh->vertices.back().bon.x=BONEINDEX;
							}
							else if(!currentMesh->vertices.back().wei.y) {
								currentMesh->vertices.back().wei.y=weight;
								currentMesh->vertices.back().bon.y=BONEINDEX;
							}
							else if(!currentMesh->vertices.back().wei.z) {
								currentMesh->vertices.back().wei.z=weight;
								currentMesh->vertices.back().bon.z=BONEINDEX;
							}
							else if(!currentMesh->vertices.back().wei.w) {
								currentMesh->vertices.back().wei.w=weight;
								currentMesh->vertices.back().bon.w=BONEINDEX;
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
						for(int v=0;v<currentMesh->vertexGroups.size();++v)
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
						for(int v=0;v<currentMesh->vertexGroups.size();++v){
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
				for(int i=0;i<armatures.size();i++)
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
						for(int i=0;i<armatures[armatureI]->boneCollection.size();i++)
							if(!armatures[armatureI]->boneCollection[i]->name.compare(boneName)){
								boneI=i;
								break;
							} //GOT BONE INDEX
						armatures[armatureI]->boneCollection[boneI]->actionFrames.resize(armatures[armatureI]->actions.size());
					}
					break;
				case 'r':
					{
						float f=0,x=0,y=0,z=0,w=0;
						file>>f>>x>>y>>z>>w;
						armatures[armatureI]->boneCollection[boneI]->actionFrames.back().keyframesRot.push_back(KeyFrame(f,x,y,z,w));
					}
					break;
				case 't':
					{
						float f=0,x=0,y=0,z=0;
						file>>f>>x>>y>>z;
						armatures[armatureI]->boneCollection[boneI]->actionFrames.back().keyframesPos.push_back(KeyFrame(f,x,y,z,0));
					}
					break;
				case 's':
					{
						float f=0,x=0,y=0,z=0;
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
void LoadWiLights(const string& directory, const string& name, const string& identifier, vector<Light*>& lights
				  , const vector<Armature*>& armatures
				  , map<string,Transform*>& transforms)
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
					bool shadow;
					string lname = "";
					file>>lname>>shadow;
					stringstream identified_name("");
					identified_name<<lname<<identifier;
					lights.back()->name=identified_name.str();
					if(shadow){
						lights.back()->shadowMap.resize(1);
						lights.back()->shadowMap[0].InitializeCube(wiRenderer::POINTLIGHTSHADOWRES,0,true);
					}
					//lights.back()->mesh=lightGwiRenderer[Light::getTypeStr(Light::POINT)];
					
					transforms.insert(pair<string,Transform*>(lights.back()->name,lights.back()));
				}
				break;
			case 'D':
				{
					lights.push_back(new Light());
					lights.back()->type=Light::DIRECTIONAL;
					file>>lights.back()->name;
					lights.back()->shadowMap.resize(3);
					for(int i=0;i<3;++i)
						lights.back()->shadowMap[i].Initialize(
							wiRenderer::SHADOWMAPRES,wiRenderer::SHADOWMAPRES
							,0,true
							);
					//lightGwiRenderer[Light::getTypeStr(Light::DIRECTIONAL)]->usedBy.push_back(lights.size()-1);
					//lights.back()->mesh=lightGwiRenderer[Light::getTypeStr(Light::DIRECTIONAL)];
				}
				break;
			case 'S':
				{
					lights.push_back(new Light());
					lights.back()->type=Light::SPOT;
					file>>lights.back()->name;
					bool shadow;
					file>>shadow>>lights.back()->enerDis.z;
					if(shadow){
						lights.back()->shadowMap.resize(1);
						lights.back()->shadowMap[0].Initialize(
							wiRenderer::SHADOWMAPRES,wiRenderer::SHADOWMAPRES
							,0,true
							);
					}
					//lightGwiRenderer[Light::getTypeStr(Light::SPOT)]->usedBy.push_back(lights.size()-1);
					//lights.back()->mesh=lightGwiRenderer[Light::getTypeStr(Light::SPOT)];
				}
				break;
			case 'p':
				{
					string parentName="";
					file>>parentName;
					
					stringstream identified_parentName("");
					identified_parentName<<parentName<<identifier;
					lights.back()->parentName=identified_parentName.str();
					for(map<string,Transform*>::iterator it=transforms.begin();it!=transforms.end();++it){
						if(!it->second->name.compare(lights.back()->parentName)){
							lights.back()->parent=it->second;
							lights.back()->attachTo(it->second,1,1,1);
							break;
						}
					}
				}
				break;
			case 'b':
				{
					string parentBone="";
					file>>parentBone;

					for(Bone* b : ((Armature*)lights.back()->parent)->boneCollection){
						if(!b->name.compare(parentBone)){
							lights.back()->parent=b;
							lights.back()->attachTo(b,1,1,1);
						}
					}
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
					lights.back()->translation_rest=XMFLOAT3(x,y,z);
					break;
				}
			case 'r':
				{
					float x,y,z,w;
					file>>x>>y>>z>>w;
					lights.back()->rotation_rest=XMFLOAT4(x,y,z,w);
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
					wiRenderer::TextureView tex=nullptr;
					if((tex = (wiRenderer::TextureView)wiResourceManager::add(rim.str()))!=nullptr){
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
		//	D3D11_BUFFER_DESC bd;
		//	ZeroMemory( &bd, sizeof(bd) );
		//	bd.Usage = D3D11_USAGE_DYNAMIC;
		//	bd.ByteWidth = sizeof( Instance )*iMesh->usedBy.size();
		//	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		//	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		//	wiRenderer::graphicsDevice->CreateBuffer( &bd, 0, &iMesh->meshInstanceBuffer );
		//}
	}
	file.close();
}
void LoadWiHitSpheres(const string& directory, const string& name, const string& identifier, vector<HitSphere*>& spheres
					  ,const vector<Armature*>& armatures, map<string,Transform*>& transforms)
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

					Transform* parent = nullptr;
					if(transforms.find(identified_parent.str())!=transforms.end())
					{
						parent = transforms[identified_parent.str()];
						spheres.push_back(new HitSphere(identified_name.str(),scale,loc,parent,prop));
						spheres.back()->attachTo(parent,1,1,1);
						transforms.insert(pair<string,Transform*>(spheres.back()->name,spheres.back()));
					}

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
void LoadWiCameras(const string&directory, const string& name, const string& identifier, vector<ActionCamera>& cameras
				   ,const vector<Armature*>& armatures, map<string,Transform*>& transforms){
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
			
					cameras.push_back(ActionCamera(
						trans,rot
						,name)
						);

					for(int i=0;i<armatures.size();++i){
						if(!armatures[i]->name.compare(identified_parentArmature.str())){
							for(int j=0;j<armatures[i]->boneCollection.size();++j){
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

void LoadFromDisk(const string& dir, const string& name, const string& identifier
				  , vector<Armature*>& armatures
				  , MaterialCollection& materials
				  , vector<Object*>& objects_norm, vector<Object*>& objects_trans, vector<Object*>& objects_water
				  , MeshCollection& meshes
				  , vector<Light*>& lights
				  , vector<HitSphere*>& spheres
				  , WorldInfo& worldInfo, Wind& wind
				  , vector<ActionCamera>& cameras
				  , vector<Armature*>& l_armatures
				  , vector<Object*>& l_objects
				  , map<string,Transform*>& transforms
				  , list<Decal*>& decals
				  )
{
	MaterialCollection		l_materials;
	vector<Object*>			l_objects_norm;
	vector<Object*>			l_objects_trans;
	vector<Object*>			l_objects_water;
	MeshCollection			l_meshes;
	vector<Light*>			l_lights;
	vector<HitSphere*>		l_spheres;
	WorldInfo				l_worldInfo = worldInfo;
	Wind					l_wind = wind;
	vector<ActionCamera>	l_cameras;
	map<string,Transform*>  l_transforms;
	list<Decal*>			l_decals;

	stringstream directory(""),armatureFilePath(""),materialLibFilePath(""),meshesFilePath(""),objectsFilePath("")
		,actionsFilePath(""),lightsFilePath(""),worldInfoFilePath(""),enviroMapFilePath(""),hitSpheresFilePath("")
		,camerasFilePath(""),decalsFilePath("");

	directory<<dir;
	armatureFilePath<<name<<".wia";
	materialLibFilePath<<name<<".wim";
	meshesFilePath<<name<<".wi";
	objectsFilePath<<name<<".wio";
	actionsFilePath<<name<<".wiact";
	lightsFilePath<<name<<".wil";
	worldInfoFilePath<<name<<".wiw";
	hitSpheresFilePath<<name<<".wih";
	camerasFilePath<<name<<".wic";
	decalsFilePath<<name<<".wid";

	LoadWiArmatures(directory.str(), armatureFilePath.str(),identifier,l_armatures,l_transforms);
	LoadWiMaterialLibrary(directory.str(), materialLibFilePath.str(),identifier, "textures/", l_materials);
	LoadWiMeshes(directory.str(), meshesFilePath.str(),identifier,meshes,l_armatures,l_materials);
	LoadWiObjects(directory.str(), objectsFilePath.str(),identifier,l_objects_norm,l_objects_trans,l_objects_water,l_armatures,meshes,l_transforms,l_materials);
	LoadWiActions(directory.str(), actionsFilePath.str(),identifier,l_armatures);
	LoadWiLights(directory.str(), lightsFilePath.str(),identifier, l_lights, l_armatures,l_transforms);
	LoadWiHitSpheres(directory.str(), hitSpheresFilePath.str(),identifier,spheres,l_armatures,l_transforms);
	LoadWiCameras(directory.str(), camerasFilePath.str(),identifier,l_cameras,l_armatures,l_transforms);
	LoadWiWorldInfo(directory.str(), worldInfoFilePath.str(),l_worldInfo,l_wind);
	LoadWiDecals(directory.str(), decalsFilePath.str(), "textures/", l_decals);

	wiRenderer::graphicsMutex.lock();
	{
		armatures.insert(armatures.end(),l_armatures.begin(),l_armatures.end());
		objects_norm.insert(objects_norm.end(),l_objects_norm.begin(),l_objects_norm.end());
		objects_trans.insert(objects_trans.end(),l_objects_trans.begin(),l_objects_trans.end());
		objects_water.insert(objects_water.end(),l_objects_water.begin(),l_objects_water.end());
		lights.insert(lights.end(),l_lights.begin(),l_lights.end());
		spheres.insert(spheres.end(),l_spheres.begin(),l_spheres.end());
		cameras.insert(cameras.end(),l_cameras.begin(),l_cameras.end());

		meshes.insert(l_meshes.begin(),l_meshes.end());
		materials.insert(l_materials.begin(),l_materials.end());

		worldInfo=l_worldInfo;
		wind=l_wind;
		
		l_objects.insert(l_objects.end(),l_objects_norm.begin(),l_objects_norm.end());
		l_objects.insert(l_objects.end(),l_objects_trans.begin(),l_objects_trans.end());
		l_objects.insert(l_objects.end(),l_objects_water.begin(),l_objects_water.end());

		transforms.insert(l_transforms.begin(),l_transforms.end());

		decals.insert(decals.end(),l_decals.begin(),l_decals.end());
	}
	wiRenderer::graphicsMutex.unlock();
}

void Material::CleanUp(){
	/*if(refMap) refMap->Release();
	if(texture) texture->Release();
	if(normalMap) normalMap->Release();
	if(displacementMap) displacementMap->Release();
	if(specularMap) specularMap->Release();*/
	wiResourceManager::del(refMapName);
	wiResourceManager::del(textureName);
	wiResourceManager::del(normalMapName);
	wiResourceManager::del(displacementMapName);
	wiResourceManager::del(specularMapName);
	refMap=nullptr;
	texture=nullptr;
	normalMap=nullptr;
	displacementMap=nullptr;
	specularMap=nullptr;
}
void Light::CleanUp(){
	shadowCam.clear();
	shadowMap.clear();
	lensFlareRimTextures.clear();
	for(string x:lensFlareNames)
		wiResourceManager::del(x);
	lensFlareNames.clear();
}

void GeneratewiSPTree(wiSPTree*& tree, vector<Cullable*>& objects, int type){
	if(type==GENERATE_QUADTREE)
		tree = new QuadTree();
	else if(type==GENERATE_OCTREE)
		tree = new Octree();
	tree->initialize(objects);
}

Cullable::Cullable():bounds(AABB())/*,lastSquaredDistMulThousand(0)*/{}
Streamable::Streamable():directory(""),meshfile(""),materialfile(""),loaded(false){}

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

bool RAY::intersects(const AABB& box) const{
	return box.intersects(*this);
}

ID3D11Buffer *HitSphere::vertexBuffer=nullptr;
void HitSphere::SetUpStatic()
{
	const int numVert = (RESOLUTION+1)*2;
	vector<XMFLOAT3A> verts(0);

	for(int i=0;i<=RESOLUTION;++i){
		float alpha = (float)i/(float)RESOLUTION*2*3.14159265359f;
		verts.push_back(XMFLOAT3A(XMFLOAT3A(sin(alpha),cos(alpha),0)));
		verts.push_back(XMFLOAT3A(XMFLOAT3A(0,0,0)));
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof( XMFLOAT3A )*verts.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = verts.data();
	wiRenderer::graphicsDevice->CreateBuffer( &bd, &InitData, &vertexBuffer );
}
void HitSphere::CleanUpStatic()
{
	if(vertexBuffer){
		vertexBuffer->Release();
		vertexBuffer=NULL;
	}
}

#pragma region BONE
XMMATRIX Bone::getTransform(int getTranslation, int getRotation, int getScale){
	//XMMATRIX& w = XMMatrixTranslation(0,0,length)*XMLoadFloat4x4(&world);

	//if(getTranslation&&getRotation&&getScale){
	//	return w;
	//}
	//XMVECTOR v[3];
	//XMMATRIX& identity = XMMatrixIdentity();
	//XMMatrixDecompose(&v[0],&v[1],&v[2],w);

	//return  (getScale?XMMatrixScalingFromVector(v[0]):identity)*
	//		(getRotation?XMMatrixRotationQuaternion(v[1]):identity)*
	//		(getTranslation?XMMatrixTranslationFromVector(v[2]):identity)
	//		;

	return XMMatrixTranslation(0,0,length)*XMLoadFloat4x4(&world);
}
#pragma endregion

#pragma region TRANSFORM


XMMATRIX Transform::getTransform(int getTranslation, int getRotation, int getScale){
	worldPrev=world;
	translationPrev=translation;
	scalePrev=scale;
	rotationPrev=rotation;

	XMVECTOR s = XMLoadFloat3(&scale_rest);
	XMVECTOR r = XMLoadFloat4(&rotation_rest);
	XMVECTOR t = XMLoadFloat3(&translation_rest);
	XMMATRIX& w = 
		XMMatrixScalingFromVector(s)*
		XMMatrixRotationQuaternion(r)*
		XMMatrixTranslationFromVector(t)
		;
	XMStoreFloat4x4( &world_rest,w );

	if(parent!=nullptr){
		w = w * XMLoadFloat4x4(&parent_inv_rest) * parent->getTransform();
		XMVECTOR v[3];
		XMMatrixDecompose(&v[0],&v[1],&v[2],w);
		XMStoreFloat3( &scale,v[0] );
		XMStoreFloat4( &rotation,v[1] );
		XMStoreFloat3( &translation,v[2] );
		XMStoreFloat4x4( &world, w );
	}
	else{
		world = world_rest;
		translation=translation_rest;
		rotation=rotation_rest;
		scale=scale_rest;
	}

	return w;

	//XMVECTOR s = XMVectorSet(1,1,1,1);
	//XMVECTOR r = XMVectorSet(0,0,0,1);
	//XMVECTOR t = XMVectorSet(0,0,0,1);
	//if(getScale) 
	//	s = XMLoadFloat3(&scale_rest);
	//if(getRotation) 
	//	r = XMLoadFloat4(&rotation_rest);
	//if(getTranslation) 
	//	t = XMLoadFloat3(&translation_rest);
	//XMMATRIX& w = 
	//	XMMatrixScalingFromVector(s)*
	//	XMMatrixRotationQuaternion(r)*
	//	XMMatrixTranslationFromVector(t)
	//	;
	//XMStoreFloat4x4( &world_rest,w );

	//if(parent!=nullptr){
	//	w = w * XMLoadFloat4x4(&parent_inv_rest) * parent->getTransform(copyParentT,copyParentR,copyParentS);
	//	XMVECTOR v[3];
	//	XMMatrixDecompose(&v[0],&v[1],&v[2],w);
	//	XMStoreFloat3( &scale,v[0] );
	//	XMStoreFloat4( &rotation,v[1] );
	//	XMStoreFloat3( &translation,v[2] );
	//	XMStoreFloat4x4( &world, w );
	//}
	//else{
	//	world = world_rest;
	//	translation=translation_rest;
	//	rotation=rotation_rest;
	//	scale=scale_rest;
	//}

	//
	//return w;
}
//attach to parent
void Transform::attachTo(Transform* newParent, int copyTranslation, int copyRotation, int copyScale){
	if(newParent!=nullptr){
		parent=newParent;
		copyParentT=copyTranslation;
		copyParentR=copyRotation;
		copyParentS=copyScale;
		XMStoreFloat4x4( &parent_inv_rest, XMMatrixInverse( nullptr,parent->getTransform(copyParentT,copyParentR,copyParentS) ) );
		parent->children.insert(this);
	}
}
//detach child - detach all if no parameters
void Transform::detachChild(Transform* child){
	if(child==nullptr){
		for(Transform* c : children){
			if(c!=nullptr){
				c->detach(0);
			}
		}
		children.clear();
	}
	else{
		if( children.find(child)!=children.end() ){
			child->detach();
		}
	}
}
//detach from parent
void Transform::detach(int eraseFromParent){
	if(parent!=nullptr){
		if(eraseFromParent && parent->children.find(this)!=parent->children.end()){
			parent->children.erase(this);
		}
		applyTransform(copyParentT,copyParentR,copyParentS);
	}
	parent=nullptr;
}
void Transform::applyTransform(int t, int r, int s){
	if(t)
		translation_rest=translation;
	if(r)
		rotation_rest=rotation;
	if(s)
		scale_rest=scale;
}
void Transform::transform(const XMFLOAT3& t, const XMFLOAT4& r, const XMFLOAT3& s){
	translation_rest.x+=t.x;
	translation_rest.y+=t.y;
	translation_rest.z+=t.z;
	
	XMStoreFloat4(&rotation_rest,XMQuaternionMultiply(XMLoadFloat4(&rotation_rest),XMLoadFloat4(&r)));
	
	scale_rest.x*=s.x;
	scale_rest.y*=s.y;
	scale_rest.z*=s.z;
}
void Transform::transform(const XMMATRIX& m){
	XMVECTOR v[3];
	if(XMMatrixDecompose(&v[0],&v[1],&v[2],m)){
		XMFLOAT3 t,s;
		XMFLOAT4 r;
		XMStoreFloat3(&s,v[0]);
		XMStoreFloat4(&r,v[1]);
		XMStoreFloat3(&t,v[2]);
		transform(t,r,s);
	}
}

#pragma endregion

#pragma region Decals
Decal::Decal(const XMFLOAT3& tra, const XMFLOAT3& sca, const XMFLOAT4& rot, const string& tex, const string& nor):Cullable(),Transform(){
	scale_rest=scale=sca;
	rotation_rest=rotation=rot;
	translation_rest=translation=tra;

	Update();

	texture=normal=nullptr;
	addTexture(tex);
	addNormal(nor);

	life = -2; //persistent
	fadeStart=0;
}
void Decal::Update(){
	XMMATRIX rotMat = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
	XMVECTOR eye = XMLoadFloat3(&translation);
	XMVECTOR frontV = XMVector3Transform( XMVectorSet(0,0,1,0),rotMat );
	XMStoreFloat3(&front,frontV);
	XMVECTOR at = XMVectorAdd(eye,frontV);
	XMVECTOR up = XMVector3Transform( XMVectorSet(0,1,0,0),rotMat );
	XMStoreFloat4x4(&view, XMMatrixLookAtLH(eye,at,up));
	XMStoreFloat4x4(&projection, XMMatrixOrthographicLH(scale.x,scale.y,-scale.z,scale.z));
	XMStoreFloat4x4(&world_rest, XMMatrixScalingFromVector(XMLoadFloat3(&scale))*rotMat*XMMatrixTranslationFromVector(eye));

	bounds.createFromHalfWidth(XMFLOAT3(0,0,0),XMFLOAT3(scale.x,scale.y,scale.z));
	bounds = bounds.get(XMLoadFloat4x4(&world_rest));
}
void Decal::addTexture(const string& tex){
	texName=tex;
	if(!tex.empty()){
		texture=(ID3D11ShaderResourceView*)wiResourceManager::add(tex);
	}
}
void Decal::addNormal(const string& nor){
	norName=nor;
	if(!nor.empty()){
		normal=(ID3D11ShaderResourceView*)wiResourceManager::add(nor);
	}
}
void Decal::CleanUp(){
	wiResourceManager::del(texName);
	wiResourceManager::del(norName);
}
#pragma endregion