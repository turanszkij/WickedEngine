#pragma once
#include "WickedEngine.h"

class EditorComponent;

class TransformWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiButton createButton;

	wiComboBox parentCombo;

	wiTextInputField txInput;
	wiTextInputField tyInput;
	wiTextInputField tzInput;
					 
	wiTextInputField rxInput;
	wiTextInputField ryInput;
	wiTextInputField rzInput;
	wiTextInputField rwInput;
					 
	wiTextInputField sxInput;
	wiTextInputField syInput;
	wiTextInputField szInput;
};

