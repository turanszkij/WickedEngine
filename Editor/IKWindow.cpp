#include "stdafx.h"
#include "IKWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void IKWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_IK " Inverse Kinematics", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(400, 120));

	closeButton.SetTooltip("Delete InverseKinematicsComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().inverse_kinematics.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
		});

	float x = 120;
	float y = 0;
	float siz = 140;
	float hei = 18;
	float step = hei + 2;

	auto forEachSelected = [this] (auto func) {
		return [this, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				InverseKinematicsComponent* ik = scene.inverse_kinematics.GetComponent(x.entity);
				if (ik != nullptr)
				{
					func(ik, args);
				}
			}
		};
	};

	targetCombo.Create("Target: ");
	targetCombo.SetSize(XMFLOAT2(siz, hei));
	targetCombo.SetPos(XMFLOAT2(x, y));
	targetCombo.SetEnabled(false);
	targetCombo.OnSelect(forEachSelected([this] (auto ik, auto args) {
		if (args.iValue == 0)
		{
			ik->target = INVALID_ENTITY;
		}
		else
		{
			ik->target = editor->GetCurrentScene().transforms.GetEntity(args.iValue - 1);
		}
	}));
	targetCombo.SetTooltip("Choose a target entity (with transform) that the IK will follow");
	AddWidget(&targetCombo);

	disabledCheckBox.Create("Disabled: ");
	disabledCheckBox.SetTooltip("Disable simulation.");
	disabledCheckBox.SetPos(XMFLOAT2(x, y += step));
	disabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	disabledCheckBox.OnClick(forEachSelected([] (auto ik, auto args) {
		ik->SetDisabled(args.bValue);
	}));
	AddWidget(&disabledCheckBox);

	chainLengthSlider.Create(0, 10, 0, 10, "Chain Length: ");
	chainLengthSlider.SetTooltip("How far the hierarchy chain is simulated backwards from this entity");
	chainLengthSlider.SetPos(XMFLOAT2(x, y += step));
	chainLengthSlider.SetSize(XMFLOAT2(siz, hei));
	chainLengthSlider.OnSlide(forEachSelected([] (auto ik, auto args) {
		ik->chain_length = args.iValue;
	}));
	AddWidget(&chainLengthSlider);

	iterationCountSlider.Create(0, 10, 1, 10, "Iteration Count: ");
	iterationCountSlider.SetTooltip("How many iterations to compute the inverse kinematics for. Higher values are slower but more accurate.");
	iterationCountSlider.SetPos(XMFLOAT2(x, y += step));
	iterationCountSlider.SetSize(XMFLOAT2(siz, hei));
	iterationCountSlider.OnSlide(forEachSelected([] (auto ik, auto args) {
		ik->iteration_count = args.iValue;
	}));
	AddWidget(&iterationCountSlider);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void IKWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const InverseKinematicsComponent* ik = scene.inverse_kinematics.GetComponent(entity);

	if (ik != nullptr)
	{
		SetEnabled(true);

		disabledCheckBox.SetCheck(ik->IsDisabled());
		chainLengthSlider.SetValue((float)ik->chain_length);
		iterationCountSlider.SetValue((float)ik->iteration_count);

		targetCombo.ClearItems();
		targetCombo.AddItem("NO TARGET");
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			Entity entity = scene.transforms.GetEntity(i);
			const NameComponent* name = scene.names.GetComponent(entity);
			targetCombo.AddItem(name == nullptr ? std::to_string(entity) : name->name);

			if (ik->target == entity)
			{
				targetCombo.SetSelected((int)i + 1);
			}
		}
	}
	else
	{
		SetEnabled(false);
	}

}

void IKWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 110;

	layout.add(targetCombo);
	layout.add_right(disabledCheckBox);
	layout.add(chainLengthSlider);
	layout.add(iterationCountSlider);

}
