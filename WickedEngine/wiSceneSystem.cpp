#include "wiSceneSystem.h"
#include "wiMath.h"
#include "wiTextureHelper.h"
#include "wiResourceManager.h"
#include "wiPHYSICS.h"

using namespace wiECS;
using namespace wiGraphicsTypes;

namespace wiSceneSystem
{

	void TransformComponent::UpdateTransform()
	{
		if (dirty)
		{
			dirty = false;

			XMVECTOR S_local = XMLoadFloat3(&scale_local);
			XMVECTOR R_local = XMLoadFloat4(&rotation_local);
			XMVECTOR T_local = XMLoadFloat3(&translation_local);
			XMMATRIX W =
				XMMatrixScalingFromVector(S_local) *
				XMMatrixRotationQuaternion(R_local) *
				XMMatrixTranslationFromVector(T_local);

			scale = scale_local;
			rotation = rotation_local;
			translation = translation_local;

			XMStoreFloat4x4(&world, W);
		}
	}
	void TransformComponent::UpdateParentedTransform(const TransformComponent& parent, const XMFLOAT4X4& inverseParentBindMatrix)
	{
		XMMATRIX W;

		// Normally, every transform would be NOT dirty at this point, but...

		if (parent.dirty)
		{
			// If parent is dirty, that means it was tagged by animation system...
			dirty = true;

			W = XMLoadFloat4x4(&world);
		}
		else
		{
			// If it is not dirty, then we still need to propagate parent's matrix to this, 
			//	because every transform is marked as NOT dirty at the end of transform update system
			//	but we look up the local matrix instead, because world matrix might contain 
			//	results from previous run of the hierarchy system...
			XMVECTOR S_local = XMLoadFloat3(&scale_local);
			XMVECTOR R_local = XMLoadFloat4(&rotation_local);
			XMVECTOR T_local = XMLoadFloat3(&translation_local);
			W = XMMatrixScalingFromVector(S_local) *
				XMMatrixRotationQuaternion(R_local) *
				XMMatrixTranslationFromVector(T_local);
		}

		XMMATRIX W_parent = XMLoadFloat4x4(&parent.world);
		XMMATRIX B = XMLoadFloat4x4(&inverseParentBindMatrix);
		W = W * B * W_parent;

		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, W);
		XMStoreFloat3(&scale, S);
		XMStoreFloat4(&rotation, R);
		XMStoreFloat3(&translation, T);

		XMStoreFloat4x4(&world, W);
	}
	void TransformComponent::ApplyTransform()
	{
		dirty = true;
		scale_local = scale;
		rotation_local = rotation;
		translation_local = translation;
	}
	void TransformComponent::ClearTransform()
	{
		dirty = true;
		scale_local = XMFLOAT3(1, 1, 1);
		rotation_local = XMFLOAT4(0, 0, 0, 1);
		translation_local = XMFLOAT3(0, 0, 0);
	}
	void TransformComponent::Translate(const XMFLOAT3& value)
	{
		dirty = true;
		translation_local.x += value.x;
		translation_local.y += value.y;
		translation_local.z += value.z;
	}
	void TransformComponent::RotateRollPitchYaw(const XMFLOAT3& value)
	{
		dirty = true;

		// This needs to be handled a bit differently
		XMVECTOR quat = XMLoadFloat4(&rotation_local);
		XMVECTOR x = XMQuaternionRotationRollPitchYaw(value.x, 0, 0);
		XMVECTOR y = XMQuaternionRotationRollPitchYaw(0, value.y, 0);
		XMVECTOR z = XMQuaternionRotationRollPitchYaw(0, 0, value.z);

		quat = XMQuaternionMultiply(x, quat);
		quat = XMQuaternionMultiply(quat, y);
		quat = XMQuaternionMultiply(z, quat);
		quat = XMQuaternionNormalize(quat);

		XMStoreFloat4(&rotation_local, quat);
	}
	void TransformComponent::Rotate(const XMFLOAT4& quaternion)
	{
		dirty = true;
		XMVECTOR result = XMQuaternionMultiply(XMLoadFloat4(&rotation_local), XMLoadFloat4(&quaternion));
		result = XMQuaternionNormalize(result);
		XMStoreFloat4(&rotation_local, result);
	}
	void TransformComponent::Scale(const XMFLOAT3& value)
	{
		dirty = true;
		scale_local.x *= value.x;
		scale_local.y *= value.y;
		scale_local.z *= value.z;
	}
	void TransformComponent::MatrixTransform(const XMFLOAT4X4& matrix)
	{
		MatrixTransform(XMLoadFloat4x4(&matrix));
	}
	void TransformComponent::MatrixTransform(const XMMATRIX& matrix)
	{
		dirty = true;

		XMVECTOR S;
		XMVECTOR R;
		XMVECTOR T;
		XMMatrixDecompose(&S, &R, &T, matrix);

		S = XMLoadFloat3(&scale_local) * S;
		R = XMQuaternionMultiply(XMLoadFloat4(&rotation_local), R);
		T = XMLoadFloat3(&translation_local) + T;

		XMStoreFloat3(&scale_local, S);
		XMStoreFloat4(&rotation_local, R);
		XMStoreFloat3(&translation_local, T);
	}
	void TransformComponent::Lerp(const TransformComponent& a, const TransformComponent& b, float t)
	{
		dirty = true;

		XMVECTOR aS = XMLoadFloat3(&a.scale);
		XMVECTOR aR = XMLoadFloat4(&a.rotation);
		XMVECTOR aT = XMLoadFloat3(&a.translation);

		XMVECTOR bS = XMLoadFloat3(&b.scale);
		XMVECTOR bR = XMLoadFloat4(&b.rotation);
		XMVECTOR bT = XMLoadFloat3(&b.translation);

		XMVECTOR S = XMVectorLerp(aS, bS, t);
		XMVECTOR R = XMQuaternionSlerp(aR, bR, t);
		XMVECTOR T = XMVectorLerp(aT, bT, t);

		XMStoreFloat3(&scale_local, S);
		XMStoreFloat4(&rotation_local, R);
		XMStoreFloat3(&translation_local, T);
	}
	void TransformComponent::CatmullRom(const TransformComponent& a, const TransformComponent& b, const TransformComponent& c, const TransformComponent& d, float t)
	{
		dirty = true;

		XMVECTOR aS = XMLoadFloat3(&a.scale);
		XMVECTOR aR = XMLoadFloat4(&a.rotation);
		XMVECTOR aT = XMLoadFloat3(&a.translation);

		XMVECTOR bS = XMLoadFloat3(&b.scale);
		XMVECTOR bR = XMLoadFloat4(&b.rotation);
		XMVECTOR bT = XMLoadFloat3(&b.translation);

		XMVECTOR cS = XMLoadFloat3(&c.scale);
		XMVECTOR cR = XMLoadFloat4(&c.rotation);
		XMVECTOR cT = XMLoadFloat3(&c.translation);

		XMVECTOR dS = XMLoadFloat3(&d.scale);
		XMVECTOR dR = XMLoadFloat4(&d.rotation);
		XMVECTOR dT = XMLoadFloat3(&d.translation);

		XMVECTOR T = XMVectorCatmullRom(aT, bT, cT, dT, t);

		// Catmull-rom has issues with full rotation for quaternions (todo):
		XMVECTOR R = XMVectorCatmullRom(aR, bR, cR, dR, t);
		R = XMQuaternionNormalize(R);

		XMVECTOR S = XMVectorCatmullRom(aS, bS, cS, dS, t);

		XMStoreFloat3(&translation_local, T);
		XMStoreFloat4(&rotation_local, R);
		XMStoreFloat3(&scale_local, S);
	}

	Texture2D* MaterialComponent::GetBaseColorMap() const
	{
		if (baseColorMap != nullptr)
		{
			return baseColorMap;
		}
		return wiTextureHelper::getInstance()->getWhite();
	}
	Texture2D* MaterialComponent::GetNormalMap() const
	{
		return normalMap;
		//if (normalMap != nullptr)
		//{
		//	return normalMap;
		//}
		//return wiTextureHelper::getInstance()->getNormalMapDefault();
	}
	Texture2D* MaterialComponent::GetSurfaceMap() const
	{
		if (surfaceMap != nullptr)
		{
			return surfaceMap;
		}
		return wiTextureHelper::getInstance()->getWhite();
	}
	Texture2D* MaterialComponent::GetDisplacementMap() const
	{
		if (displacementMap != nullptr)
		{
			return displacementMap;
		}
		return wiTextureHelper::getInstance()->getWhite();
	}

	void MeshComponent::CreateRenderData()
	{
		HRESULT hr;

		// Create index buffer GPU data:
		{
			uint32_t counter = 0;
			uint8_t stride;
			void* gpuIndexData;
			if (vertex_positions.size() > 65535)
			{
				indexFormat = INDEXFORMAT_32BIT;
				gpuIndexData = new uint32_t[indices.size()];
				stride = sizeof(uint32_t);

				for (auto& x : indices)
				{
					static_cast<uint32_t*>(gpuIndexData)[counter++] = static_cast<uint32_t>(x);
				}

			}
			else
			{
				indexFormat = INDEXFORMAT_16BIT;
				gpuIndexData = new uint16_t[indices.size()];
				stride = sizeof(uint16_t);

				for (auto& x : indices)
				{
					static_cast<uint16_t*>(gpuIndexData)[counter++] = static_cast<uint16_t>(x);
				}

			}


			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_INDEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = 0;
			bd.StructureByteStride = stride;
			bd.Format = GetIndexFormat() == INDEXFORMAT_16BIT ? FORMAT_R16_UINT : FORMAT_R32_UINT;

			SubresourceData InitData;
			InitData.pSysMem = gpuIndexData;
			bd.ByteWidth = (UINT)(stride * indices.size());
			indexBuffer.reset(new GPUBuffer);
			hr = wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, indexBuffer.get());
			assert(SUCCEEDED(hr));

			SAFE_DELETE_ARRAY(gpuIndexData);
		}


		XMFLOAT3 _min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
		XMFLOAT3 _max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		// vertexBuffer - POSITION + NORMAL + SUBSETINDEX:
		{
			std::vector<uint8_t> vertex_subsetindices(vertex_positions.size());

			uint32_t subsetCounter = 0;
			for (auto& subset : subsets)
			{
				for (uint32_t i = 0; i < subset.indexCount; ++i)
				{
					uint32_t index = indices[subset.indexOffset + i];
					vertex_subsetindices[index] = subsetCounter;
				}
				subsetCounter++;
			}

			std::vector<Vertex_POS> vertices(vertex_positions.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				const XMFLOAT3& pos = vertex_positions[i];
				XMFLOAT3& nor = vertex_normals.empty() ? XMFLOAT3(1, 1, 1) : vertex_normals[i];
				XMStoreFloat3(&nor, XMVector3Normalize(XMLoadFloat3(&nor)));
				uint32_t subsetIndex = vertex_subsetindices[i];
				vertices[i].FromFULL(pos, nor, subsetIndex);

				_min = wiMath::Min(_min, pos);
				_max = wiMath::Max(_max, pos);
			}

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			bd.ByteWidth = (UINT)(sizeof(Vertex_POS) * vertices.size());

			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			vertexBuffer_POS.reset(new GPUBuffer);
			hr = wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer_POS.get());
			assert(SUCCEEDED(hr));
		}

		aabb.create(_min, _max);

		// skinning buffers:
		if (!vertex_boneindices.empty())
		{
			std::vector<Vertex_BON> vertices(vertex_boneindices.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				XMFLOAT4& wei = vertex_boneweights[i];
				// normalize bone weights
				float len = wei.x + wei.y + wei.z + wei.w;
				if (len > 0)
				{
					wei.x /= len;
					wei.y /= len;
					wei.z /= len;
					wei.w /= len;
				}
				vertices[i].FromFULL(vertex_boneindices[i], wei);
			}

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.BindFlags = BIND_SHADER_RESOURCE;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
			bd.ByteWidth = (UINT)(sizeof(Vertex_BON) * vertices.size());

			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			vertexBuffer_BON.reset(new GPUBuffer);
			hr = wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer_BON.get());
			assert(SUCCEEDED(hr));

			bd.Usage = USAGE_DEFAULT;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_UNORDERED_ACCESS;
			bd.CPUAccessFlags = 0;
			bd.MiscFlags = RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

			bd.ByteWidth = (UINT)(sizeof(Vertex_POS) * vertex_positions.size());
			streamoutBuffer_POS.reset(new GPUBuffer);
			hr = wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, streamoutBuffer_POS.get());
			assert(SUCCEEDED(hr));

			bd.ByteWidth = (UINT)(sizeof(Vertex_POS) * vertex_positions.size());
			streamoutBuffer_PRE.reset(new GPUBuffer);
			hr = wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, streamoutBuffer_PRE.get());
			assert(SUCCEEDED(hr));
		}

		// vertexBuffer - TEXCOORDS
		if(!vertex_texcoords.empty())
		{
			std::vector<Vertex_TEX> vertices(vertex_texcoords.size());
			for (size_t i = 0; i < vertices.size(); ++i)
			{
				vertices[i].FromFULL(vertex_texcoords[i]);
			}

			GPUBufferDesc bd;
			bd.Usage = USAGE_IMMUTABLE;
			bd.CPUAccessFlags = 0;
			bd.BindFlags = BIND_VERTEX_BUFFER | BIND_SHADER_RESOURCE;
			bd.MiscFlags = 0;
			bd.StructureByteStride = sizeof(Vertex_TEX);
			bd.ByteWidth = (UINT)(bd.StructureByteStride * vertices.size());
			bd.Format = Vertex_TEX::FORMAT;

			SubresourceData InitData;
			InitData.pSysMem = vertices.data();
			vertexBuffer_TEX.reset(new GPUBuffer);
			hr = wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer_TEX.get());
			assert(SUCCEEDED(hr));
		}

	}
	void MeshComponent::ComputeNormals(bool smooth)
	{
		// Start recalculating normals:

		if (smooth)
		{
			// Compute smooth surface normals:

			// 1.) Zero normals, they will be averaged later
			for (size_t i = 0; i < vertex_normals.size(); i++)
			{
				vertex_normals[i] = XMFLOAT3(0, 0, 0);
			}

			// 2.) Find identical vertices by POSITION, accumulate face normals
			for (size_t i = 0; i < vertex_positions.size(); i++)
			{
				XMFLOAT3& v_search_pos = vertex_positions[i];

				for (size_t ind = 0; ind < indices.size() / 3; ++ind)
				{
					uint32_t i0 = indices[ind * 3 + 0];
					uint32_t i1 = indices[ind * 3 + 1];
					uint32_t i2 = indices[ind * 3 + 2];

					XMFLOAT3& v0 = vertex_positions[i0];
					XMFLOAT3& v1 = vertex_positions[i1];
					XMFLOAT3& v2 = vertex_positions[i2];

					bool match_pos0 =
						fabs(v_search_pos.x - v0.x) < FLT_EPSILON &&
						fabs(v_search_pos.y - v0.y) < FLT_EPSILON &&
						fabs(v_search_pos.z - v0.z) < FLT_EPSILON;

					bool match_pos1 =
						fabs(v_search_pos.x - v1.x) < FLT_EPSILON &&
						fabs(v_search_pos.y - v1.y) < FLT_EPSILON &&
						fabs(v_search_pos.z - v1.z) < FLT_EPSILON;

					bool match_pos2 =
						fabs(v_search_pos.x - v2.x) < FLT_EPSILON &&
						fabs(v_search_pos.y - v2.y) < FLT_EPSILON &&
						fabs(v_search_pos.z - v2.z) < FLT_EPSILON;

					if (match_pos0 || match_pos1 || match_pos2)
					{
						XMVECTOR U = XMLoadFloat3(&v2) - XMLoadFloat3(&v0);
						XMVECTOR V = XMLoadFloat3(&v1) - XMLoadFloat3(&v0);

						XMVECTOR N = XMVector3Cross(U, V);
						N = XMVector3Normalize(N);

						XMFLOAT3 normal;
						XMStoreFloat3(&normal, N);

						vertex_normals[i].x += normal.x;
						vertex_normals[i].y += normal.y;
						vertex_normals[i].z += normal.z;
					}

				}
			}

			// 3.) Find duplicated vertices by POSITION and TEXCOORD and SUBSET and remove them:
			for (auto& subset : subsets)
			{
				for (uint32_t i = 0; i < subset.indexCount - 1; i++)
				{
					uint32_t ind0 = indices[subset.indexOffset + (uint32_t)i];
					const XMFLOAT3& p0 = vertex_positions[ind0];
					const XMFLOAT3& n0 = vertex_normals[ind0];
					const XMFLOAT2& t0 = vertex_texcoords[ind0];

					for (uint32_t j = i + 1; j < subset.indexCount; j++)
					{
						uint32_t ind1 = indices[subset.indexOffset + (uint32_t)j];

						if (ind1 == ind0)
						{
							continue;
						}

						const XMFLOAT3& p1 = vertex_positions[ind1];
						const XMFLOAT3& n1 = vertex_normals[ind1];
						const XMFLOAT2& t1 = vertex_texcoords[ind1];

						bool duplicated_pos =
							fabs(p0.x - p1.x) < FLT_EPSILON &&
							fabs(p0.y - p1.y) < FLT_EPSILON &&
							fabs(p0.z - p1.z) < FLT_EPSILON;

						bool duplicated_tex =
							fabs(t0.x - t1.x) < FLT_EPSILON &&
							fabs(t0.y - t1.y) < FLT_EPSILON;

						if (duplicated_pos && duplicated_tex)
						{
							// Erase vertices[ind1] because it is a duplicate:
							if (ind1 < vertex_positions.size())
							{
								vertex_positions.erase(vertex_positions.begin() + ind1);
							}
							if (ind1 < vertex_normals.size())
							{
								vertex_normals.erase(vertex_normals.begin() + ind1);
							}
							if (ind1 < vertex_texcoords.size())
							{
								vertex_texcoords.erase(vertex_texcoords.begin() + ind1);
							}
							if (ind1 < vertex_boneindices.size())
							{
								vertex_boneindices.erase(vertex_boneindices.begin() + ind1);
							}
							if (ind1 < vertex_boneweights.size())
							{
								vertex_boneweights.erase(vertex_boneweights.begin() + ind1);
							}

							// The vertices[ind1] was removed, so each index after that needs to be updated:
							for (auto& index : indices)
							{
								if (index > ind1 && index > 0)
								{
									index--;
								}
								else if (index == ind1)
								{
									index = ind0;
								}
							}

						}

					}
				}

			}

		}
		else
		{
			// Compute hard surface normals:

			std::vector<uint32_t> newIndexBuffer;
			std::vector<XMFLOAT3> newPositionsBuffer;
			std::vector<XMFLOAT3> newNormalsBuffer;
			std::vector<XMFLOAT2> newTexcoordsBuffer;
			std::vector<XMUINT4> newBoneIndicesBuffer;
			std::vector<XMFLOAT4> newBoneWeightsBuffer;

			for (size_t face = 0; face < indices.size() / 3; face++)
			{
				uint32_t i0 = indices[face * 3 + 0];
				uint32_t i1 = indices[face * 3 + 1];
				uint32_t i2 = indices[face * 3 + 2];

				XMFLOAT3& p0 = vertex_positions[i0];
				XMFLOAT3& p1 = vertex_positions[i1];
				XMFLOAT3& p2 = vertex_positions[i2];

				XMVECTOR U = XMLoadFloat3(&p2) - XMLoadFloat3(&p0);
				XMVECTOR V = XMLoadFloat3(&p1) - XMLoadFloat3(&p0);

				XMVECTOR N = XMVector3Cross(U, V);
				N = XMVector3Normalize(N);

				XMFLOAT3 normal;
				XMStoreFloat3(&normal, N);

				newPositionsBuffer.push_back(p0);
				newPositionsBuffer.push_back(p1);
				newPositionsBuffer.push_back(p2);

				newNormalsBuffer.push_back(normal);
				newNormalsBuffer.push_back(normal);
				newNormalsBuffer.push_back(normal);

				newTexcoordsBuffer.push_back(vertex_texcoords[i0]);
				newTexcoordsBuffer.push_back(vertex_texcoords[i1]);
				newTexcoordsBuffer.push_back(vertex_texcoords[i2]);

				if (!vertex_boneindices.empty())
				{
					newBoneIndicesBuffer.push_back(vertex_boneindices[i0]);
					newBoneIndicesBuffer.push_back(vertex_boneindices[i1]);
					newBoneIndicesBuffer.push_back(vertex_boneindices[i2]);
				}

				if (!vertex_boneweights.empty())
				{
					newBoneWeightsBuffer.push_back(vertex_boneweights[i0]);
					newBoneWeightsBuffer.push_back(vertex_boneweights[i1]);
					newBoneWeightsBuffer.push_back(vertex_boneweights[i2]);
				}

				newIndexBuffer.push_back(static_cast<uint32_t>(newIndexBuffer.size()));
				newIndexBuffer.push_back(static_cast<uint32_t>(newIndexBuffer.size()));
				newIndexBuffer.push_back(static_cast<uint32_t>(newIndexBuffer.size()));
			}

			// For hard surface normals, we created a new mesh in the previous loop through faces, so swap data:
			vertex_positions = newPositionsBuffer;
			vertex_normals = newNormalsBuffer;
			vertex_texcoords = newTexcoordsBuffer;
			if (!vertex_boneindices.empty())
			{
				vertex_boneindices = newBoneIndicesBuffer;
			}
			if (!vertex_boneweights.empty())
			{
				vertex_boneweights = newBoneWeightsBuffer;
			}
			indices = newIndexBuffer;
		}

		// Restore subsets:


		CreateRenderData();
	}
	void MeshComponent::FlipCulling()
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

		CreateRenderData();
	}
	void MeshComponent::FlipNormals()
	{
		for (auto& normal : vertex_normals)
		{
			normal.x *= -1;
			normal.y *= -1;
			normal.z *= -1;
		}

		CreateRenderData();
	}



	void CameraComponent::CreatePerspective(float newWidth, float newHeight, float newNear, float newFar, float newFOV)
	{
		zNearP = newNear;
		zFarP = newFar;
		width = newWidth;
		height = newHeight;
		fov = newFOV;
		Eye = XMFLOAT3(0, 0, 0);
		At = XMFLOAT3(0, 0, 1);
		Up = XMFLOAT3(0, 1, 0);

		UpdateProjection();
		UpdateCamera();
	}
	void CameraComponent::UpdateProjection()
	{
		XMStoreFloat4x4(&Projection, XMMatrixPerspectiveFovLH(fov, width / height, zFarP, zNearP)); // reverse zbuffer!
		XMStoreFloat4x4(&realProjection, XMMatrixPerspectiveFovLH(fov, width / height, zNearP, zFarP)); // normal zbuffer!
	}
	void CameraComponent::UpdateCamera(const TransformComponent* transform)
	{
		XMVECTOR _Eye;
		XMVECTOR _At;
		XMVECTOR _Up;

		if (transform != nullptr)
		{
			_Eye = XMLoadFloat3(&transform->translation);
			_At = XMVectorSet(0, 0, 1, 0);
			_Up = XMVectorSet(0, 1, 0, 0);

			XMMATRIX _Rot = XMMatrixRotationQuaternion(XMLoadFloat4(&transform->rotation));
			_At = XMVector3TransformNormal(_At, _Rot);
			_Up = XMVector3TransformNormal(_Up, _Rot);
			XMStoreFloat3x3(&rotationMatrix, _Rot);
		}
		else
		{
			_Eye = XMLoadFloat3(&Eye);
			_At = XMLoadFloat3(&At);
			_Up = XMLoadFloat3(&Up);
		}

		XMMATRIX _P = XMLoadFloat4x4(&Projection);
		XMMATRIX _InvP = XMMatrixInverse(nullptr, _P);
		XMStoreFloat4x4(&InvProjection, _InvP);

		XMMATRIX _V = XMMatrixLookToLH(_Eye, _At, _Up);
		XMMATRIX _VP = XMMatrixMultiply(_V, _P);
		XMStoreFloat4x4(&View, _V);
		XMStoreFloat4x4(&VP, _VP);
		XMStoreFloat4x4(&InvView, XMMatrixInverse(nullptr, _V));
		XMStoreFloat4x4(&InvVP, XMMatrixInverse(nullptr, _VP));
		XMStoreFloat4x4(&Projection, _P);
		XMStoreFloat4x4(&InvProjection, XMMatrixInverse(nullptr, _P));

		XMStoreFloat3(&Eye, _Eye);
		XMStoreFloat3(&At, _At);
		XMStoreFloat3(&Up, _Up);

		frustum.ConstructFrustum(zFarP, realProjection, View);
	}
	void CameraComponent::Reflect(const XMFLOAT4& plane)
	{
		XMVECTOR _Eye = XMLoadFloat3(&Eye);
		XMVECTOR _At = XMLoadFloat3(&At);
		XMVECTOR _Up = XMLoadFloat3(&Up);
		XMMATRIX _Ref = XMMatrixReflect(XMLoadFloat4(&plane));

		_Eye = XMVector3Transform(_Eye, _Ref);
		_At = XMVector3TransformNormal(_At, _Ref);
		_Up = XMVector3TransformNormal(_Up, _Ref);

		XMStoreFloat3(&Eye, _Eye);
		XMStoreFloat3(&At, _At);
		XMStoreFloat3(&Up, _Up);

		UpdateCamera();
	}


	void Scene::Update(float dt)
	{

		RunPreviousFrameTransformUpdateSystem(transforms, prev_transforms);

		RunPhysicsUpdateSystem(transforms, meshes, objects, rigidbodies, softbodies, dt);

		RunTransformUpdateSystem(transforms);

		RunAnimationUpdateSystem(animations, transforms, dt);

		RunHierarchyUpdateSystem(parents, transforms, layers);

		RunArmatureUpdateSystem(transforms, armatures);

		RunMaterialUpdateSystem(materials, dt);

		RunObjectUpdateSystem(transforms, meshes, materials, objects, aabb_objects, bounds, waterPlane);

		RunCameraUpdateSystem(transforms, cameras);

		RunDecalUpdateSystem(transforms, aabb_decals, decals);

		RunProbeUpdateSystem(transforms, aabb_probes, probes);

		RunLightUpdateSystem(*cameras.GetComponent(wiRenderer::getCameraID()), transforms, aabb_lights, lights, sunDirection, sunColor);

	}
	void Scene::Clear()
	{
		for (Entity entity : owned_entities)
		{
			names.Remove(entity);
			layers.Remove(entity);
			transforms.Remove(entity);
			prev_transforms.Remove(entity);
			parents.Remove(entity);
			materials.Remove(entity);
			meshes.Remove(entity);
			objects.Remove(entity);
			aabb_objects.Remove(entity);
			rigidbodies.Remove(entity);
			softbodies.Remove(entity);
			armatures.Remove(entity);
			lights.Remove(entity);
			aabb_lights.Remove(entity);
			cameras.Remove(entity);
			probes.Remove(entity);
			aabb_probes.Remove(entity);
			forces.Remove(entity);
			decals.Remove(entity);
			aabb_decals.Remove(entity);
			animations.Remove(entity);
			models.Remove(entity);
		}

		owned_entities.clear();
	}

	void Scene::Entity_Remove(Entity entity)
	{
		owned_entities.erase(entity);

		names.Remove(entity);
		layers.Remove(entity);
		transforms.Remove(entity);
		prev_transforms.Remove(entity);
		parents.Remove_KeepSorted(entity);
		materials.Remove(entity);
		meshes.Remove(entity);
		objects.Remove(entity);
		aabb_objects.Remove(entity);
		rigidbodies.Remove(entity);
		softbodies.Remove(entity);
		armatures.Remove(entity);
		lights.Remove(entity);
		aabb_lights.Remove(entity);
		cameras.Remove(entity);
		probes.Remove(entity);
		aabb_probes.Remove(entity);
		forces.Remove(entity);
		decals.Remove(entity);
		aabb_decals.Remove(entity);
		animations.Remove(entity);
		models.Remove(entity);
	}
	Entity Scene::Entity_FindByName(const std::string& name)
	{
		for (size_t i = 0; i < names.GetCount(); ++i)
		{
			if (names[i] == name)
			{
				return names.GetEntity(i);
			}
		}
		return INVALID_ENTITY;
	}
	Entity Scene::Entity_CreateModel(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		models.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateMaterial(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		materials.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateObject(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		prev_transforms.Create(entity);

		aabb_objects.Create(entity);

		objects.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateMesh(
		const std::string& name
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		meshes.Create(entity);

		return entity;
	}
	Entity Scene::Entity_CreateLight(
		const std::string& name,
		const XMFLOAT3& position,
		const XMFLOAT3& color,
		float energy,
		float range)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity).Translate(position);
		transforms.GetComponent(entity)->UpdateTransform();

		aabb_lights.Create(entity).createFromHalfWidth(position, XMFLOAT3(range, range, range));

		LightComponent& light = lights.Create(entity);
		light.energy = energy;
		light.range = range;
		light.fov = XM_PIDIV4;
		light.color = color;
		light.SetType(LightComponent::POINT);

		return entity;
	}
	Entity Scene::Entity_CreateForce(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity).Translate(position);
		transforms.GetComponent(entity)->UpdateTransform();

		ForceFieldComponent& force = forces.Create(entity);
		force.gravity = 0;
		force.range = 0;
		force.type = ENTITY_TYPE_FORCEFIELD_POINT;

		return entity;
	}
	Entity Scene::Entity_CreateEnvironmentProbe(
		const std::string& name,
		const XMFLOAT3& position
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity).Translate(position);
		transforms.GetComponent(entity)->UpdateTransform();

		aabb_probes.Create(entity);

		EnvironmentProbeComponent& probe = probes.Create(entity);
		probe.isUpToDate = false;
		probe.realTime = false;
		probe.textureIndex = -1;

		return entity;
	}
	Entity Scene::Entity_CreateDecal(
		const std::string& name,
		const std::string& textureName,
		const std::string& normalMapName
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		aabb_decals.Create(entity);

		DecalComponent& decal = decals.Create(entity);
		decal.textureName = textureName;
		decal.normalMapName = normalMapName;

		if (!decal.textureName.empty())
		{
			decal.texture = (Texture2D*)wiResourceManager::GetGlobal()->add(decal.textureName);
		}
		if (!decal.normalMapName.empty())
		{
			decal.normal = (Texture2D*)wiResourceManager::GetGlobal()->add(decal.normalMapName);
		}

		return entity;
	}
	Entity Scene::Entity_CreateCamera(
		const std::string& name,
		float width, float height, float nearPlane, float farPlane, float fov
	)
	{
		Entity entity = CreateEntity();

		owned_entities.insert(entity);

		names.Create(entity) = name;

		layers.Create(entity);

		transforms.Create(entity);

		CameraComponent& camera = cameras.Create(entity);
		camera.CreatePerspective(width, height, nearPlane, farPlane, fov);

		return entity;
	}

	void Scene::Component_Attach(Entity entity, Entity parent)
	{
		assert(entity != parent);

		if (parents.Contains(entity))
		{
			Component_Detach(entity);
		}

		// Add a new hierarchy node to the end of container:
		parents.Create(entity).parentID = parent;

		// If this entity was already a part of a tree however, we must move it before children:
		for (size_t i = 0; i < parents.GetCount(); ++i)
		{
			const ParentComponent& parent = parents[i];
			
			if (parent.parentID == entity)
			{
				parents.MoveLastTo(i);
				break;
			}
		}

		// Re-query parent after potential MoveLastTo(), because it invalidates references:
		ParentComponent& parentcomponent = *parents.GetComponent(entity);

		TransformComponent* transform_parent = transforms.GetComponent(parent);
		if (transform_parent != nullptr)
		{
			// Save the parent's inverse worldmatrix:
			XMStoreFloat4x4(&parentcomponent.world_parent_inverse_bind, XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent->world)));
		}

		TransformComponent* transform_child = transforms.GetComponent(entity);
		if (transform_child != nullptr)
		{
			// Child updated immediately, to that it can be immediately attached to afterwards:
			transform_child->UpdateParentedTransform(*transform_parent, parentcomponent.world_parent_inverse_bind);
		}

		LayerComponent* layer_child = layers.GetComponent(entity);
		if (layer_child != nullptr)
		{
			// Save the initial layermask of the child so that it can be restored if detached:
			parentcomponent.layerMask_bind = layer_child->GetLayerMask();
		}
	}
	void Scene::Component_Detach(Entity entity)
	{
		const ParentComponent* parent = parents.GetComponent(entity);

		if (parent != nullptr)
		{
			TransformComponent* transform = transforms.GetComponent(entity);
			if (transform != nullptr)
			{
				transform->ApplyTransform();
			}

			LayerComponent* layer = layers.GetComponent(entity);
			if (layer != nullptr)
			{
				layer->layerMask = parent->layerMask_bind;
			}

			parents.Remove_KeepSorted(entity);
		}
	}
	void Scene::Component_DetachChildren(Entity parent)
	{
		for (size_t i = 0; i < parents.GetCount(); )
		{
			if (parents[i].parentID == parent)
			{
				Entity entity = parents.GetEntity(i);
				Component_Detach(entity);
			}
			else
			{
				++i;
			}
		}
	}




	void RunPreviousFrameTransformUpdateSystem(
		const ComponentManager<TransformComponent>& transforms,
		ComponentManager<PreviousFrameTransformComponent>& prev_transforms
	)
	{
		for (size_t i = 0; i < prev_transforms.GetCount(); ++i)
		{
			PreviousFrameTransformComponent& prev_transform = prev_transforms[i];
			Entity entity = prev_transforms.GetEntity(i);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			prev_transform.world_prev = transform.world;
		}
	}
	void RunPhysicsUpdateSystem(
		ComponentManager<TransformComponent>& transforms,
		ComponentManager<MeshComponent>& meshes,
		ComponentManager<ObjectComponent>& objects,
		ComponentManager<RigidBodyPhysicsComponent>& rigidbodies,
		ComponentManager<SoftBodyPhysicsComponent>& softbodies,
		float dt
	)
	{
		PHYSICS* physicsEngine = wiRenderer::physicsEngine;

		if (physicsEngine != nullptr)
		{
			physicsEngine->Update(dt);

			// Rigid bodies - Objects:
			for (size_t i = 0; i < rigidbodies.GetCount(); ++i)
			{
				RigidBodyPhysicsComponent& rigidbody = rigidbodies[i];
				Entity entity = rigidbodies.GetEntity(i);
				TransformComponent& transform = *transforms.GetComponent(entity);

				if (rigidbody.kinematic)
				{
					physicsEngine->transformBody(transform.rotation, transform.translation, rigidbody.physicsObjectID);
				}
				else
				{
					PHYSICS::PhysicsTransform* pt = physicsEngine->getObject(rigidbody.physicsObjectID);
					transform.translation_local = pt->position;
					transform.rotation_local = pt->rotation;
					transform.dirty = true;
				}

			}

			// Soft bodies - Meshes:
			for (size_t i = 0; i < softbodies.GetCount(); ++i)
			{
				SoftBodyPhysicsComponent& softbody = softbodies[i];
				Entity entity = softbodies.GetEntity(i);

				MeshComponent& mesh = *meshes.GetComponent(entity);
				mesh.dynamicVB = true;

			}

			physicsEngine->NextRunWorld();
		}

	}
	void RunTransformUpdateSystem(ComponentManager<TransformComponent>& transforms)
	{
		for (size_t i = 0; i < transforms.GetCount(); ++i)
		{
			TransformComponent& transform = transforms[i];
			transform.UpdateTransform();
		}
	}
	void RunAnimationUpdateSystem(
		ComponentManager<AnimationComponent>& animations,
		ComponentManager<TransformComponent>& transforms,
		float dt
	)
	{
		for (size_t i = 0; i < animations.GetCount(); ++i)
		{
			AnimationComponent& animation = animations[i];
			if (!animation.playing && animation.timer == 0.0f)
			{
				continue;
			}

			for (auto& channel : animation.channels)
			{
				if (channel.keyframe_times.empty())
				{
					// No keyframes!
					assert(0);
					continue;
				}

				int keyLeft = 0;
				int keyRight = 0;

				if (channel.keyframe_times.back() < animation.timer)
				{
					// Rightmost keyframe is already outside animation, so just snap to last keyframe:
					keyLeft = keyRight = (int)channel.keyframe_times.size() - 1;
				}
				else
				{
					// Search for the right keyframe (greater/equal to anim time):
					while (channel.keyframe_times[keyRight++] < animation.timer) {}
					keyRight--;

					// Left keyframe is just near right:
					keyLeft = max(0, keyRight - 1);
				}

				float left = channel.keyframe_times[keyLeft];

				TransformComponent& transform = *transforms.GetComponent(channel.target);

				if (channel.mode == AnimationComponent::AnimationChannel::Mode::STEP || keyLeft == keyRight)
				{
					// Nearest neighbor method (snap to left):
					switch (channel.type)
					{
					case AnimationComponent::AnimationChannel::Type::TRANSLATION:
					{
						assert(channel.keyframe_data.size() == channel.keyframe_times.size() * 3);
						transform.translation = ((const XMFLOAT3*)channel.keyframe_data.data())[keyLeft];
					}
					break;
					case AnimationComponent::AnimationChannel::Type::ROTATION:
					{
						assert(channel.keyframe_data.size() == channel.keyframe_times.size() * 4);
						transform.rotation = ((const XMFLOAT4*)channel.keyframe_data.data())[keyLeft];
					}
					break;
					case AnimationComponent::AnimationChannel::Type::SCALE:
					{
						assert(channel.keyframe_data.size() == channel.keyframe_times.size() * 3);
						transform.scale = ((const XMFLOAT3*)channel.keyframe_data.data())[keyLeft];
					}
					break;
					}
				}
				else
				{
					// Linear interpolation method:
					float right = channel.keyframe_times[keyRight];
					float t = (animation.timer - left) / (right - left);

					switch (channel.type)
					{
					case AnimationComponent::AnimationChannel::Type::TRANSLATION:
					{
						assert(channel.keyframe_data.size() == channel.keyframe_times.size() * 3);
						const XMFLOAT3* data = (const XMFLOAT3*)channel.keyframe_data.data();
						XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft]);
						XMVECTOR vRight = XMLoadFloat3(&data[keyRight]);
						XMVECTOR vAnim = XMVectorLerp(vLeft, vRight, t);
						XMStoreFloat3(&transform.translation, vAnim);
					}
					break;
					case AnimationComponent::AnimationChannel::Type::ROTATION:
					{
						assert(channel.keyframe_data.size() == channel.keyframe_times.size() * 4);
						const XMFLOAT4* data = (const XMFLOAT4*)channel.keyframe_data.data();
						XMVECTOR vLeft = XMLoadFloat4(&data[keyLeft]);
						XMVECTOR vRight = XMLoadFloat4(&data[keyRight]);
						XMVECTOR vAnim = XMQuaternionSlerp(vLeft, vRight, t);
						vAnim = XMQuaternionNormalize(vAnim);
						XMStoreFloat4(&transform.rotation, vAnim);
					}
					break;
					case AnimationComponent::AnimationChannel::Type::SCALE:
					{
						assert(channel.keyframe_data.size() == channel.keyframe_times.size() * 3);
						const XMFLOAT3* data = (const XMFLOAT3*)channel.keyframe_data.data();
						XMVECTOR vLeft = XMLoadFloat3(&data[keyLeft]);
						XMVECTOR vRight = XMLoadFloat3(&data[keyRight]);
						XMVECTOR vAnim = XMVectorLerp(vLeft, vRight, t);
						XMStoreFloat3(&transform.scale, vAnim);
					}
					break;
					}
				}

				XMVECTOR S = XMLoadFloat3(&transform.scale);
				XMVECTOR R = XMLoadFloat4(&transform.rotation);
				XMVECTOR T = XMLoadFloat3(&transform.translation);
				XMMATRIX W = XMMatrixScalingFromVector(S) * XMMatrixRotationQuaternion(R) * XMMatrixTranslationFromVector(T);
				XMStoreFloat4x4(&transform.world, W);
				transform.dirty = true;

			}

			if (animation.playing)
			{
				animation.timer += dt;
			}

			if (animation.looped && animation.timer > animation.length)
			{
				animation.timer = 0.0f;
			}

			animation.timer = min(animation.timer, animation.length);
		}
	}
	void RunHierarchyUpdateSystem(
		const ComponentManager<ParentComponent>& parents,
		ComponentManager<TransformComponent>& transforms,
		ComponentManager<LayerComponent>& layers
		)
	{
		for (size_t i = 0; i < parents.GetCount(); ++i)
		{
			const ParentComponent& parentcomponent = parents[i];
			Entity entity = parents.GetEntity(i);

			TransformComponent* transform_child = transforms.GetComponent(entity);
			TransformComponent* transform_parent = transforms.GetComponent(parentcomponent.parentID);
			if (transform_child != nullptr && transform_parent != nullptr)
			{
				transform_child->UpdateParentedTransform(*transform_parent, parentcomponent.world_parent_inverse_bind);
			}


			LayerComponent* layer_child = layers.GetComponent(entity);
			LayerComponent* layer_parent = layers.GetComponent(parentcomponent.parentID);
			if (layer_child != nullptr && layer_parent != nullptr)
			{
				layer_child->layerMask = parentcomponent.layerMask_bind & layer_parent->GetLayerMask();
			}

		}
	}
	void RunArmatureUpdateSystem(
		const ComponentManager<TransformComponent>& transforms,
		ComponentManager<ArmatureComponent>& armatures
	)
	{
		for (size_t i = 0; i < armatures.GetCount(); ++i)
		{
			ArmatureComponent& armature = armatures[i];
			Entity entity = armatures.GetEntity(i);

			if (armature.skinningMatrices.size() != armature.boneCollection.size())
			{
				armature.skinningMatrices.resize(armature.boneCollection.size());
			}

			XMMATRIX R = XMLoadFloat4x4(&armature.remapMatrix);

			int boneIndex = 0;
			for (Entity boneEntity : armature.boneCollection)
			{
				const TransformComponent& bone = *transforms.GetComponent(boneEntity);

				XMMATRIX B = XMLoadFloat4x4(&armature.inverseBindMatrices[boneIndex]);
				XMMATRIX W = XMLoadFloat4x4(&bone.world);
				XMMATRIX M = B * W * R;

				XMStoreFloat4x4(&armature.skinningMatrices[boneIndex++], M);
			}

		}
	}
	void RunMaterialUpdateSystem(ComponentManager<MaterialComponent>& materials, float dt)
	{
		for (size_t i = 0; i < materials.GetCount(); ++i)
		{
			MaterialComponent& material = materials[i];

			material.texAnimSleep -= dt * material.texAnimFrameRate;
			if (material.texAnimSleep <= 0)
			{
				material.texMulAdd.z = fmodf(material.texMulAdd.z + material.texAnimDirection.x, 1);
				material.texMulAdd.w = fmodf(material.texMulAdd.w + material.texAnimDirection.y, 1);
				material.texAnimSleep = 1.0f;

				material.dirty = true; // will trigger constant buffer update!
			}

			material.engineStencilRef = STENCILREF_DEFAULT;
			if (material.subsurfaceScattering > 0)
			{
				material.engineStencilRef = STENCILREF_SKIN;
			}

		}
	}
	void RunObjectUpdateSystem(
		const ComponentManager<TransformComponent>& transforms,
		const ComponentManager<MeshComponent>& meshes,
		const ComponentManager<MaterialComponent>& materials,
		ComponentManager<ObjectComponent>& objects,
		wiECS::ComponentManager<AABB>& aabb_objects,
		AABB& sceneBounds,
		XMFLOAT4& waterPlane
	)
	{
		assert(objects.GetCount() == aabb_objects.GetCount());

		sceneBounds.create(XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX), XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX));

		for (size_t i = 0; i < objects.GetCount(); ++i)
		{
			ObjectComponent& object = objects[i];
			Entity entity = objects.GetEntity(i);
			AABB& aabb = aabb_objects[i];

			aabb.create(XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX), XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
			object.rendertypeMask = 0;
			object.dynamic = false;
			object.cast_shadow = false;

			if (object.meshID != INVALID_ENTITY)
			{
				const TransformComponent* transform = transforms.GetComponent(entity);
				const MeshComponent* mesh = meshes.GetComponent(object.meshID);

				if (mesh != nullptr && transform != nullptr)
				{
					aabb = mesh->aabb.get(transform->world);
					sceneBounds = AABB::Merge(sceneBounds, aabb);

					if (mesh->IsSkinned() || mesh->IsDynamicVB())
					{
						object.dynamic = true;
					}

					for (auto& subset : mesh->subsets)
					{
						const MaterialComponent* material = materials.GetComponent(subset.materialID);

						if (material != nullptr)
						{
							if (material->IsTransparent())
							{
								object.rendertypeMask |= RENDERTYPE_TRANSPARENT;
							}
							else
							{
								object.rendertypeMask |= RENDERTYPE_OPAQUE;
							}

							if (material->IsWater())
							{
								object.rendertypeMask |= RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER;

								XMVECTOR _refPlane = XMPlaneFromPointNormal(XMLoadFloat3(&transform->translation), XMVectorSet(0, 1, 0, 0));
								XMStoreFloat4(&waterPlane, _refPlane);
							}

							object.cast_shadow |= material->cast_shadow;
						}
					}
				}
			}
		}
	}
	void RunCameraUpdateSystem(
		const ComponentManager<TransformComponent>& transforms,
		ComponentManager<CameraComponent>& cameras
	)
	{
		for (size_t i = 0; i < cameras.GetCount(); ++i)
		{
			CameraComponent& camera = cameras[i];
			Entity entity = cameras.GetEntity(i);
			const TransformComponent* transform = transforms.GetComponent(entity);
			camera.UpdateCamera(transform);
		}
	}
	void RunDecalUpdateSystem(
		const ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<AABB>& aabb_decals,
		ComponentManager<DecalComponent>& decals
	)
	{
		assert(decals.GetCount() == aabb_decals.GetCount());

		for (size_t i = 0; i < decals.GetCount(); ++i)
		{
			DecalComponent& decal = decals[i];
			Entity entity = decals.GetEntity(i);
			const TransformComponent& transform = *transforms.GetComponent(entity);
			decal.world = transform.world;
			XMVECTOR front = XMVectorSet(0, 0, 1, 0);
			front = XMVector3TransformNormal(front, XMLoadFloat4x4(&decal.world));
			XMStoreFloat3(&decal.front, front);

			AABB& aabb = aabb_decals[i];
			aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
			aabb = aabb.get(transform.world);
		}
	}
	void RunProbeUpdateSystem(
		const ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<AABB>& aabb_probes,
		ComponentManager<EnvironmentProbeComponent>& probes
	)
	{
		assert(probes.GetCount() == aabb_probes.GetCount());

		for (size_t i = 0; i < probes.GetCount(); ++i)
		{
			EnvironmentProbeComponent& probe = probes[i];
			Entity entity = probes.GetEntity(i);
			const TransformComponent& transform = *transforms.GetComponent(entity);

			probe.position = transform.translation;

			AABB& aabb = aabb_probes[i];
			aabb.createFromHalfWidth(XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1));
			aabb = aabb.get(transform.world);
		}
	}
	void RunLightUpdateSystem(
		const CameraComponent& cascadeCamera,
		const ComponentManager<TransformComponent>& transforms,
		wiECS::ComponentManager<AABB>& aabb_lights,
		ComponentManager<LightComponent>& lights,
		XMFLOAT3& sunDirection, XMFLOAT3& sunColor
	)
	{
		assert(lights.GetCount() == aabb_lights.GetCount());

		for (size_t i = 0; i < lights.GetCount(); ++i)
		{
			LightComponent& light = lights[i];
			Entity entity = lights.GetEntity(i);
			const TransformComponent& transform = *transforms.GetComponent(entity);
			AABB& aabb = aabb_lights[i];

			XMMATRIX world = XMLoadFloat4x4(&transform.world);
			XMVECTOR translation = XMLoadFloat3(&transform.translation);
			XMVECTOR rotation = XMLoadFloat4(&transform.rotation);

			XMStoreFloat3(&light.direction, XMVector3TransformNormal(XMVectorSet(0, 1, 0, 1), world));

			switch (light.type)
			{
			case LightComponent::DIRECTIONAL:
			{
				sunDirection = light.direction;
				sunColor = light.color;

				if (light.shadow)
				{
					XMFLOAT2 screen = XMFLOAT2((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);
					float nearPlane = cascadeCamera.zNearP;
					float farPlane = cascadeCamera.zFarP;
					XMMATRIX view = cascadeCamera.GetView();
					XMMATRIX projection = cascadeCamera.GetRealProjection();
					XMMATRIX world = XMMatrixIdentity();

					// Set up three shadow cascades (far - mid - near):
					const float referenceFrustumDepth = 800.0f;									// this was the frustum depth used for reference
					const float currentFrustumDepth = farPlane - nearPlane;						// current frustum depth
					const float lerp0 = referenceFrustumDepth / currentFrustumDepth * 0.5f;		// third slice distance from cam (percentage)
					const float lerp1 = referenceFrustumDepth / currentFrustumDepth * 0.12f;	// second slice distance from cam (percentage)
					const float lerp2 = referenceFrustumDepth / currentFrustumDepth * 0.016f;	// first slice distance from cam (percentage)

					// Create shadow cascades for main camera:
					if (light.shadowCam_dirLight.empty())
					{
						light.shadowCam_dirLight.resize(3);
					}

					// Place the shadow cascades inside the viewport:
					if (!light.shadowCam_dirLight.empty())
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
						light.shadowCam_dirLight[0] = LightComponent::SHCAM(size0, rotDefault, -farPlane * 0.5f, farPlane * 0.5f);
						light.shadowCam_dirLight[1] = LightComponent::SHCAM(size1, rotDefault, -farPlane * 0.5f, farPlane * 0.5f);
						light.shadowCam_dirLight[2] = LightComponent::SHCAM(size2, rotDefault, -farPlane * 0.5f, farPlane * 0.5f);

						// frustum center - near
						XMVECTOR c = XMVector3Unproject(XMVectorSet(screen.x * 0.5f, screen.y * 0.5f, 0, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);
						// frustum center - far
						XMVECTOR d = XMVector3Unproject(XMVectorSet(screen.x * 0.5f, screen.y * 0.5f, 1, 1), 0, 0, screen.x, screen.y, 0.0f, 1.0f, projection, view, world);

						// Avoid shadowmap texel swimming by aligning them to a discrete grid:
						float f0 = light.shadowCam_dirLight[0].size / (float)wiRenderer::SHADOWRES_2D;
						float f1 = light.shadowCam_dirLight[1].size / (float)wiRenderer::SHADOWRES_2D;
						float f2 = light.shadowCam_dirLight[2].size / (float)wiRenderer::SHADOWRES_2D;
						XMVECTOR e0 = XMVectorFloor(XMVectorLerp(c, d, lerp0) / f0) * f0;
						XMVECTOR e1 = XMVectorFloor(XMVectorLerp(c, d, lerp1) / f1) * f1;
						XMVECTOR e2 = XMVectorFloor(XMVectorLerp(c, d, lerp2) / f2) * f2;

						XMMATRIX rrr = XMMatrixRotationQuaternion(XMLoadFloat4(&transform.rotation));

						light.shadowCam_dirLight[0].Update(rrr, e0);
						light.shadowCam_dirLight[1].Update(rrr, e1);
						light.shadowCam_dirLight[2].Update(rrr, e2);
					}
				}

				aabb.createFromHalfWidth(wiRenderer::getCamera()->Eye, XMFLOAT3(10000, 10000, 10000));
			}
			break;
			case LightComponent::SPOT:
			{
				if (light.shadow)
				{
					if (light.shadowCam_spotLight.empty())
					{
						light.shadowCam_spotLight.push_back(LightComponent::SHCAM(XMFLOAT4(0, 0, 0, 1), 0.1f, 1000.0f, light.fov));
					}
					light.shadowCam_spotLight[0].Update(world);
					light.shadowCam_spotLight[0].farplane = light.range;
					light.shadowCam_spotLight[0].Create_Perspective(light.fov);
				}

				aabb.createFromHalfWidth(transform.translation, XMFLOAT3(light.range, light.range, light.range));
			}
			break;
			case LightComponent::POINT:
			case LightComponent::SPHERE:
			case LightComponent::DISC:
			case LightComponent::RECTANGLE:
			case LightComponent::TUBE:
			{
				if (light.shadow)
				{
					if (light.shadowCam_pointLight.empty())
					{
						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0.5f, -0.5f, -0.5f, -0.5f), 0.1f, 1000.0f, XM_PIDIV2)); //+x
						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0.5f, 0.5f, 0.5f, -0.5f), 0.1f, 1000.0f, XM_PIDIV2)); //-x

						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(1, 0, 0, -0), 0.1f, 1000.0f, XM_PIDIV2)); //+y
						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0, 0, 0, -1), 0.1f, 1000.0f, XM_PIDIV2)); //-y

						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0.707f, 0, 0, -0.707f), 0.1f, 1000.0f, XM_PIDIV2)); //+z
						light.shadowCam_pointLight.push_back(LightComponent::SHCAM(XMFLOAT4(0, 0.707f, 0.707f, 0), 0.1f, 1000.0f, XM_PIDIV2)); //-z
					}
					for (auto& x : light.shadowCam_pointLight) {
						x.Update(translation);
						x.farplane = max(1.0f, light.range);
						x.Create_Perspective(XM_PIDIV2);
					}
				}

				if (light.type == LightComponent::POINT)
				{
					aabb.createFromHalfWidth(transform.translation, XMFLOAT3(light.range, light.range, light.range));
				}
				else
				{
					// area lights have no bounds, just like directional lights (maybe todo)
					aabb.createFromHalfWidth(wiRenderer::getCamera()->Eye, XMFLOAT3(10000, 10000, 10000));
				}
			}
			break;
			}

		}
	}


}
