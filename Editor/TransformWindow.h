#pragma once
#include "WickedEngine.h"

class EditorComponent;

class TransformWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::Button createButton;

	wi::widget::ComboBox parentCombo;

	wi::widget::TextInputField txInput;
	wi::widget::TextInputField tyInput;
	wi::widget::TextInputField tzInput;
					 
	wi::widget::TextInputField rxInput;
	wi::widget::TextInputField ryInput;
	wi::widget::TextInputField rzInput;
	wi::widget::TextInputField rwInput;
					 
	wi::widget::TextInputField sxInput;
	wi::widget::TextInputField syInput;
	wi::widget::TextInputField szInput;
};

