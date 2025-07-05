#include "stdafx.h"
#include "ExpressionWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ExpressionWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_EXPRESSION " Expression", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(670, 580));

	closeButton.SetTooltip("Delete ExpressionComponent");
	OnClose([&](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().expressions.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	infoLabel.Create("");
	infoLabel.SetText("Tip: If you also attach a Sound component to this entity, the mouth expression (if available) will be animated based on the sound playing.");
	infoLabel.SetFitTextEnabled(true);
	AddWidget(&infoLabel);

	auto handleExpr = [&] (auto func) {
		return [&] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			ExpressionComponent* expression_mastering = scene.expressions.GetComponent(entity);
			if (expression_mastering != nullptr) {
				func(expression_mastering, args);
			}
		};
	};

	talkCheckBox.Create("Force Talking: ");
	talkCheckBox.SetTooltip("Force continuous talking animation, even if no voice is playing");
	talkCheckBox.OnClick(handleExpr([&] (auto expression_mastering, auto args) {
		expression_mastering->SetForceTalkingEnabled(args.bValue);
	}));
	AddWidget(&talkCheckBox);

	blinkFrequencySlider.Create(0, 1, 0, 1000, "Blinks: ");
	blinkFrequencySlider.SetTooltip("Specifies the number of blinks per second.");
	blinkFrequencySlider.OnSlide(handleExpr([&] (auto expression_mastering, auto args) {
		expression_mastering->blink_frequency = args.fValue;
	}));
	AddWidget(&blinkFrequencySlider);

	blinkLengthSlider.Create(0, 1, 0, 1000, "Blink Length: ");
	blinkLengthSlider.OnSlide(handleExpr([&] (auto expression_mastering, auto args) {
		expression_mastering->blink_length = args.fValue;
	}));
	AddWidget(&blinkLengthSlider);

	blinkCountSlider.Create(1, 4, 2, 3, "Blink Count: ");
	blinkCountSlider.OnSlide(handleExpr([&] (auto expression_mastering, auto args) {
		expression_mastering->blink_count = args.iValue;
	}));
	AddWidget(&blinkCountSlider);

	lookFrequencySlider.Create(0, 1, 0, 1000, "Looks: ");
	lookFrequencySlider.SetTooltip("Specifies the number of look-aways per second.");
	lookFrequencySlider.OnSlide(handleExpr([&] (auto expression_mastering, auto args) {
		expression_mastering->look_frequency = args.fValue;
	}));
	AddWidget(&lookFrequencySlider);

	lookLengthSlider.Create(0, 1, 0, 1000, "Look Length: ");
	lookLengthSlider.OnSlide(handleExpr([&] (auto expression_mastering, auto args) {
		expression_mastering->look_length = args.fValue;
	}));
	AddWidget(&lookLengthSlider);

	expressionList.Create("Expressions: ");
	expressionList.OnSelect(handleExpr([&] (auto expression_mastering, auto args) {
		if (args.iValue >= expression_mastering->expressions.size())
			return;

		const ExpressionComponent::Expression& expression = expression_mastering->expressions[args.iValue];
		binaryCheckBox.SetCheck(expression.IsBinary());
		weightSlider.SetValue(expression.weight);
		overrideMouthCombo.SetSelectedByUserdataWithoutCallback((uint64_t)expression.override_mouth);
		overrideBlinkCombo.SetSelectedByUserdataWithoutCallback((uint64_t)expression.override_blink);
		overrideLookCombo.SetSelectedByUserdataWithoutCallback((uint64_t)expression.override_look);
	}));
	AddWidget(&expressionList);

	auto handleExprList = [&] (auto func, const bool checkExprSize = true) {
		return handleExpr([&] (auto expression_mastering, auto args) {
			if (checkExprSize && args.iValue >= expression_mastering->expressions.size())
				return;

			int count = expressionList.GetItemCount();
			for (int i = 0; i < count; ++i)
			{
				if (!expressionList.GetItem(i).selected)
					continue;
				if (i >= expression_mastering->expressions.size())
					return;

				ExpressionComponent::Expression& expression = expression_mastering->expressions[i];
				func(expression, args);
				expression.SetDirty();
			}
		});
	};

	binaryCheckBox.Create("Binary: ");
	binaryCheckBox.OnClick(handleExprList([&] (auto expression, auto args) {
		expression.SetBinary(args.bValue);
	}, false));
	AddWidget(&binaryCheckBox);

	weightSlider.Create(0, 1, 0, 100000, "Weight: ");
	weightSlider.OnSlide(handleExprList([&] (auto expression, auto args) {
		expression.weight = args.fValue;
	}));
	AddWidget(&weightSlider);

	overrideMouthCombo.Create("Override Mouth: ");
	overrideMouthCombo.SetTooltip("Lip sync override control");
	overrideMouthCombo.AddItem("None", (uint64_t)ExpressionComponent::Override::None);
	overrideMouthCombo.AddItem("Block", (uint64_t)ExpressionComponent::Override::Block);
	overrideMouthCombo.AddItem("Blend", (uint64_t)ExpressionComponent::Override::Blend);
	overrideMouthCombo.OnSelect(handleExprList([&] (auto expression, auto args) {
		expression.override_mouth = (ExpressionComponent::Override)args.userdata;
	}));
	AddWidget(&overrideMouthCombo);

	overrideBlinkCombo.Create("Override Blink: ");
	overrideBlinkCombo.SetTooltip("Blink override control");
	overrideBlinkCombo.AddItem("None", (uint64_t)ExpressionComponent::Override::None);
	overrideBlinkCombo.AddItem("Block", (uint64_t)ExpressionComponent::Override::Block);
	overrideBlinkCombo.AddItem("Blend", (uint64_t)ExpressionComponent::Override::Blend);
	overrideBlinkCombo.OnSelect(handleExprList([&] (auto expression, auto args) {
		expression.override_blink = (ExpressionComponent::Override)args.userdata;
	}));
	AddWidget(&overrideBlinkCombo);

	overrideLookCombo.Create("Override Look: ");
	overrideLookCombo.SetTooltip("Look-away override control");
	overrideLookCombo.AddItem("None", (uint64_t)ExpressionComponent::Override::None);
	overrideLookCombo.AddItem("Block", (uint64_t)ExpressionComponent::Override::Block);
	overrideLookCombo.AddItem("Blend", (uint64_t)ExpressionComponent::Override::Blend);
	overrideLookCombo.OnSelect(handleExprList([&] (auto expression, auto args) {
		expression.override_look = (ExpressionComponent::Override)args.userdata;
	}));
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
	layout.margin_left = 110;

	layout.add_fullwidth(infoLabel);
	layout.add_right(talkCheckBox);
	layout.add(blinkFrequencySlider);
	layout.add(blinkLengthSlider);
	layout.add(blinkCountSlider);
	layout.add(lookFrequencySlider);
	layout.add(lookLengthSlider);
	layout.add_fullwidth(expressionList);
	layout.add_right(binaryCheckBox);
	layout.add(weightSlider);
	layout.add(overrideMouthCombo);
	layout.add(overrideBlinkCombo);
	layout.add(overrideLookCombo);

}
