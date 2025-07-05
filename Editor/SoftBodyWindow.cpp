#include "stdafx.h"
#include "SoftBodyWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void SoftBodyWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_SOFTBODY " Soft Body Physics", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(580, 320));

	closeButton.SetTooltip("Delete SoftBodyPhysicsComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().softbodies.Remove(entity);

		MeshComponent* mesh = editor->GetCurrentScene().meshes.GetComponent(entity);
		if (mesh != nullptr && mesh->armatureID == INVALID_ENTITY)
		{
			// When removing soft body, and mesh also doesn't have an armature,
			//	then remove the bone vertex buffers
			mesh->vertex_boneindices.clear();
			mesh->vertex_boneweights.clear();
			mesh->vertex_boneindices2.clear();
			mesh->vertex_boneweights2.clear();
			mesh->CreateRenderData();
		}

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	infoLabel.Create("");
	infoLabel.SetText("Soft body physics must be used together with a MeshComponent, otherwise it will have no effect.\nYou can use the Paint Tool to pin or soften soft body vertices.");
	infoLabel.SetFitTextEnabled(true);
	AddWidget(&infoLabel);

	auto forEachSelected = [&] (auto func) {
		return [&, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				SoftBodyPhysicsComponent* physicscomponent = scene.softbodies.GetComponent(x.entity);
				if (physicscomponent == nullptr)
				{
					// Try also getting it through object's mesh:
					ObjectComponent* object = scene.objects.GetComponent(x.entity);
					if (object != nullptr)
					{
						physicscomponent = scene.softbodies.GetComponent(object->meshID);
					}
				}
				if (physicscomponent != nullptr)
				{
					func(physicscomponent, args);
				}
			}
		};
	};

	resetButton.Create("Reset");
	resetButton.SetTooltip("Set the detail to keep between simulation and graphics mesh.\nLower = less detailed, higher = more detailed.");
	resetButton.OnClick(forEachSelected([&] (auto physicscomponent, auto args) {
		physicscomponent->Reset();
	}));
	AddWidget(&resetButton);

	detailSlider.Create(0.001f, 1, 1, 1000, "LOD Detail: ");
	detailSlider.SetTooltip("Set the detail to keep between simulation and graphics mesh.\nLower = less detailed, higher = more detailed.");
	detailSlider.OnSlide(forEachSelected([&] (auto physicscomponent, auto args) {
		physicscomponent->SetDetail(args.fValue);
	}));
	AddWidget(&detailSlider);

	massSlider.Create(0, 100, 1, 100000, "Mass: ");
	massSlider.SetTooltip("Set the mass amount for the physics engine.");
	massSlider.OnSlide(forEachSelected([&] (auto physicscomponent, auto args) {
		physicscomponent->physicsobject = {};
		physicscomponent->mass = args.fValue;
	}));
	AddWidget(&massSlider);

	frictionSlider.Create(0, 1, 0.5f, 100000, "Friction: ");
	frictionSlider.SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider.OnSlide(forEachSelected([&] (auto physicscomponent, auto args) {
		physicscomponent->friction = args.fValue;
	}));
	AddWidget(&frictionSlider);

	restitutionSlider.Create(0, 1, 0, 100000, "Restitution: ");
	restitutionSlider.SetTooltip("Set the restitution amount for the physics engine.");
	restitutionSlider.OnSlide(forEachSelected([&] (auto physicscomponent, auto args) {
		physicscomponent->restitution = args.fValue;
	}));
	AddWidget(&restitutionSlider);

	pressureSlider.Create(0, 100000, 0, 100000, "Pressure: ");
	pressureSlider.SetTooltip("Set the pressure amount for the physics engine.");
	pressureSlider.OnSlide(forEachSelected([&] (auto physicscomponent, auto args) {
		physicscomponent->pressure = args.fValue;
		physicscomponent->physicsobject = {};
	}));
	AddWidget(&pressureSlider);

	vertexRadiusSlider.Create(0, 1, 0, 100000, "Vertex Radius: ");
	vertexRadiusSlider.SetTooltip("Set how much distance vertices should keep from other physics bodies.");
	vertexRadiusSlider.OnSlide(forEachSelected([&] (auto physicscomponent, auto args) {
		physicscomponent->physicsobject = {};
		physicscomponent->vertex_radius = args.fValue;
	}));
	AddWidget(&vertexRadiusSlider);

	windCheckbox.Create("Wind: ");
	windCheckbox.SetTooltip("Enable/disable wind force on this soft body.");
	windCheckbox.OnClick(forEachSelected([&] (auto physicscomponent, auto args) {
		physicscomponent->SetWindEnabled(args.bValue);
	}));
	AddWidget(&windCheckbox);



	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void SoftBodyWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();

	const SoftBodyPhysicsComponent* physicscomponent = scene.softbodies.GetComponent(entity);
	if (physicscomponent != nullptr)
	{
		detailSlider.SetValue(physicscomponent->detail);
		massSlider.SetValue(physicscomponent->mass);
		frictionSlider.SetValue(physicscomponent->friction);
		restitutionSlider.SetValue(physicscomponent->restitution);
		pressureSlider.SetValue(physicscomponent->pressure);
		vertexRadiusSlider.SetValue(physicscomponent->vertex_radius);
		windCheckbox.SetCheck(physicscomponent->IsWindEnabled());
	}
}

void SoftBodyWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 120;

	layout.add_fullwidth(infoLabel);
	layout.add_fullwidth(resetButton);
	layout.add(detailSlider);
	layout.add(massSlider);
	layout.add(frictionSlider);
	layout.add(restitutionSlider);
	layout.add(pressureSlider);
	layout.add(vertexRadiusSlider);
	layout.add_right(windCheckbox);

}
