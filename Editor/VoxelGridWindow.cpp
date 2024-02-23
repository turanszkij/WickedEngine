#include "stdafx.h"
#include "VoxelGridWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void VoxelGridWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_VOXELGRID " VoxelGrid", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(520, 300));

	closeButton.SetTooltip("Delete VoxelGrid");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().voxel_grids.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 80;
	float y = 0;
	float hei = 18;
	float wid = 200;
	float step = hei + 4;

	infoLabel.Create("");
	infoLabel.SetSize(XMFLOAT2(100, 150));
	AddWidget(&infoLabel);

	dimXInput.Create("DimX");
	dimXInput.SetSize(XMFLOAT2(100, hei));
	dimXInput.SetDescription("Dimension X: ");
	dimXInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		wi::VoxelGrid* voxelgrid = scene.voxel_grids.GetComponent(entity);
		if (voxelgrid == nullptr)
			return;
		voxelgrid->init(uint32_t(args.iValue), voxelgrid->resolution.y, voxelgrid->resolution.z);
	});
	AddWidget(&dimXInput);

	dimYInput.Create("DimY");
	dimYInput.SetSize(XMFLOAT2(100, hei));
	dimYInput.SetDescription("Y: ");
	dimYInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		wi::VoxelGrid* voxelgrid = scene.voxel_grids.GetComponent(entity);
		if (voxelgrid == nullptr)
			return;
		voxelgrid->init(voxelgrid->resolution.x, uint32_t(args.iValue), voxelgrid->resolution.z);
		});
	AddWidget(&dimYInput);

	dimZInput.Create("DimZ");
	dimZInput.SetSize(XMFLOAT2(100, hei));
	dimZInput.SetDescription("Z: ");
	dimZInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		wi::VoxelGrid* voxelgrid = scene.voxel_grids.GetComponent(entity);
		if (voxelgrid == nullptr)
			return;
		voxelgrid->init(voxelgrid->resolution.x, voxelgrid->resolution.y, uint32_t(args.iValue));
		});
	AddWidget(&dimZInput);

	clearButton.Create("Clear voxels " ICON_CLEARVOXELS);
	clearButton.SetSize(XMFLOAT2(100, hei));
	clearButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		wi::VoxelGrid* voxelgrid = scene.voxel_grids.GetComponent(entity);
		if (voxelgrid == nullptr)
			return;
		voxelgrid->cleardata();
	});
	AddWidget(&clearButton);

	generateWholeButton.Create("Generate full grid " ICON_VOXELIZE);
	generateWholeButton.SetTooltip("Generate navigation grid including all meshes.");
	generateWholeButton.SetSize(XMFLOAT2(100, hei));
	generateWholeButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		wi::VoxelGrid* voxelgrid = scene.voxel_grids.GetComponent(entity);
		if (voxelgrid == nullptr)
			return;
		wi::jobsystem::context ctx;
		voxelgrid->cleardata();
		scene.VoxelizeWholeScene(ctx, *voxelgrid);
		wi::jobsystem::Wait(ctx);
	});
	AddWidget(&generateWholeButton);

	generateNavigationButton.Create("Generate navigation grid " ICON_VOXELIZE);
	generateNavigationButton.SetTooltip("Generate navigation grid including all navmeshes (object tagged as navmesh).");
	generateNavigationButton.SetSize(XMFLOAT2(100, hei));
	generateNavigationButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		wi::VoxelGrid* voxelgrid = scene.voxel_grids.GetComponent(entity);
		if (voxelgrid == nullptr)
			return;
		wi::jobsystem::context ctx;
		voxelgrid->cleardata();
		scene.VoxelizeNavigation(ctx, *voxelgrid);
		wi::jobsystem::Wait(ctx);
		});
	AddWidget(&generateNavigationButton);

	debugAllCheckBox.Create("Debug draw all: ");
	debugAllCheckBox.SetTooltip("Draw all voxel grids, whether they are selected or not.");
	debugAllCheckBox.SetSize(XMFLOAT2(hei, hei));
	AddWidget(&debugAllCheckBox);


	SetMinimized(true);
	SetVisible(false);

}

void VoxelGridWindow::SetEntity(Entity entity)
{
	bool changed = this->entity != entity;
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	wi::VoxelGrid* voxelgrid = scene.voxel_grids.GetComponent(entity);

	if (voxelgrid != nullptr)
	{
		std::string infotext = "Voxel grid can be used for navigation. By voxelizing the scene into the grid, path finding can be used on the resulting voxel grid.";
		infotext += "\n\nMemory usage: ";
		infotext += wi::helper::GetMemorySizeText(voxelgrid->get_memory_size());
		infoLabel.SetText(infotext);

		dimXInput.SetValue((int)voxelgrid->resolution.x);
		dimYInput.SetValue((int)voxelgrid->resolution.y);
		dimZInput.SetValue((int)voxelgrid->resolution.z);
	}
}

void VoxelGridWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 90;
	const float margin_right = 40;

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
		const float margin_right = padding;
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

	add_fullwidth(infoLabel);

	const float padding2 = 20;
	const float l = 95;
	const float r = width;
	float w = ((r - l) - padding2 * 2) / 3.0f;
	dimXInput.SetSize(XMFLOAT2(w, dimXInput.GetSize().y));
	dimYInput.SetSize(XMFLOAT2(w, dimYInput.GetSize().y));
	dimZInput.SetSize(XMFLOAT2(w, dimZInput.GetSize().y));
	dimXInput.SetPos(XMFLOAT2(margin_left, y));
	dimYInput.SetPos(XMFLOAT2(dimXInput.GetPos().x + w + padding2, y));
	dimZInput.SetPos(XMFLOAT2(dimYInput.GetPos().x + w + padding2, y));
	y += dimZInput.GetSize().y;
	y += padding;

	add_fullwidth(clearButton);
	add_fullwidth(generateWholeButton);
	add_fullwidth(generateNavigationButton);
	add_right(debugAllCheckBox);
}
