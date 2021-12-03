#pragma once
#include "WickedEngine.h"

class EditorComponent;

class TransformWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button createButton;

	wi::gui::ComboBox parentCombo;

	wi::gui::TextInputField txInput;
	wi::gui::TextInputField tyInput;
	wi::gui::TextInputField tzInput;
					 
	wi::gui::TextInputField rxInput;
	wi::gui::TextInputField ryInput;
	wi::gui::TextInputField rzInput;
	wi::gui::TextInputField rwInput;
					 
	wi::gui::TextInputField sxInput;
	wi::gui::TextInputField syInput;
	wi::gui::TextInputField szInput;
};

