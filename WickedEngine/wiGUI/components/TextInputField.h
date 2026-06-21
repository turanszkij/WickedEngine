#pragma once
#include "wiGUI/Widget.h"

namespace wi::gui
{
	// Text input box
	class TextInputField : public Widget
	{
	protected:
		std::function<void(const EventArgs& args)> onInputAccepted;
		std::function<void(const EventArgs& args)> onInput;
		static wi::SpriteFont font_input;
		bool cancel_input_enabled = true;
		int float_precision = -1;

	public:
		void Create(const std::string& name);

		void SetValue(const std::string& newValue);
		void SetValue(int newValue);
		void SetValue(float newValue);
		const std::string GetValue();
		const std::string GetCurrentInputValue();

		// Set whether incomplete input will be removed on lost activation state (default: true)
		void SetCancelInputEnabled(bool value) { cancel_input_enabled = value; }
		bool IsCancelInputEnabled() const { return cancel_input_enabled; }

		// There can only be ONE active text input field, so these methods modify the active one
		static void AddInput(const wchar_t inputChar);
		static void AddInput(const char inputChar);
		static void DeleteFromInput(int direction = -1);

		// Sets this text input as currently active in typing state
		//	selectall params: selects the current input, so typing will immediately overwrite it
		void SetAsActive(bool selectall = false);

		void Activate() override;
		void Deactivate() override;

		void SetEnabled(bool val) override;
		void SetVisible(bool val) override;
		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "TextInputField"; }

		// Called when input was accepted with ENTER key:
		void OnInputAccepted(std::function<void(const EventArgs& args)> func);
		// Called when input was updated with new character:
		void OnInput(std::function<void(const EventArgs& args)> func);

		wi::SpriteFont font_description;
		void SetDescription(const std::string& desc) { font_description.SetText(desc); }
		const std::string GetDescription() const { return font_description.GetTextA(); }

		// Set number of fractional float digits (-1 = default)
		constexpr void SetFloatPrecision(int value) { float_precision = value; }
	};
}
