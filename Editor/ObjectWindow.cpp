#include "stdafx.h"
#include "ObjectWindow.h"
#include "wiScene.h"

#include "xatlas.h"

using namespace wi::ecs;
using namespace wi::scene;

static void SetPixel(uint8_t *dest, int destWidth, int x, int y, const uint8_t *color)
{
	uint8_t *pixel = &dest[x * 4 + y * (destWidth * 4)];
	pixel[0] = color[0];
	pixel[1] = color[1];
	pixel[2] = color[2];
	pixel[3] = color[3];
}

// https://github.com/miloyip/line/blob/master/line_bresenham.c
static void RasterizeLine(uint8_t *dest, int destWidth, const int *p1, const int *p2, const uint8_t *color)
{
	const int dx = abs(p2[0] - p1[0]), sx = p1[0] < p2[0] ? 1 : -1;
	const int dy = abs(p2[1] - p1[1]), sy = p1[1] < p2[1] ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2;
	int current[2];
	current[0] = p1[0];
	current[1] = p1[1];
	while (SetPixel(dest, destWidth, current[0], current[1], color), current[0] != p2[0] || current[1] != p2[1])
	{
		const int e2 = err;
		if (e2 > -dx) { err -= dy; current[0] += sx; }
		if (e2 < dy) { err += dx; current[1] += sy; }
	}
}

// https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
static void RasterizeTriangle(uint8_t *dest, int destWidth, const int *t0, const int *t1, const int *t2, const uint8_t *color)
{
	if (t0[1] > t1[1]) std::swap(t0, t1);
	if (t0[1] > t2[1]) std::swap(t0, t2);
	if (t1[1] > t2[1]) std::swap(t1, t2);
	int total_height = t2[1] - t0[1];
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1[1] - t0[1] || t1[1] == t0[1];
		int segment_height = second_half ? t2[1] - t1[1] : t1[1] - t0[1];
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1[1] - t0[1] : 0)) / segment_height;
		int A[2], B[2];
		for (int j = 0; j < 2; j++) {
			A[j] = int(t0[j] + (t2[j] - t0[j]) * alpha);
			B[j] = int(second_half ? t1[j] + (t2[j] - t1[j]) * beta : t0[j] + (t1[j] - t0[j]) * beta);
		}
		if (A[0] > B[0]) std::swap(A, B);
		for (int j = A[0]; j <= B[0]; j++)
			SetPixel(dest, destWidth, j, t0[1] + i, color);
	}
}

struct Atlas_Dim
{
	uint32_t width = 0;
	uint32_t height = 0;
};
static Atlas_Dim GenerateMeshAtlas(MeshComponent& meshcomponent, uint32_t resolution)
{
	Atlas_Dim dim;

	xatlas::Atlas* atlas = xatlas::Create();

	// Prepare mesh to be processed by xatlas:
	{
		xatlas::MeshDecl mesh;
		mesh.vertexCount = (int)meshcomponent.vertex_positions.size();
		mesh.vertexPositionData = meshcomponent.vertex_positions.data();
		mesh.vertexPositionStride = sizeof(float) * 3;
		if (!meshcomponent.vertex_normals.empty()) {
			mesh.vertexNormalData = meshcomponent.vertex_normals.data();
			mesh.vertexNormalStride = sizeof(float) * 3;
		}
		if (!meshcomponent.vertex_uvset_0.empty()) {
			mesh.vertexUvData = meshcomponent.vertex_uvset_0.data();
			mesh.vertexUvStride = sizeof(float) * 2;
		}
		mesh.indexCount = (int)meshcomponent.indices.size();
		mesh.indexData = meshcomponent.indices.data();
		mesh.indexFormat = xatlas::IndexFormat::UInt32;
		xatlas::AddMeshError::Enum error = xatlas::AddMesh(atlas, mesh);
		if (error != xatlas::AddMeshError::Success) {
			wi::helper::messageBox(xatlas::StringForEnum(error), "Adding mesh to xatlas failed!");
			return dim;
		}
	}

	// Generate atlas:
	{
		xatlas::ChartOptions chartoptions;
		xatlas::ParameterizeOptions parametrizeoptions;
		xatlas::PackOptions packoptions;

		packoptions.resolution = resolution;
		packoptions.blockAlign = true;

		xatlas::Generate(atlas, chartoptions, parametrizeoptions, packoptions);
		dim.width = atlas->width;
		dim.height = atlas->height;

		xatlas::Mesh& mesh = atlas->meshes[0];

		// Note: we must recreate all vertex buffers, because the index buffer will be different (the atlas could have removed shared vertices)
		meshcomponent.indices.clear();
		meshcomponent.indices.resize(mesh.indexCount);
		wi::vector<XMFLOAT3> positions(mesh.vertexCount);
		wi::vector<XMFLOAT2> atlas(mesh.vertexCount);
		wi::vector<XMFLOAT3> normals;
		wi::vector<XMFLOAT4> tangents;
		wi::vector<XMFLOAT2> uvset_0;
		wi::vector<XMFLOAT2> uvset_1;
		wi::vector<uint32_t> colors;
		wi::vector<XMUINT4> boneindices;
		wi::vector<XMFLOAT4> boneweights;
		if (!meshcomponent.vertex_normals.empty())
		{
			normals.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_tangents.empty())
		{
			tangents.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_uvset_0.empty())
		{
			uvset_0.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_uvset_1.empty())
		{
			uvset_1.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_colors.empty())
		{
			colors.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_boneindices.empty())
		{
			boneindices.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_boneweights.empty())
		{
			boneweights.resize(mesh.vertexCount);
		}

		for (uint32_t j = 0; j < mesh.indexCount; ++j)
		{
			const uint32_t ind = mesh.indexArray[j];
			const xatlas::Vertex &v = mesh.vertexArray[ind];
			meshcomponent.indices[j] = ind;
			atlas[ind].x = v.uv[0] / float(dim.width);
			atlas[ind].y = v.uv[1] / float(dim.height);
			positions[ind] = meshcomponent.vertex_positions[v.xref];
			if (!normals.empty())
			{
				normals[ind] = meshcomponent.vertex_normals[v.xref];
			}
			if (!tangents.empty())
			{
				tangents[ind] = meshcomponent.vertex_tangents[v.xref];
			}
			if (!uvset_0.empty())
			{
				uvset_0[ind] = meshcomponent.vertex_uvset_0[v.xref];
			}
			if (!uvset_1.empty())
			{
				uvset_1[ind] = meshcomponent.vertex_uvset_1[v.xref];
			}
			if (!colors.empty())
			{
				colors[ind] = meshcomponent.vertex_colors[v.xref];
			}
			if (!boneindices.empty())
			{
				boneindices[ind] = meshcomponent.vertex_boneindices[v.xref];
			}
			if (!boneweights.empty())
			{
				boneweights[ind] = meshcomponent.vertex_boneweights[v.xref];
			}
		}

		meshcomponent.vertex_positions = positions;
		meshcomponent.vertex_atlas = atlas;
		if (!normals.empty())
		{
			meshcomponent.vertex_normals = normals;
		}
		if (!tangents.empty())
		{
			meshcomponent.vertex_tangents = tangents;
		}
		if (!uvset_0.empty())
		{
			meshcomponent.vertex_uvset_0 = uvset_0;
		}
		if (!uvset_1.empty())
		{
			meshcomponent.vertex_uvset_1 = uvset_1;
		}
		if (!colors.empty())
		{
			meshcomponent.vertex_colors = colors;
		}
		if (!boneindices.empty())
		{
			meshcomponent.vertex_boneindices = boneindices;
		}
		if (!boneweights.empty())
		{
			meshcomponent.vertex_boneweights = boneweights;
		}
		meshcomponent.CreateRenderData();

	}

	//// DEBUG
	//{
	//	const uint32_t width = objectcomponent.lightmapWidth;
	//	const uint32_t height = objectcomponent.lightmapHeight;
	//	objectcomponent.lightmapTextureData.resize(width * height * 4);
	//	const xatlas::OutputMesh *mesh = xatlas::GetOutputMeshes(atlas)[0];
	//	// Rasterize mesh triangles.
	//	const uint8_t white[] = { 255, 255, 255 };
	//	for (uint32_t j = 0; j < mesh->indexCount; j += 3) {
	//		int verts[3][2];
	//		uint8_t color[4];
	//		for (int k = 0; k < 3; k++) {
	//			const xatlas::OutputVertex &v = mesh->vertexArray[mesh->indexArray[j + k]];
	//			verts[k][0] = int(v.uv[0]);
	//			verts[k][1] = int(v.uv[1]);
	//			color[k] = rand() % 255;
	//		}
	//		color[3] = 255;
	//		if (!verts[0][0] && !verts[0][1] && !verts[1][0] && !verts[1][1] && !verts[2][0] && !verts[2][1])
	//			continue; // Skip triangles that weren't atlased.
	//		RasterizeTriangle(objectcomponent.lightmapTextureData.data(), width, verts[0], verts[1], verts[2], color);
	//		RasterizeLine(objectcomponent.lightmapTextureData.data(), width, verts[0], verts[1], white);
	//		RasterizeLine(objectcomponent.lightmapTextureData.data(), width, verts[1], verts[2], white);
	//		RasterizeLine(objectcomponent.lightmapTextureData.data(), width, verts[2], verts[0], white);
	//	}
	//}

	xatlas::Destroy(atlas);

	return dim;
}


void ObjectWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_OBJECT " Object", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 740));

	closeButton.SetTooltip("Delete ObjectComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().objects.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 140;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 130;

	meshCombo.Create("Mesh: ");
	meshCombo.SetSize(XMFLOAT2(wid, hei));
	meshCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr)
			return;

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		object->meshID = (Entity)args.userdata;

		editor->RecordEntity(archive, entity);
		});
	AddWidget(&meshCombo);

	renderableCheckBox.Create("Renderable: ");
	renderableCheckBox.SetTooltip("Set object to be participating in rendering.");
	renderableCheckBox.SetSize(XMFLOAT2(hei, hei));
	renderableCheckBox.SetPos(XMFLOAT2(x, y));
	renderableCheckBox.SetCheck(true);
	renderableCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(x.entity);
			if (object != nullptr)
			{
				object->SetRenderable(args.bValue);
			}
		}
	});
	AddWidget(&renderableCheckBox);

	shadowCheckBox.Create("Cast Shadow: ");
	shadowCheckBox.SetTooltip("Set object to be participating in shadows.");
	shadowCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowCheckBox.SetPos(XMFLOAT2(x, y += step));
	shadowCheckBox.SetCheck(true);
	shadowCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(x.entity);
			if (object != nullptr)
			{
				object->SetCastShadow(args.bValue);
			}
		}
		});
	AddWidget(&shadowCheckBox);

	navmeshCheckBox.Create("Navmesh: ");
	navmeshCheckBox.SetTooltip("Set object to be a navigation mesh filtering (FILTER_NAVIGATION_MESH).\nTurning on navmesh also enables BVH for the underlying mesh.");
	navmeshCheckBox.SetSize(XMFLOAT2(hei, hei));
	navmeshCheckBox.SetPos(XMFLOAT2(x, y += step));
	navmeshCheckBox.SetCheck(true);
	navmeshCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(x.entity);
			if (object != nullptr)
			{
				if (args.bValue)
				{
					object->filterMask |= wi::enums::FILTER_NAVIGATION_MESH;

					MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
					if (mesh != nullptr)
					{
						mesh->SetBVHEnabled(args.bValue);
					}
				}
				else
				{
					object->filterMask &= ~wi::enums::FILTER_NAVIGATION_MESH;
				}
			}
		}
		});
	AddWidget(&navmeshCheckBox);

	foregroundCheckBox.Create("Foreground: ");
	foregroundCheckBox.SetTooltip("Set object to be rendered in the foreground.\nThis is useful for first person hands or weapons, so that they not clip into walls and other objects.");
	foregroundCheckBox.SetSize(XMFLOAT2(hei, hei));
	foregroundCheckBox.SetPos(XMFLOAT2(x, y += step));
	foregroundCheckBox.SetCheck(true);
	foregroundCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(x.entity);
			if (object != nullptr)
			{
				object->SetForeground(args.bValue);
			}
		}
		});
	AddWidget(&foregroundCheckBox);

	notVisibleInMainCameraCheckBox.Create("Not visible in main camera: ");
	notVisibleInMainCameraCheckBox.SetTooltip("Set object to be not rendered in the main camera.\nThis is useful for first person character model, as the character will still be rendered in reflections and shadows.");
	notVisibleInMainCameraCheckBox.SetSize(XMFLOAT2(hei, hei));
	notVisibleInMainCameraCheckBox.SetPos(XMFLOAT2(x, y += step));
	notVisibleInMainCameraCheckBox.SetCheck(true);
	notVisibleInMainCameraCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(x.entity);
			if (object != nullptr)
			{
				object->SetNotVisibleInMainCamera(args.bValue);
			}
		}
		});
	AddWidget(&notVisibleInMainCameraCheckBox);

	notVisibleInReflectionsCheckBox.Create("Not visible in reflections: ");
	notVisibleInReflectionsCheckBox.SetTooltip("Set object to be not rendered in the reflections.\nThis is useful for vampires.");
	notVisibleInReflectionsCheckBox.SetSize(XMFLOAT2(hei, hei));
	notVisibleInReflectionsCheckBox.SetPos(XMFLOAT2(x, y += step));
	notVisibleInReflectionsCheckBox.SetCheck(true);
	notVisibleInReflectionsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(x.entity);
			if (object != nullptr)
			{
				object->SetNotVisibleInReflections(args.bValue);
			}
		}
		});
	AddWidget(&notVisibleInReflectionsCheckBox);

	ditherSlider.Create(0, 1, 0, 1000, "Transparency: ");
	ditherSlider.SetTooltip("Adjust transparency of the object. Opaque materials will use dithered transparency in this case!");
	ditherSlider.SetSize(XMFLOAT2(wid, hei));
	ditherSlider.SetPos(XMFLOAT2(x, y += step));
	ditherSlider.OnSlide([&](wi::gui::EventArgs args) {
		ObjectComponent* object = editor->GetCurrentScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->color.w = 1 - args.fValue;
		}
	});
	AddWidget(&ditherSlider);

	cascadeMaskSlider.Create(0, 3, 0, 3, "Cascade Mask: ");
	cascadeMaskSlider.SetTooltip("How many shadow cascades to skip when rendering this object into shadow maps? (0: skip none, it will be in all cascades, 1: skip first (biggest cascade), ...etc...");
	cascadeMaskSlider.SetSize(XMFLOAT2(wid, hei));
	cascadeMaskSlider.SetPos(XMFLOAT2(x, y += step));
	cascadeMaskSlider.OnSlide([&](wi::gui::EventArgs args) {
		ObjectComponent* object = editor->GetCurrentScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->cascadeMask = (uint32_t)args.iValue;
		}
	});
	AddWidget(&cascadeMaskSlider);

	lodSlider.Create(0.001f, 10, 1, 10000, "LOD Multiplier: ");
	lodSlider.SetTooltip("How much the distance to camera will affect LOD selection. (If the mesh has lods)");
	lodSlider.SetSize(XMFLOAT2(wid, hei));
	lodSlider.SetPos(XMFLOAT2(x, y += step));
	lodSlider.OnSlide([&](wi::gui::EventArgs args) {
		ObjectComponent* object = editor->GetCurrentScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->lod_distance_multiplier = args.fValue;
		}
		});
	AddWidget(&lodSlider);

	drawdistanceSlider.Create(0, 1000, 1, 10000, "Draw Distance: ");
	drawdistanceSlider.SetTooltip("Specify the draw distance of the object");
	drawdistanceSlider.SetSize(XMFLOAT2(wid, hei));
	drawdistanceSlider.SetPos(XMFLOAT2(x, y += step));
	drawdistanceSlider.OnSlide([&](wi::gui::EventArgs args) {
		ObjectComponent* object = editor->GetCurrentScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->draw_distance = args.fValue;
		}
		});
	AddWidget(&drawdistanceSlider);

	sortPrioritySlider.Create(0, 15, 0, 15, "Sort Priority: ");
	sortPrioritySlider.SetTooltip("Set to larger value to draw earlier (most useful for transparents with alpha blending, if the sorting order by distance is not good enough)");
	sortPrioritySlider.SetSize(XMFLOAT2(wid, hei));
	sortPrioritySlider.SetPos(XMFLOAT2(x, y += step));
	sortPrioritySlider.OnSlide([&](wi::gui::EventArgs args) {
		ObjectComponent* object = editor->GetCurrentScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->sort_priority = (uint8_t)args.iValue;
		}
		});
	AddWidget(&sortPrioritySlider);

	y += step;


	lightmapResolutionSlider.Create(32, 1024, 128, 1024 - 32, "Lightmap resolution: ");
	lightmapResolutionSlider.SetTooltip("Set the approximate resolution for this object's lightmap. This will be packed into the larger global lightmap later.");
	lightmapResolutionSlider.SetSize(XMFLOAT2(wid, hei));
	lightmapResolutionSlider.SetPos(XMFLOAT2(x, y += step));
	lightmapResolutionSlider.OnSlide([&](wi::gui::EventArgs args) {
		// unfortunately, we must be pow2 with full float lightmap format, otherwise it could be unlimited (but accumulation blending would suffer then)
		//	or at least for me, downloading the lightmap was glitching out when non-pow 2 and RGBA32_FLOAT format
		lightmapResolutionSlider.SetValue(float(wi::math::GetNextPowerOfTwo(uint32_t(args.fValue)))); 
	});
	AddWidget(&lightmapResolutionSlider);

	lightmapBlockCompressionCheckBox.Create("Block Compression (BC6H): ");
	lightmapBlockCompressionCheckBox.SetTooltip("Enabling block compression for lightmaps will reduce memory usage, but it can lead to reduced quality.\nIf enabled, lightmaps will use BC6H block compression.\nIf disabled, lightmaps will use R11G11B10_FLOAT packing.\nChanging this will take effect after a new lightmap was generated.");
	lightmapBlockCompressionCheckBox.SetSize(XMFLOAT2(hei, hei));
	lightmapBlockCompressionCheckBox.SetPos(XMFLOAT2(x, y += step));
	lightmapBlockCompressionCheckBox.SetCheck(true);
	lightmapBlockCompressionCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(x.entity);
			if (object != nullptr)
			{
				object->SetLightmapDisableBlockCompression(!args.bValue);
			}
		}
	});
	AddWidget(&lightmapBlockCompressionCheckBox);

	lightmapSourceUVSetComboBox.Create("UV Set: ");
	lightmapSourceUVSetComboBox.SetPos(XMFLOAT2(x, y += step));
	lightmapSourceUVSetComboBox.SetSize(XMFLOAT2(wid, hei));
	lightmapSourceUVSetComboBox.AddItem("Copy UV 0");
	lightmapSourceUVSetComboBox.AddItem("Copy UV 1");
	lightmapSourceUVSetComboBox.AddItem("Keep Atlas");
	lightmapSourceUVSetComboBox.AddItem("Generate Atlas");
	lightmapSourceUVSetComboBox.SetSelected(3);
	lightmapSourceUVSetComboBox.SetTooltip("Set which UV set to use when generating the lightmap Atlas");
	AddWidget(&lightmapSourceUVSetComboBox);

	generateLightmapButton.Create("Generate Lightmap");
	generateLightmapButton.SetTooltip("Render the lightmap for only this object. It will automatically combined with the global lightmap.");
	generateLightmapButton.SetPos(XMFLOAT2(x, y += step));
	generateLightmapButton.SetSize(XMFLOAT2(wid, hei));
	generateLightmapButton.OnClick([&](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();

		enum UV_GEN_TYPE
		{
			UV_GEN_COPY_UVSET_0,
			UV_GEN_COPY_UVSET_1,
			UV_GEN_KEEP_ATLAS,
			UV_GEN_GENERATE_ATLAS,
		};
		UV_GEN_TYPE gen_type = (UV_GEN_TYPE)lightmapSourceUVSetComboBox.GetSelected();

		wi::unordered_set<ObjectComponent*> gen_objects;
		wi::unordered_map<MeshComponent*, Atlas_Dim> gen_meshes;

		for (auto& x : this->editor->translator.selected)
		{
			ObjectComponent* objectcomponent = scene.objects.GetComponent(x.entity);
			if (objectcomponent != nullptr)
			{
				MeshComponent* meshcomponent = scene.meshes.GetComponent(objectcomponent->meshID);

				if (meshcomponent != nullptr)
				{
					gen_objects.insert(objectcomponent);
					gen_meshes[meshcomponent] = Atlas_Dim();
				}
			}

		}

		wi::jobsystem::context ctx;

		for (auto& it : gen_meshes)
		{
			MeshComponent& mesh = *it.first;
			if (gen_type == UV_GEN_COPY_UVSET_0)
			{
				mesh.vertex_atlas = mesh.vertex_uvset_0;
				mesh.CreateRenderData();
			}
			else if (gen_type == UV_GEN_COPY_UVSET_1)
			{
				mesh.vertex_atlas = mesh.vertex_uvset_1;
				mesh.CreateRenderData();
			}
			else if (gen_type == UV_GEN_GENERATE_ATLAS)
			{
				wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
					it.second = GenerateMeshAtlas(mesh, (uint32_t)lightmapResolutionSlider.GetValue());
				});
			}
		}
		wi::jobsystem::Wait(ctx);

		for (auto& x : gen_objects)
		{
			x->ClearLightmap();
			MeshComponent* meshcomponent = scene.meshes.GetComponent(x->meshID);
			if (gen_type == UV_GEN_GENERATE_ATLAS)
			{
				x->lightmapWidth = gen_meshes.at(meshcomponent).width;
				x->lightmapHeight = gen_meshes.at(meshcomponent).height;
			}
			else
			{
				x->lightmapWidth = x->lightmapHeight = (uint32_t)lightmapResolutionSlider.GetValue();
			}
			x->SetLightmapRenderRequest(true);
		}

		scene.SetAccelerationStructureUpdateRequested(true);

	});
	AddWidget(&generateLightmapButton);

	stopLightmapGenButton.Create("Stop Lightmap Gen");
	stopLightmapGenButton.SetTooltip("Stop the lightmap rendering and save the lightmap.\nIf denoiser is enabled, this is the point at which lightmap will be denoised, which could take a while.");
	stopLightmapGenButton.SetPos(XMFLOAT2(x, y += step));
	stopLightmapGenButton.SetSize(XMFLOAT2(wid, hei));
	stopLightmapGenButton.OnClick([&](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();

		for (auto& x : this->editor->translator.selected)
		{
			ObjectComponent* objectcomponent = scene.objects.GetComponent(x.entity);
			if (objectcomponent != nullptr)
			{
				objectcomponent->SetLightmapRenderRequest(false);
				objectcomponent->SaveLightmap();
			}
		}

	});
	AddWidget(&stopLightmapGenButton);

	clearLightmapButton.Create("Clear Lightmap");
	clearLightmapButton.SetTooltip("Clear the lightmap from this object.");
	clearLightmapButton.SetPos(XMFLOAT2(x, y += step));
	clearLightmapButton.SetSize(XMFLOAT2(wid, hei));
	clearLightmapButton.OnClick([&](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();

		for (auto& x : this->editor->translator.selected)
		{
			ObjectComponent* objectcomponent = scene.objects.GetComponent(x.entity);
			if (objectcomponent != nullptr)
			{
				objectcomponent->ClearLightmap();
			}
		}

	});
	AddWidget(&clearLightmapButton);

	y += step;

	colorComboBox.Create("Color picker mode: ");
	colorComboBox.SetSize(XMFLOAT2(wid, hei));
	colorComboBox.SetPos(XMFLOAT2(x, y += step));
	colorComboBox.AddItem("Base color");
	colorComboBox.AddItem("Emissive color");
	colorComboBox.SetTooltip("Choose the destination data of the color picker.");
	AddWidget(&colorComboBox);

	colorPicker.Create("Object Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.SetPos(XMFLOAT2(5, y += step));
	colorPicker.SetVisible(true);
	colorPicker.SetEnabled(true);
	colorPicker.OnColorChanged([&](wi::gui::EventArgs args) {
		ObjectComponent* object = editor->GetCurrentScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			switch (colorComboBox.GetSelected())
			{
			default:
			case 0:
			{
				XMFLOAT3 col = args.color.toFloat3();
				object->color = XMFLOAT4(col.x, col.y, col.z, object->color.w);
			}
			break;
			case 1:
				object->emissiveColor = args.color.toFloat4();
				break;
			}
		}
		});
	AddWidget(&colorPicker);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


void ObjectWindow::SetEntity(Entity entity)
{
	Scene& scene = editor->GetCurrentScene();
	const ObjectComponent* object = scene.objects.GetComponent(entity);
	if (object == nullptr)
		entity = INVALID_ENTITY;

	if (this->entity == entity)
		return;

	this->entity = entity;

	if (object != nullptr)
	{
		SetEnabled(true);

		meshCombo.ClearItems();
		meshCombo.AddItem("INVALID_ENTITY", (uint64_t)INVALID_ENTITY);
		for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
		{
			Entity meshID = scene.meshes.GetEntity(i);
			const NameComponent* name = scene.names.GetComponent(meshID);
			if (name == nullptr)
			{
				meshCombo.AddItem(std::to_string(meshID), (uint64_t)meshID);
			}
			else
			{
				meshCombo.AddItem(name->name, (uint64_t)meshID);
			}
		}
		meshCombo.SetSelectedByUserdataWithoutCallback(object->meshID);

		renderableCheckBox.SetCheck(object->IsRenderable());
		shadowCheckBox.SetCheck(object->IsCastingShadow());
		foregroundCheckBox.SetCheck(object->IsForeground());
		notVisibleInMainCameraCheckBox.SetCheck(object->IsNotVisibleInMainCamera());
		notVisibleInReflectionsCheckBox.SetCheck(object->IsNotVisibleInReflections());
		navmeshCheckBox.SetCheck(object->filterMask & wi::enums::FILTER_NAVIGATION_MESH);
		cascadeMaskSlider.SetValue((float)object->cascadeMask);
		ditherSlider.SetValue(object->GetTransparency());
		lodSlider.SetValue(object->lod_distance_multiplier);
		drawdistanceSlider.SetValue(object->draw_distance);
		sortPrioritySlider.SetValue((int)object->sort_priority);

		switch (colorComboBox.GetSelected())
		{
		default:
		case 0:
			colorPicker.SetPickColor(wi::Color::fromFloat4(object->color));
			break;
		case 1:
			colorPicker.SetPickColor(wi::Color::fromFloat4(object->emissiveColor));
			break;
		}

		lightmapBlockCompressionCheckBox.SetCheck(!object->IsLightmapDisableBlockCompression());

	}
	else
	{
		SetEnabled(false);
	}


}


void ObjectWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	float margin_left = 140;
	float margin_right = 40;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	margin_left = 80;

	add(meshCombo);

	margin_left = 140;

	add_right(renderableCheckBox);
	add_right(shadowCheckBox);
	add_right(foregroundCheckBox);
	add_right(notVisibleInMainCameraCheckBox);
	add_right(notVisibleInReflectionsCheckBox);
	add_right(navmeshCheckBox);
	add(ditherSlider);
	add(cascadeMaskSlider);
	add(lodSlider);
	add(drawdistanceSlider);
	add(sortPrioritySlider);
	add(colorComboBox);
	add_fullwidth(colorPicker);

	y += jump;
	add(lightmapResolutionSlider);

	margin_left = 80;

	add_right(lightmapBlockCompressionCheckBox);
	add(lightmapSourceUVSetComboBox);
	add(generateLightmapButton);
	add(stopLightmapGenButton);
	add(clearLightmapButton);

}
