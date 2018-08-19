#include "wiSceneComponents.h"
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

#include <sstream>

using namespace std;
using namespace wiGraphicsTypes;

namespace wiSceneComponents
{

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
		bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = sizeof(Vertex_TEX);
		bd.ByteWidth = (UINT)(bd.StructureByteStride * vertices_TEX.size());
		bd.Format = Vertex_TEX::FORMAT;
		InitData.pSysMem = vertices_TEX.data();
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
Mesh::Vertex_FULL Mesh::TransformVertex(int vertexI, const XMMATRIX& mat)
{
	XMMATRIX sump;
	XMVECTOR pos = vertices_POS[vertexI].LoadPOS();
	XMVECTOR nor = vertices_POS[vertexI].LoadNOR();

	if (hasArmature() && !armature->boneCollection.empty())
	{
		XMFLOAT4 ind = vertices_BON[vertexI].GetInd_FULL();
		XMFLOAT4 wei = vertices_BON[vertexI].GetWei_FULL();


		float inWei[4] = {
			wei.x,
			wei.y,
			wei.z,
			wei.w
		};
		float inBon[4] = {
			ind.x,
			ind.y,
			ind.z,
			ind.w
		};
		if (inWei[0] || inWei[1] || inWei[2] || inWei[3])
		{
			sump = XMMATRIX(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
			for (unsigned int i = 0; i < 4; i++)
			{
				sump += XMLoadFloat4x4(&armature->boneCollection[int(inBon[i])]->boneRelativity) * inWei[i];
			}
		}
		else
		{
			sump = XMMatrixIdentity();
		}
		sump = XMMatrixMultiply(sump, mat);
	}
	else
	{
		sump = mat;
	}

	XMFLOAT3 transformedP, transformedN;
	XMStoreFloat3(&transformedP, XMVector3Transform(pos, sump));

	XMStoreFloat3(&transformedN, XMVector3Normalize(XMVector3TransformNormal(nor, sump)));

	Mesh::Vertex_FULL retV(transformedP);
	retV.nor = XMFLOAT4(transformedN.x, transformedN.y, transformedN.z, retV.nor.w);
	retV.tex = vertices_FULL[vertexI].tex;

	return retV;
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
			bool tmp;
			archive >> tmp/*optimized*/;
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
			bool tmp = true;
			archive << tmp/*optimized*/;
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
				auto& found = meshes.find(x->meshName);
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
						auto& it = materials.find(x->mesh->materialNames[i]);
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
				auto& it = materials.find(y->materialName);
				if (it != materials.end())
				{
					y->material = it->second;
				}
			}
			for (auto& y : x->hParticleSystems)
			{
				y->object = x;
				auto& it = materials.find(y->materialName);
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

	XMFLOAT4X4 tmp;

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

		if (archive.GetVersion() < 21)
		{
			//archive >> restInv;
			archive >> tmp;
		}

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

		if (archive.GetVersion() < 21)
		{
			//archive >> recursivePose;
			//archive >> recursiveRest;
			//archive >> recursiveRestInv;
			archive >> tmp;
			archive >> tmp;
			archive >> tmp;
		}

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

		if (archive.GetVersion() < 21)
		{
			//archive << restInv;
			archive << tmp;
		}

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

		if (archive.GetVersion() < 21)
		{
			//archive << recursivePose;
			//archive << recursiveRest;
			//archive << recursiveRestInv;
			archive << tmp;
			archive << tmp;
			archive << tmp;
		}

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

	XMMATRIX worldMatrix = getMatrix();
	XMMATRIX remapMat = XMLoadFloat4x4(&skinningRemap);

	// Update bone tree from root:
	for (Bone* root : rootbones)
	{
		// Update tree for skinning:
		//	Note that skinning is not using the armature transform, it will be calculated in armature local space
		//	This is needed because we want to skin meshes, then reuse the meshes for multiple objects without additional deform
		RecursiveBoneTransform(this, root, remapMat);

		// Update tree for bone attaching:
		//	The Skinning (RecursiveBoneTrasform) was operating in armature local space, but for bone attachments without deform,
		//	we need the matrices in world space
		XMMATRIX boneMatrix = XMLoadFloat4x4(&root->world);
		boneMatrix = boneMatrix * worldMatrix;
		XMStoreFloat4x4(&root->world, boneMatrix);
		root->UpdateTransform();
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
void Armature::RecursiveBoneTransform(Armature* armature, Bone* bone, const XMMATRIX& parentBoneMat)
{
	// TRANSITION BLENDING + ADDITIVE BLENDING
	XMVECTOR finalTrans = XMVectorSet(0, 0, 0, 0);
	XMVECTOR finalRotat = XMQuaternionIdentity();
	XMVECTOR finalScala = XMVectorSet(1, 1, 1, 0);

	for (auto& x : armature->animationLayers)
	{
		AnimationLayer& anim = *x;

		int frameCountPrev = armature->actions[anim.prevAction].frameCount;
		XMVECTOR prevTrans = InterpolateKeyFrames(anim.currentFramePrevAction, frameCountPrev, bone->actionFrames[anim.prevAction].keyframesPos, POSITIONKEYFRAMETYPE);
		XMVECTOR prevRotat = InterpolateKeyFrames(anim.currentFramePrevAction, frameCountPrev, bone->actionFrames[anim.prevAction].keyframesRot, ROTATIONKEYFRAMETYPE);
		XMVECTOR prevScala = InterpolateKeyFrames(anim.currentFramePrevAction, frameCountPrev, bone->actionFrames[anim.prevAction].keyframesSca, SCALARKEYFRAMETYPE);

		int frameCount = armature->actions[anim.activeAction].frameCount;
		XMVECTOR currTrans = InterpolateKeyFrames(anim.currentFrame, frameCount, bone->actionFrames[anim.activeAction].keyframesPos, POSITIONKEYFRAMETYPE);
		XMVECTOR currRotat = InterpolateKeyFrames(anim.currentFrame, frameCount, bone->actionFrames[anim.activeAction].keyframesRot, ROTATIONKEYFRAMETYPE);
		XMVECTOR currScala = InterpolateKeyFrames(anim.currentFrame, frameCount, bone->actionFrames[anim.activeAction].keyframesSca, SCALARKEYFRAMETYPE);

		currTrans = XMVectorLerp(prevTrans, currTrans, anim.blendFact);
		currRotat = XMQuaternionSlerp(prevRotat, currRotat, anim.blendFact);
		currScala = XMVectorLerp(prevScala, currScala, anim.blendFact);

		switch (anim.type)
		{
		case AnimationLayer::ANIMLAYER_TYPE_PRIMARY:
			finalTrans = currTrans;
			finalRotat = currRotat;
			finalScala = currScala;
			break;
		case AnimationLayer::ANIMLAYER_TYPE_ADDITIVE:
			finalTrans = XMVectorLerp(finalTrans, XMVectorAdd(finalTrans, currTrans), anim.weight);
			finalRotat = XMQuaternionSlerp(finalRotat, XMQuaternionMultiply(finalRotat, currRotat), anim.weight); // normalize?
			finalScala = XMVectorLerp(finalScala, XMVectorMultiply(finalScala, currScala), anim.weight);
			break;
		default:
			assert(0);
			break;
		}
	}

	bone->worldPrev = bone->world;
	bone->translationPrev = bone->translation;
	bone->rotationPrev = bone->rotation;
	bone->scalePrev = bone->scale;
	XMStoreFloat3(&bone->translation, finalTrans);
	XMStoreFloat4(&bone->rotation, finalRotat);
	XMStoreFloat3(&bone->scale, finalScala);

	// Bone local T-pose matrix (relative to PARENT):
	XMMATRIX rest = XMLoadFloat4x4(&bone->world_rest);

	// Bone global T-pose matrix inverse (relative to ROOT):
	XMMATRIX recursive_rest_inv = XMLoadFloat4x4(&bone->parent_inv_rest);

	// Animation local matrix (relative to ITSELF):
	XMMATRIX anim_relative = XMMatrixScalingFromVector(finalScala) * XMMatrixRotationQuaternion(finalRotat) * XMMatrixTranslationFromVector(finalTrans);

	// Animation local matrix (relative to PARENT):
	XMMATRIX anim_absolute = anim_relative * rest;

	// Bone global final matrix (WORLD-SPACE):
	XMMATRIX boneMat = anim_absolute * parentBoneMat;

	// Bone global final matrix (SKINNING-SPACE):
	XMMATRIX skinningMat = recursive_rest_inv * boneMat;

	XMStoreFloat4x4(&bone->world, boneMat); // usable in scene graph
	XMStoreFloat4x4(&bone->boneRelativity, skinningMat); // usable in skinning

	for (unsigned int i = 0; i<bone->childrenI.size(); ++i) {
		RecursiveBoneTransform(armature, bone->childrenI[i], boneMat);
	}
}
XMVECTOR Armature::InterpolateKeyFrames(float cf, const int maxCf, const std::vector<KeyFrame>& keyframeList, KeyFrameType type)
{
	XMVECTOR result = XMVectorSet(0, 0, 0, 0);

	switch (type)
	{
	case Armature::ROTATIONKEYFRAMETYPE:
		result = XMVectorSet(0, 0, 0, 1);
		break;
	case Armature::POSITIONKEYFRAMETYPE:
		result = XMVectorSet(0, 0, 0, 1);
		break;
	case Armature::SCALARKEYFRAMETYPE:
		result = XMVectorSet(1, 1, 1, 1);
		break;
	default:
		assert(0);
		break;
	}

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
		for (size_t i = 1; i < actions.size(); ++i)
		{
			if (!actions[i].name.compare(actionName))
			{
				anim->ChangeAction((int)i, blendFrames, weight);
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
void Armature::RecursiveRest(Bone* bone, XMMATRIX recursiveRest)
{
	Bone* parent = (Bone*)bone->parent;

	recursiveRest = XMLoadFloat4x4(&bone->world_rest) * recursiveRest;

	XMStoreFloat4x4(&bone->parent_inv_rest, XMMatrixInverse(0, recursiveRest));

	for (size_t i = 0; i < bone->childrenI.size(); ++i) {
		RecursiveRest(bone->childrenI[i], recursiveRest);
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

	XMMATRIX remapMat = XMLoadFloat4x4(&skinningRemap);
	for (Bone* root : rootbones)
	{
		RecursiveRest(root, remapMat);
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

		if (archive.GetVersion() >= 21)
		{
			archive >> skinningRemap;
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

		if (archive.GetVersion() >= 21)
		{
			archive << skinningRemap;
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
			baseP = mesh->TransformVertex(base).pos;
			tipP = mesh->TransformVertex(tip).pos;

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


}
