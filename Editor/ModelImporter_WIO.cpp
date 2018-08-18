#include "stdafx.h"
#include "ModelImporter.h"

#include <fstream>
#include <algorithm>

using namespace std;
using namespace wiGraphicsTypes;
using namespace wiSceneComponents;


Mesh* LoadMeshFromBinaryFile(const std::string& newName, const std::string& fname, const std::map<std::string, Material*>& materialColl, const unordered_set<Armature*>& armatures)
{
	Mesh* mesh = new Mesh(newName);

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
				mesh->doubleSided = true;
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
				mesh->billboardAxis = XMFLOAT3(0, 1, 0);
			else if (toupper(axis) == 'X')
				mesh->billboardAxis = XMFLOAT3(1, 0, 0);
			else if (toupper(axis) == 'Z')
				mesh->billboardAxis = XMFLOAT3(0, 0, 1);
			else
				mesh->billboardAxis = XMFLOAT3(0, 0, 0);
			mesh->isBillboarded = true;
		}

		int parented; //parentnamelength
		memcpy(&parented, buffer + offset, sizeof(int));
		offset += sizeof(int);
		if (parented) {
			char* pName = new char[parented + 1]();
			memcpy(pName, buffer + offset, parented);
			offset += parented;
			mesh->parent = pName;
			delete[] pName;

			stringstream identified_parent("");
			identified_parent << mesh->parent;
			for (Armature* a : armatures) {
				if (!a->name.compare(identified_parent.str())) {
					mesh->armatureName = identified_parent.str();
					mesh->armature = a;
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
			auto& iter = materialColl.find(identified_matname.str());
			if (iter != materialColl.end()) {
				mesh->subsets.push_back(MeshSubset());
				mesh->subsets.back().material = iter->second;
				//materials.push_back(iter->second);
			}

			mesh->materialNames.push_back(identified_matname.str());
			delete[] matName;
		}
		int rendermesh, vertexCount;
		memcpy(&rendermesh, buffer + offset, sizeof(int));
		offset += sizeof(int);
		memcpy(&vertexCount, buffer + offset, sizeof(int));
		offset += sizeof(int);

		mesh->vertices_FULL.resize(vertexCount);

		for (int i = 0; i<vertexCount; ++i) {
			float v[8];
			memcpy(v, buffer + offset, sizeof(float) * 8);
			offset += sizeof(float) * 8;
			mesh->vertices_FULL[i].pos.x = v[0];
			mesh->vertices_FULL[i].pos.y = v[1];
			mesh->vertices_FULL[i].pos.z = v[2];
			mesh->vertices_FULL[i].pos.w = 0;
			if (!mesh->isBillboarded) {
				mesh->vertices_FULL[i].nor.x = v[3];
				mesh->vertices_FULL[i].nor.y = v[4];
				mesh->vertices_FULL[i].nor.z = v[5];
			}
			else {
				mesh->vertices_FULL[i].nor.x = mesh->billboardAxis.x;
				mesh->vertices_FULL[i].nor.y = mesh->billboardAxis.y;
				mesh->vertices_FULL[i].nor.z = mesh->billboardAxis.z;
			}
			mesh->vertices_FULL[i].tex.x = v[6];
			mesh->vertices_FULL[i].tex.y = v[7];
			int matIndex;
			memcpy(&matIndex, buffer + offset, sizeof(int));
			offset += sizeof(int);
			mesh->vertices_FULL[i].tex.z = (float)matIndex;

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
				if (mesh->armature) {
					bool gotBone = false;
					int BONEINDEX = 0;
					int b = 0;
					while (!gotBone && b<(int)mesh->armature->boneCollection.size()) {
						if (!mesh->armature->boneCollection[b]->name.compare(nameB)) {
							gotBone = true;
							BONEINDEX = b; //GOT INDEX OF BONE OF THE WEIGHT IN THE PARENT ARMATURE
						}
						b++;
					}
					if (gotBone) { //ONLY PROCEED IF CORRESPONDING BONE WAS FOUND
						if (!mesh->vertices_FULL[i].wei.x) {
							mesh->vertices_FULL[i].wei.x = weightValue;
							mesh->vertices_FULL[i].ind.x = (float)BONEINDEX;
						}
						else if (!mesh->vertices_FULL[i].wei.y) {
							mesh->vertices_FULL[i].wei.y = weightValue;
							mesh->vertices_FULL[i].ind.y = (float)BONEINDEX;
						}
						else if (!mesh->vertices_FULL[i].wei.z) {
							mesh->vertices_FULL[i].wei.z = weightValue;
							mesh->vertices_FULL[i].ind.z = (float)BONEINDEX;
						}
						else if (!mesh->vertices_FULL[i].wei.w) {
							mesh->vertices_FULL[i].wei.w = weightValue;
							mesh->vertices_FULL[i].ind.w = (float)BONEINDEX;
						}
					}
				}

				//(+RIBBONTRAIL SETUP)(+VERTEXGROUP SETUP)

				if (nameB.find("trailbase") != string::npos)
					mesh->trailInfo.base = i;
				else if (nameB.find("trailtip") != string::npos)
					mesh->trailInfo.tip = i;

				bool windAffection = false;
				if (nameB.find("wind") != string::npos)
					windAffection = true;
				bool gotvg = false;
				for (unsigned int v = 0; v<mesh->vertexGroups.size(); ++v)
					if (!nameB.compare(mesh->vertexGroups[v].name)) {
						gotvg = true;
						mesh->vertexGroups[v].addVertex(VertexRef(i, weightValue));
						if (windAffection)
							mesh->vertices_FULL[i].pos.w = weightValue;
					}
				if (!gotvg) {
					mesh->vertexGroups.push_back(VertexGroup(nameB));
					mesh->vertexGroups.back().addVertex(VertexRef(i, weightValue));
					if (windAffection)
						mesh->vertices_FULL[i].pos.w = weightValue;
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
			mesh->indices.reserve(indexCount);
			for (int i = 0; i<indexCount; ++i) {
				mesh->indices.push_back(indexArray[i]);
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

				mesh->physicsindices.reserve(softCount[0]);
				mesh->physicsverts.reserve(softCount[1] / 3);
				for (int i = 0; i<softCount[0]; ++i) {
					mesh->physicsindices.push_back(softind[i]);
				}
				for (int i = 0; i<softCount[1]; i += 3) {
					mesh->physicsverts.push_back(XMFLOAT3(softvert[i], softvert[i + 1], softvert[i + 2]));
				}

				delete[] softind;
				delete[] softvert;
			}
			else {

			}
		}
		else {

		}

		memcpy(mesh->aabb.corners, buffer + offset, sizeof(mesh->aabb.corners));
		offset += sizeof(mesh->aabb.corners);

		int isSoftbody;
		memcpy(&isSoftbody, buffer + offset, sizeof(int));
		offset += sizeof(int);
		if (isSoftbody) {
			float prop[2]; //mass,friction
			memcpy(prop, buffer + offset, sizeof(float) * 2);
			offset += sizeof(float) * 2;
			mesh->softBody = true;
			mesh->mass = prop[0];
			mesh->friction = prop[1];
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

			for (unsigned int v = 0; v<mesh->vertexGroups.size(); ++v) {
				if (!strcmp(vgm, mesh->vertexGroups[v].name.c_str()))
					mesh->massVG = v;
				if (!strcmp(vgg, mesh->vertexGroups[v].name.c_str()))
					mesh->goalVG = v;
				if (!strcmp(vgs, mesh->vertexGroups[v].name.c_str()))
					mesh->softVG = v;
			}

			delete[]vgg;
			delete[]vgm;
			delete[]vgs;
		}

		delete[] buffer;

		mesh->renderable = rendermesh == 0 ? false : true;
	}

	return mesh;
}

void LoadWiArmatures(const std::string& directory, const std::string& name, unordered_set<Armature*>& armatures)
{
	stringstream filename("");
	filename << directory << name;

	ifstream file(filename.str().c_str());
	if (file)
	{
		Armature* armature = nullptr;
		while (!file.eof())
		{
			float trans[] = { 0,0,0,0 };
			string line = "";
			file >> line;
			if (line[0] == '/' && line.substr(2, 8) == "ARMATURE")
			{
				armature = new Armature(line.substr(11, strlen(line.c_str()) - 11));
				armatures.insert(armature);
			}
			else
			{
				switch (line[0])
				{
				case 'r':
					file >> trans[0] >> trans[1] >> trans[2] >> trans[3];
					armature->rotation_rest = XMFLOAT4(trans[0], trans[1], trans[2], trans[3]);
					break;
				case 's':
					file >> trans[0] >> trans[1] >> trans[2];
					armature->scale_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					break;
				case 't':
					file >> trans[0] >> trans[1] >> trans[2];
					armature->translation_rest = XMFLOAT3(trans[0], trans[1], trans[2]);
					{
						XMMATRIX world = XMMatrixScalingFromVector(XMLoadFloat3(&armature->scale))*XMMatrixRotationQuaternion(XMLoadFloat4(&armature->rotation))*XMMatrixTranslationFromVector(XMLoadFloat3(&armature->translation));
						XMStoreFloat4x4(&armature->world_rest, world);
					}
					break;
				case 'b':
				{
					string boneName;
					file >> boneName;
					armature->boneCollection.push_back(new Bone(boneName));
				}
				break;
				case 'p':
					file >> armature->boneCollection.back()->parentName;
					break;
				case 'l':
				{
					float x = 0, y = 0, z = 0, w = 0;
					file >> x >> y >> z >> w;
					XMVECTOR quaternion = XMVectorSet(x, y, z, w);
					file >> x >> y >> z;
					XMVECTOR translation = XMVectorSet(x, y, z, 0);

					XMMATRIX frame;
					frame = XMMatrixRotationQuaternion(quaternion) * XMMatrixTranslationFromVector(translation);

					XMStoreFloat3(&armature->boneCollection.back()->translation_rest, translation);
					XMStoreFloat4(&armature->boneCollection.back()->rotation_rest, quaternion);
					XMStoreFloat4x4(&armature->boneCollection.back()->world_rest, frame);
					//XMStoreFloat4x4(&armature->boneCollection.back()->restInv,XMMatrixInverse(0,frame));

				}
				break;
				case 'c':
					armature->boneCollection.back()->connected = true;
					break;
				case 'h':
					file >> armature->boneCollection.back()->length;
					break;
				default: break;
				}
			}
		}
	}
	file.close();



	//CREATE FAMILY
	for (Armature* armature : armatures)
	{
		armature->CreateFamily();
	}

}
void LoadWiMaterialLibrary(const std::string& directory, const std::string& name, const std::string& texturesDir, std::map<std::string, Material*>& materials)
{
	int materialI = (int)(materials.size() - 1);

	Material* currentMat = NULL;

	stringstream filename("");
	filename << directory << name;

	ifstream file(filename.str().c_str());
	if (file) {
		while (!file.eof()) {
			string line = "";
			file >> line;
			if (line[0] == '/' && !strcmp(line.substr(2, 8).c_str(), "MATERIAL")) {
				if (currentMat)
				{
					currentMat->ConvertToPhysicallyBasedMaterial();
					materials.insert(pair<string, Material*>(currentMat->name, currentMat));
				}

				currentMat = new Material(line.substr(11, strlen(line.c_str()) - 11));
				materialI++;
			}
			else {
				switch (line[0]) {
				case 'd':
					file >> currentMat->diffuseColor.x;
					file >> currentMat->diffuseColor.y;
					file >> currentMat->diffuseColor.z;
					break;
				case 'X':
					currentMat->cast_shadow = false;
					break;
				case 'r':
				{
					string resourceName = "";
					file >> resourceName;
					stringstream ss("");
					ss << directory << texturesDir << resourceName.c_str();
					currentMat->surfaceMapName = ss.str();
					currentMat->surfaceMap = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
				}
				break;
				case 'n':
				{
					string resourceName = "";
					file >> resourceName;
					stringstream ss("");
					ss << directory << texturesDir << resourceName.c_str();
					currentMat->normalMapName = ss.str();
					currentMat->normalMap = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
				}
				break;
				case 't':
				{
					string resourceName = "";
					file >> resourceName;
					stringstream ss("");
					ss << directory << texturesDir << resourceName.c_str();
					currentMat->textureName = ss.str();
					currentMat->texture = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
				}
				file >> currentMat->premultipliedTexture;
				break;
				case 'D':
				{
					string resourceName = "";
					file >> resourceName;
					stringstream ss("");
					ss << directory << texturesDir << resourceName.c_str();
					currentMat->displacementMapName = ss.str();
					currentMat->displacementMap = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
				}
				break;
				case 'S':
				{
					string resourceName = "";
					file >> resourceName;
					stringstream ss("");
					ss << directory << texturesDir << resourceName.c_str();
					currentMat->specularMapName = ss.str();
					currentMat->specularMap = (Texture2D*)wiResourceManager::GetGlobal()->add(ss.str());
				}
				break;
				case 'a':
					file >> currentMat->alpha;
					break;
				case 'h':
					currentMat->shadeless = true;
					break;
				case 'R':
					file >> currentMat->refractionIndex;
					break;
				case 'e':
					file >> currentMat->enviroReflection;
					break;
				case 's':
					file >> currentMat->specular.x;
					file >> currentMat->specular.y;
					file >> currentMat->specular.z;
					file >> currentMat->specular.w;
					break;
				case 'p':
					file >> currentMat->specular_power;
					break;
				case 'k':
					currentMat->isSky = true;
					break;
				case 'm':
					file >> currentMat->movingTex.x;
					file >> currentMat->movingTex.y;
					file >> currentMat->movingTex.z;
					currentMat->framesToWaitForTexCoordOffset = currentMat->movingTex.z;
					break;
				case 'w':
					currentMat->water = true;
					break;
				case 'u':
					currentMat->subsurfaceScattering = true;
					break;
				case 'b':
				{
					string blend;
					file >> blend;
					if (!blend.compare("ADD"))
						currentMat->blendFlag = BLENDMODE_ADDITIVE;
				}
				break;
				case 'i':
				{
					file >> currentMat->emissive;
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
	, std::map<std::string, Mesh*>& meshes, const std::map<std::string, Material*>& materials)
{

	stringstream filename("");
	filename << directory << name;

	ifstream file(filename.str().c_str());
	if (file)
	{
		Object* object = nullptr;
		while (!file.eof())
		{
			float trans[] = { 0,0,0,0 };
			string line = "";
			file >> line;
			if (line[0] == '/' && !strcmp(line.substr(2, 6).c_str(), "OBJECT"))
			{
				object = new Object(line.substr(9, strlen(line.c_str()) - 9));
				objects.insert(object);
			}
			else
			{
				switch (line[0])
				{
				case 'm':
				{
					string meshName = "";
					file >> meshName;
					object->meshName = meshName;
					auto& iter = meshes.find(meshName);

					if (line[1] == 'b')
					{
						//binary load mesh in place if not present
						if (iter == meshes.end())
						{
							stringstream meshFileName("");
							meshFileName << directory << meshName << ".wimesh";
							Mesh* mesh = LoadMeshFromBinaryFile(meshName, meshFileName.str(), materials, armatures);
							object->mesh = mesh;
							meshes.insert(pair<string, Mesh*>(meshName, mesh));
						}
						else
						{
							object->mesh = iter->second;
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
					XMFLOAT3 s, t;
					XMFLOAT4 r;
					file >> t.x >> t.y >> t.z >> r.x >> r.y >> r.z >> r.w >> s.x >> s.y >> s.z;
					XMStoreFloat4x4(&object->parent_inv_rest
						, XMMatrixScalingFromVector(XMLoadFloat3(&s)) *
						XMMatrixRotationQuaternion(XMLoadFloat4(&r)) *
						XMMatrixTranslationFromVector(XMLoadFloat3(&t))
					);
				}
				break;
				case 'r':
					file >> trans[0] >> trans[1] >> trans[2] >> trans[3];
					object->Rotate(XMFLOAT4(trans[0], trans[1], trans[2], trans[3]));
					break;
				case 's':
					file >> trans[0] >> trans[1] >> trans[2];
					object->Scale(XMFLOAT3(trans[0], trans[1], trans[2]));
					break;
				case 't':
					file >> trans[0] >> trans[1] >> trans[2];
					object->Translate(XMFLOAT3(trans[0], trans[1], trans[2]));
					break;
				case 'E':
				{
					string systemName, materialName;
					bool visibleEmitter;
					float size, randfac, norfac;
					float count, life, randlife;
					float scaleX, scaleY, rot;
					file >> systemName >> visibleEmitter >> materialName >> size >> randfac >> norfac >> count >> life >> randlife;
					file >> scaleX >> scaleY >> rot;

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
					string name, mat, densityG, lenG;
					float len;
					int count;
					file >> name >> mat >> len >> count >> densityG >> lenG;

					object->hParticleSystems.push_back(new wiHairParticle(name, len, count, mat, object, densityG, lenG));
				}
				break;
				case 'P':
					object->rigidBody = true;
					file >> object->collisionShape >> object->mass >>
						object->friction >> object->restitution >> object->damping >> object->physicsType >>
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
void LoadWiMeshes(const std::string& directory, const std::string& name, std::map<std::string, Mesh*>& meshes,
	const unordered_set<Armature*>& armatures, const std::map<std::string, Material*>& materials)
{
	int meshI = (int)(meshes.size() - 1);
	Mesh* currentMesh = NULL;

	stringstream filename("");
	filename << directory << name;

	ifstream file(filename.str().c_str());
	if (file) {
		while (!file.eof())
		{
			float trans[] = { 0,0,0,0 };
			string line = "";
			file >> line;
			if (line[0] == '/' && !line.substr(2, 4).compare("MESH")) {
				currentMesh = new Mesh(line.substr(7, strlen(line.c_str()) - 7));
				meshes.insert(pair<string, Mesh*>(currentMesh->name, currentMesh));
				meshI++;
			}
			else
			{
				switch (line[0])
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
					if (currentMesh->isBillboarded) {
						currentMesh->vertices_FULL.back().nor.x = currentMesh->billboardAxis.x;
						currentMesh->vertices_FULL.back().nor.y = currentMesh->billboardAxis.y;
						currentMesh->vertices_FULL.back().nor.z = currentMesh->billboardAxis.z;
					}
					else {
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
					float weight = 0;
					int BONEINDEX = 0;
					file >> nameB >> weight;
					bool gotBone = false;
					if (currentMesh->armature != nullptr) {
						int j = 0;
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
						else if (!currentMesh->vertices_FULL.back().wei.y) {
							currentMesh->vertices_FULL.back().wei.y = weight;
							currentMesh->vertices_FULL.back().ind.y = (float)BONEINDEX;
						}
						else if (!currentMesh->vertices_FULL.back().wei.z) {
							currentMesh->vertices_FULL.back().wei.z = weight;
							currentMesh->vertices_FULL.back().ind.z = (float)BONEINDEX;
						}
						else if (!currentMesh->vertices_FULL.back().wei.w) {
							currentMesh->vertices_FULL.back().wei.w = weight;
							currentMesh->vertices_FULL.back().ind.w = (float)BONEINDEX;
						}
					}

					//(+RIBBONTRAIL SETUP)(+VERTEXGROUP SETUP)

					if (nameB.find("trailbase") != string::npos)
						currentMesh->trailInfo.base = (int)(currentMesh->vertices_FULL.size() - 1);
					else if (nameB.find("trailtip") != string::npos)
						currentMesh->trailInfo.tip = (int)(currentMesh->vertices_FULL.size() - 1);

					bool windAffection = false;
					if (nameB.find("wind") != string::npos)
						windAffection = true;
					bool gotvg = false;
					for (unsigned int v = 0; v<currentMesh->vertexGroups.size(); ++v)
						if (!nameB.compare(currentMesh->vertexGroups[v].name)) {
							gotvg = true;
							currentMesh->vertexGroups[v].addVertex(VertexRef((int)(currentMesh->vertices_FULL.size() - 1), weight));
							if (windAffection)
								currentMesh->vertices_FULL.back().pos.w = weight;
						}
					if (!gotvg) {
						currentMesh->vertexGroups.push_back(VertexGroup(nameB));
						currentMesh->vertexGroups.back().addVertex(VertexRef((int)(currentMesh->vertices_FULL.size() - 1), weight));
						if (windAffection)
							currentMesh->vertices_FULL.back().pos.w = weight;
					}
				}
				break;
				case 'i':
				{
					int count;
					file >> count;
					for (int i = 0; i<count; i++) {
						int index;
						file >> index;
						currentMesh->indices.push_back(index);
					}
					break;
				}
				case 'V':
				{
					XMFLOAT3 pos;
					file >> pos.x >> pos.y >> pos.z;
					currentMesh->physicsverts.push_back(pos);
				}
				break;
				case 'I':
				{
					int count;
					file >> count;
					for (int i = 0; i<count; i++) {
						int index;
						file >> index;
						currentMesh->physicsindices.push_back(index);
					}
					break;
				}
				case 'm':
				{
					string mName = "";
					file >> mName;
					currentMesh->materialNames.push_back(mName);
					auto& iter = materials.find(mName);
					if (iter != materials.end()) {
						currentMesh->subsets.push_back(MeshSubset());
						currentMesh->renderable = true;
						currentMesh->subsets.back().material = (iter->second);
					}
				}
				break;
				case 'a':
					file >> currentMesh->vertices_FULL.back().tex.z;
					break;
				case 'B':
					for (int corner = 0; corner<8; ++corner) {
						file >> currentMesh->aabb.corners[corner].x;
						file >> currentMesh->aabb.corners[corner].y;
						file >> currentMesh->aabb.corners[corner].z;
					}
					break;
				case 'b':
				{
					currentMesh->isBillboarded = true;
					string read = "";
					file >> read;
					transform(read.begin(), read.end(), read.begin(), toupper);
					if (read.find(toupper('y')) != string::npos) currentMesh->billboardAxis = XMFLOAT3(0, 1, 0);
					else if (read.find(toupper('x')) != string::npos) currentMesh->billboardAxis = XMFLOAT3(1, 0, 0);
					else if (read.find(toupper('z')) != string::npos) currentMesh->billboardAxis = XMFLOAT3(0, 0, 1);
					else currentMesh->billboardAxis = XMFLOAT3(0, 0, 0);
				}
				break;
				case 'S':
				{
					currentMesh->softBody = true;
					string mvgi = "", gvgi = "", svgi = "";
					file >> currentMesh->mass >> currentMesh->friction >> gvgi >> mvgi >> svgi;
					for (unsigned int v = 0; v<currentMesh->vertexGroups.size(); ++v) {
						if (!strcmp(mvgi.c_str(), currentMesh->vertexGroups[v].name.c_str()))
							currentMesh->massVG = v;
						if (!strcmp(gvgi.c_str(), currentMesh->vertexGroups[v].name.c_str()))
							currentMesh->goalVG = v;
						if (!strcmp(svgi.c_str(), currentMesh->vertexGroups[v].name.c_str()))
							currentMesh->softVG = v;
					}
				}
				break;
				default: break;
				}
			}
		}
	}
	file.close();

	if (currentMesh)
		meshes.insert(pair<string, Mesh*>(currentMesh->name, currentMesh));

}
void LoadWiActions(const std::string& directory, const std::string& name, unordered_set<Armature*>& armatures)
{
	Armature* armatureI = nullptr;
	Bone* boneI = nullptr;
	int firstFrame = INT_MAX;

	stringstream filename("");
	filename << directory << name;

	ifstream file(filename.str().c_str());
	if (file) {
		while (!file.eof()) {
			string line = "";
			file >> line;
			if (line[0] == '/' && !strcmp(line.substr(2, 8).c_str(), "ARMATURE")) {
				string armaturename = line.substr(11, strlen(line.c_str()) - 11);
				for (auto& a : armatures)
				{
					if (!a->name.compare(armaturename)) {
						armatureI = a;
						break;
					}
				}
			}
			else {
				switch (line[0]) {
				case 'C':
					armatureI->actions.push_back(Action());
					file >> armatureI->actions.back().name;
					break;
				case 'A':
					file >> armatureI->actions.back().frameCount;
					break;
				case 'b':
				{
					string boneName;
					file >> boneName;
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
					float x = 0, y = 0, z = 0, w = 0;
					file >> f >> x >> y >> z >> w;
					if (boneI != nullptr)
					{
						boneI->actionFrames.back().keyframesRot.push_back(KeyFrame(f, x, y, z, w));
					}
				}
				break;
				case 't':
				{
					int f = 0;
					float x = 0, y = 0, z = 0;
					file >> f >> x >> y >> z;
					if (boneI != nullptr)
					{
						boneI->actionFrames.back().keyframesPos.push_back(KeyFrame(f, x, y, z, 0));
					}
				}
				break;
				case 's':
				{
					int f = 0;
					float x = 0, y = 0, z = 0;
					file >> f >> x >> y >> z;
					if (boneI != nullptr)
					{
						boneI->actionFrames.back().keyframesSca.push_back(KeyFrame(f, x, y, z, 0));
					}
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
	filename << directory << name;

	ifstream file(filename.str().c_str());
	if (file)
	{
		Light* light = nullptr;
		while (!file.eof())
		{
			string line = "";
			file >> line;
			switch (line[0])
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
				file >> light->name;
				light->shadow = true;
			}
			break;
			case 'S':
			{
				light = new Light();
				lights.insert(light);
				light->SetType(Light::SPOT);
				file >> light->name;
				file >> light->shadow >> light->enerDis.z;
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
				XMFLOAT3 s, t;
				XMFLOAT4 r;
				file >> t.x >> t.y >> t.z >> r.x >> r.y >> r.z >> r.w >> s.x >> s.y >> s.z;
				XMStoreFloat4x4(&light->parent_inv_rest
					, XMMatrixScalingFromVector(XMLoadFloat3(&s)) *
					XMMatrixRotationQuaternion(XMLoadFloat4(&r)) *
					XMMatrixTranslationFromVector(XMLoadFloat3(&t))
				);
			}
			break;
			case 't':
			{
				float x, y, z;
				file >> x >> y >> z;
				light->Translate(XMFLOAT3(x, y, z));
				break;
			}
			case 'r':
			{
				float x, y, z, w;
				file >> x >> y >> z >> w;
				light->Rotate(XMFLOAT4(x, y, z, w));
				break;
			}
			case 'c':
			{
				float r, g, b;
				file >> r >> g >> b;
				light->color = XMFLOAT4(r, g, b, 0);
				break;
			}
			case 'e':
				file >> light->enerDis.x;
				break;
			case 'd':
				file >> light->enerDis.y;
				break;
			case 'n':
				light->noHalo = true;
				break;
			case 'l':
			{
				string t = "";
				file >> t;
				stringstream rim("");
				rim << directory << "rims/" << t;
				Texture2D* tex = nullptr;
				if ((tex = (Texture2D*)wiResourceManager::GetGlobal()->add(rim.str())) != nullptr) {
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
void LoadWiCameras(const std::string&directory, const std::string& name, std::list<Camera*>& cameras
	, const unordered_set<Armature*>& armatures)
{
	stringstream filename("");
	filename << directory << name;

	ifstream file(filename.str().c_str());
	if (file)
	{
		string voidStr("");
		file >> voidStr;
		while (!file.eof()) {
			string line = "";
			file >> line;
			switch (line[0]) {

			case 'c':
			{
				XMFLOAT3 trans;
				XMFLOAT4 rot;
				string name(""), parentA(""), parentB("");
				file >> name >> parentA >> parentB >> trans.x >> trans.y >> trans.z >> rot.x >> rot.y >> rot.z >> rot.w;

				cameras.push_back(new Camera(
					trans, rot
					, name)
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
				XMFLOAT3 s, t;
				XMFLOAT4 r;
				file >> t.x >> t.y >> t.z >> r.x >> r.y >> r.z >> r.w >> s.x >> s.y >> s.z;
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
	filename << directory << name;

	ifstream file(filename.str().c_str());
	if (file)
	{
		Decal* decal = nullptr;
		string voidStr = "";
		file >> voidStr;
		while (!file.eof())
		{
			string line = "";
			file >> line;
			switch (line[0])
			{
			case 'd':
			{
				string name;
				XMFLOAT3 loc, scale;
				XMFLOAT4 rot;
				file >> name >> scale.x >> scale.y >> scale.z >> loc.x >> loc.y >> loc.z >> rot.x >> rot.y >> rot.z >> rot.w;
				decal = new Decal(loc, scale, rot);
				decal->name = name;
				decals.insert(decal);
			}
			break;
			case 't':
			{
				string tex = "";
				file >> tex;
				stringstream ss("");
				ss << directory << texturesDir << tex;
				decal->addTexture(ss.str());
			}
			break;
			case 'n':
			{
				string tex = "";
				file >> tex;
				stringstream ss("");
				ss << directory << texturesDir << tex;
				decal->addNormal(ss.str());
			}
			break;
			default:break;
			};
		}
	}
	file.close();
}

Model* ImportModel_WIO(const std::string& fileName)
{
	string directory, name;
	wiHelper::SplitPath(fileName, directory, name);
	string extension = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(name));
	wiHelper::RemoveExtensionFromFileName(name);

	Model* model = new Model;
	model->name = name;

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

	LoadWiArmatures(directory, armatureFilePath.str(), model->armatures);
	LoadWiMaterialLibrary(directory, materialLibFilePath.str(), "textures/", model->materials);
	LoadWiMeshes(directory, meshesFilePath.str(), model->meshes, model->armatures, model->materials);
	LoadWiObjects(directory, objectsFilePath.str(), model->objects, model->armatures, model->meshes, model->materials);
	LoadWiActions(directory, actionsFilePath.str(), model->armatures);
	LoadWiLights(directory, lightsFilePath.str(), model->lights);
	LoadWiDecals(directory, decalsFilePath.str(), "textures/", model->decals);
	LoadWiCameras(directory, camerasFilePath.str(), model->cameras, model->armatures);

	model->FinishLoading();

	return model;
}
