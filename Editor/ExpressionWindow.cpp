#include "stdafx.h"
#include "ExpressionWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;

void ExpressionWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_EXPRESSION " Expression", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 80));

	closeButton.SetTooltip("Delete ExpressionComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().colliders.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 60;
	float y = 4;
	float hei = 20;
	float step = hei + 2;
	float wid = 220;

	binaryCheckBox.Create("Binary: ");
	binaryCheckBox.SetSize(XMFLOAT2(hei, hei));
	binaryCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression = scene.expressions.GetComponent(entity);
		if (expression == nullptr)
			return;

		expression->SetBinary(args.bValue);
		expression->SetDirty();
	});
	AddWidget(&binaryCheckBox);

	weightSlider.Create(0, 1, 0, 100000, "Weight: ");
	weightSlider.SetSize(XMFLOAT2(wid, hei));
	weightSlider.SetPos(XMFLOAT2(x, y += step));
	weightSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression = scene.expressions.GetComponent(entity);
		if (expression == nullptr)
			return;

		expression->weight = args.fValue;
		expression->SetDirty();
		});
	AddWidget(&weightSlider);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void ExpressionWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();

	const ExpressionComponent* expression = scene.expressions.GetComponent(entity);

	if (expression != nullptr)
	{
		binaryCheckBox.SetCheck(expression->IsBinary());
		weightSlider.SetValue(expression->weight);
	}
}

void ExpressionWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 60;
	const float margin_right = 50;

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

	add_right(binaryCheckBox);
	add(weightSlider);

}
