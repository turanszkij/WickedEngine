#include "stdafx.h"
#include "ExpressionWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ExpressionWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_EXPRESSION " Expression", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 580));

	closeButton.SetTooltip("Delete ExpressionComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().expressions.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
		});

	float x = 60;
	float y = 4;
	float hei = 20;
	float step = hei + 2;
	float wid = 220;

	infoLabel.Create("");
	infoLabel.SetSize(XMFLOAT2(100, 60));
	infoLabel.SetText("Tip: If you also attach a Sound component to this entity, the mouth expression (if available) will be animated based on the sound playing.");
	AddWidget(&infoLabel);

	talkCheckBox.Create("Force Talking: ");
	talkCheckBox.SetTooltip("Force continuous talking animation, even if no voice is playing");
	talkCheckBox.SetSize(XMFLOAT2(hei, hei));
	talkCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;

		expression_mastering->SetForceTalkingEnabled(args.bValue);
	});
	AddWidget(&talkCheckBox);

	blinkFrequencySlider.Create(0, 1, 0, 1000, "Blinks: ");
	blinkFrequencySlider.SetTooltip("Specifies the number of blinks per second.");
	blinkFrequencySlider.SetSize(XMFLOAT2(wid, hei));
	blinkFrequencySlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		expression_mastering->blink_frequency = args.fValue;
		});
	AddWidget(&blinkFrequencySlider);

	blinkLengthSlider.Create(0, 1, 0, 1000, "Blink Length: ");
	blinkLengthSlider.SetSize(XMFLOAT2(wid, hei));
	blinkLengthSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		expression_mastering->blink_length = args.fValue;
		});
	AddWidget(&blinkLengthSlider);

	blinkCountSlider.Create(1, 4, 2, 3, "Blink Count: ");
	blinkCountSlider.SetSize(XMFLOAT2(wid, hei));
	blinkCountSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		expression_mastering->blink_count = args.iValue;
		});
	AddWidget(&blinkCountSlider);

	lookFrequencySlider.Create(0, 1, 0, 1000, "Looks: ");
	lookFrequencySlider.SetTooltip("Specifies the number of look-aways per second.");
	lookFrequencySlider.SetSize(XMFLOAT2(wid, hei));
	lookFrequencySlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		expression_mastering->look_frequency = args.fValue;
		});
	AddWidget(&lookFrequencySlider);

	lookLengthSlider.Create(0, 1, 0, 1000, "Look Length: ");
	lookLengthSlider.SetSize(XMFLOAT2(wid, hei));
	lookLengthSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		expression_mastering->look_length = args.fValue;
		});
	AddWidget(&lookLengthSlider);

	expressionList.Create("Expressions: ");
	expressionList.SetSize(XMFLOAT2(wid, 200));
	expressionList.SetPos(XMFLOAT2(4, y += step));
	expressionList.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		if (args.iValue >= expression_mastering->expressions.size())
			return;

		const ExpressionComponent::Expression& expression = expression_mastering->expressions[args.iValue];
		binaryCheckBox.SetCheck(expression.IsBinary());
		weightSlider.SetValue(expression.weight);
		overrideMouthCombo.SetSelectedByUserdataWithoutCallback((uint64_t)expression.override_mouth);
		overrideBlinkCombo.SetSelectedByUserdataWithoutCallback((uint64_t)expression.override_blink);
		overrideLookCombo.SetSelectedByUserdataWithoutCallback((uint64_t)expression.override_look);
		});
	AddWidget(&expressionList);

	binaryCheckBox.Create("Binary: ");
	binaryCheckBox.SetSize(XMFLOAT2(hei, hei));
	binaryCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;

		int count = expressionList.GetItemCount();
		for (int i = 0; i < count; ++i)
		{
			if (!expressionList.GetItem(i).selected)
				continue;
			if (i >= expression_mastering->expressions.size())
				return;

			ExpressionComponent::Expression& expression = expression_mastering->expressions[i];
			expression.SetBinary(args.bValue);
			expression.SetDirty();
		}
	});
	AddWidget(&binaryCheckBox);

	weightSlider.Create(0, 1, 0, 100000, "Weight: ");
	weightSlider.SetSize(XMFLOAT2(wid, hei));
	weightSlider.SetPos(XMFLOAT2(x, y += step));
	weightSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		if (args.iValue >= expression_mastering->expressions.size())
			return;

		int count = expressionList.GetItemCount();
		for (int i = 0; i < count; ++i)
		{
			if (!expressionList.GetItem(i).selected)
				continue;
			if (i >= expression_mastering->expressions.size())
				return;

			ExpressionComponent::Expression& expression = expression_mastering->expressions[i];
			expression.weight = args.fValue;
			expression.SetDirty();
		}
	});
	AddWidget(&weightSlider);

	overrideMouthCombo.Create("Override Mouth: ");
	overrideMouthCombo.SetTooltip("Lip sync override control");
	overrideMouthCombo.SetSize(XMFLOAT2(wid, hei));
	overrideMouthCombo.AddItem("None", (uint64_t)ExpressionComponent::Override::None);
	overrideMouthCombo.AddItem("Block", (uint64_t)ExpressionComponent::Override::Block);
	overrideMouthCombo.AddItem("Blend", (uint64_t)ExpressionComponent::Override::Blend);
	overrideMouthCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		if (args.iValue >= expression_mastering->expressions.size())
			return;

		int count = expressionList.GetItemCount();
		for (int i = 0; i < count; ++i)
		{
			if (!expressionList.GetItem(i).selected)
				continue;
			if (i >= expression_mastering->expressions.size())
				return;

			ExpressionComponent::Expression& expression = expression_mastering->expressions[i];
			expression.override_mouth = (ExpressionComponent::Override)args.userdata;
			expression.SetDirty();
		}
	});
	AddWidget(&overrideMouthCombo);

	overrideBlinkCombo.Create("Override Blink: ");
	overrideBlinkCombo.SetTooltip("Blink override control");
	overrideBlinkCombo.SetSize(XMFLOAT2(wid, hei));
	overrideBlinkCombo.AddItem("None", (uint64_t)ExpressionComponent::Override::None);
	overrideBlinkCombo.AddItem("Block", (uint64_t)ExpressionComponent::Override::Block);
	overrideBlinkCombo.AddItem("Blend", (uint64_t)ExpressionComponent::Override::Blend);
	overrideBlinkCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		if (args.iValue >= expression_mastering->expressions.size())
			return;

		int count = expressionList.GetItemCount();
		for (int i = 0; i < count; ++i)
		{
			if (!expressionList.GetItem(i).selected)
				continue;
			if (i >= expression_mastering->expressions.size())
				return;

			ExpressionComponent::Expression& expression = expression_mastering->expressions[i];
			expression.override_blink = (ExpressionComponent::Override)args.userdata;
			expression.SetDirty();
		}
		});
	AddWidget(&overrideBlinkCombo);

	overrideLookCombo.Create("Override Look: ");
	overrideLookCombo.SetTooltip("Look-away override control");
	overrideLookCombo.SetSize(XMFLOAT2(wid, hei));
	overrideLookCombo.AddItem("None", (uint64_t)ExpressionComponent::Override::None);
	overrideLookCombo.AddItem("Block", (uint64_t)ExpressionComponent::Override::Block);
	overrideLookCombo.AddItem("Blend", (uint64_t)ExpressionComponent::Override::Blend);
	overrideLookCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
		if (expression_mastering == nullptr)
			return;
		if (args.iValue >= expression_mastering->expressions.size())
			return;

		int count = expressionList.GetItemCount();
		for (int i = 0; i < count; ++i)
		{
			if (!expressionList.GetItem(i).selected)
				continue;
			if (i >= expression_mastering->expressions.size())
				return;

			ExpressionComponent::Expression& expression = expression_mastering->expressions[i];
			expression.override_look = (ExpressionComponent::Override)args.userdata;
			expression.SetDirty();
		}
		});
	AddWidget(&overrideLookCombo);


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

	const ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);

	if (expression_mastering != nullptr)
	{
		blinkFrequencySlider.SetValue(expression_mastering->blink_frequency);
		blinkLengthSlider.SetValue(expression_mastering->blink_length);
		blinkCountSlider.SetValue(expression_mastering->blink_count);
		lookFrequencySlider.SetValue(expression_mastering->look_frequency);
		lookLengthSlider.SetValue(expression_mastering->look_length);
		talkCheckBox.SetCheck(expression_mastering->IsForceTalkingEnabled());

		expressionList.ClearItems();
		for (const ExpressionComponent::Expression& expression : expression_mastering->expressions)
		{
			expressionList.AddItem(expression.name);
		}
	}
}

void ExpressionWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 110;
	const float margin_right = 45;

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

	add_fullwidth(infoLabel);
	add_right(talkCheckBox);
	add(blinkFrequencySlider);
	add(blinkLengthSlider);
	add(blinkCountSlider);
	add(lookFrequencySlider);
	add(lookLengthSlider);
	add_fullwidth(expressionList);
	add_right(binaryCheckBox);
	add(weightSlider);
	add(overrideMouthCombo);
	add(overrideBlinkCombo);
	add(overrideLookCombo);

}
