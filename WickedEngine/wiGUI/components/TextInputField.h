#pragma once

/**
 * @file
 * The `wi::gui::TextInputField` widget.
 *
 * An editable single-line text box with a caret, text selection, clipboard
 * support (copy / cut / paste), and optional arithmetic-expression evaluation
 * of the entered text when accepted with ENTER.
 */

#include "wiGUI/Widget.h"

namespace wi::gui
{
	/**
	 * Editable single-line text input box.
	 *
	 * Supports caret movement, text selection, clipboard operations and, on
	 * ENTER, evaluation of simple arithmetic expressions (e.g. `10 * 2` becomes
	 * `20`). Accepting input raises @ref OnInputAccepted; each keystroke raises
	 * @ref OnInput. Because there can be only **one** active text input field
	 * at a time, the live editing buffer and caret are shared static state, and
	 * the input-mutating helpers (@ref AddInput, @ref DeleteFromInput) act on
	 * the currently active field.
	 *
	 * @note The widget's text is excluded from localization (it can hold user
	 *       input that must not be overwritten); only its tooltip is localized.
	 */
	class TextInputField : public Widget
	{
	protected:
		/** Callback invoked when input is accepted (ENTER). */
		std::function<void(const EventArgs& args)> onInputAccepted;

		/** Callback invoked on every input change (per keystroke). */
		std::function<void(const EventArgs& args)> onInput;

		/** Shared live editing buffer of the single active input field. */
		static wi::SpriteFont font_input;

		/** Whether incomplete input is discarded when activation is lost. */
		bool cancel_input_enabled = true;

		/** Fractional digits used when formatting floats (-1 = default). */
		int float_precision = -1;

	public:
		/**
		 * Initializes the input field with default size, text and callback.
		 *
		 * @param[in] name - Widget name; also used as the initial text.
		 */
		void Create(const std::string& name);

		/**
		 * Sets the field's text to a string value.
		 *
		 * @param[in] newValue - New text value.
		 */
		void SetValue(const std::string& newValue);

		/**
		 * Sets the field's text from an integer value.
		 *
		 * @param[in] newValue - New value.
		 */
		void SetValue(int newValue);

		/**
		 * Sets the field's text from a float value.
		 *
		 * `FLT_MAX` / `-FLT_MAX` are shown as the literal text "FLT_MAX" /
		 * "-FLT_MAX"; otherwise @ref float_precision controls formatting.
		 *
		 * @param[in] newValue - New value.
		 */
		void SetValue(float newValue);

		/**
		 * Returns the field's committed text as an ASCII string.
		 */
		[[nodiscard]] std::string GetValue() const;

		/**
		 * Returns the text currently being edited, or the committed value.
		 *
		 * @return The live edit buffer while active, otherwise @ref GetValue.
		 */
		[[nodiscard]] std::string GetCurrentInputValue() const;

		/**
		 * Sets whether incomplete input is removed on losing active state.
		 *
		 * @param[in] value - true (default) discards uncommitted input.
		 */
		void SetCancelInputEnabled(bool value) noexcept {
			cancel_input_enabled = value;
		}

		/**
		 * Returns whether incomplete input is discarded on losing focus.
		 */
		[[nodiscard]] bool IsCancelInputEnabled() const noexcept {
			return cancel_input_enabled;
		}

		/**
		 * Inserts a wide character into the active field's input.
		 *
		 * Acts on the single currently active text input field. Handles
		 * selection replacement and clipboard shortcuts (copy/cut/paste).
		 *
		 * @param[in] inputChar - Character to insert.
		 */
		static void AddInput(const wchar_t inputChar);

		/**
		 * Inserts a narrow character into the active field's input.
		 *
		 * @param[in] inputChar - Character to insert.
		 */
		static void AddInput(const char inputChar);

		/**
		 * Deletes a character (or the selection) from the active field.
		 *
		 * @param[in] direction - Negative deletes before the caret
		 *                        (backspace), positive deletes after (delete).
		 */
		static void DeleteFromInput(int direction = -1);

		/**
		 * Makes this field the active one and enters typing state.
		 *
		 * @param[in] selectall - If true, selects the current input so typing
		 *                        immediately overwrites it.
		 */
		void SetAsActive(bool selectall = false);

		/**
		 * Activates the field, beginning typing.
		 */
		void Activate() override;

		/**
		 * Deactivates the field, ending typing.
		 */
		void Deactivate() override;

		/**
		 * Enables or disables the field; clears typing state if disabled.
		 *
		 * @param[in] val - true to enable.
		 */
		void SetEnabled(bool val) override;

		/**
		 * Shows or hides the field; clears typing state if hidden.
		 *
		 * @param[in] val - true to show.
		 */
		void SetVisible(bool val) override;

		/**
		 * Handles input, caret movement, selection and acceptance.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the box, text, caret, selection highlight and shadow.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Sets the color of the field's per-state sprites.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID; the
		 *                 `WIDGET_ID_TEXTINPUTFIELD_*` IDs address its states.
		 *                 -1 applies to all.
		 */
		void SetColor(wi::Color color, int id = -1) override;

		/**
		 * Applies a theme to the field and its description label.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetTheme(const Theme& theme, int id = -1) override;

		/**
		 * Returns the widget's type name ("TextInputField").
		 */
		[[nodiscard]] const char* GetWidgetTypeName() const override {
			return "TextInputField";
		}

		/**
		 * Sets the callback invoked when input is accepted (ENTER).
		 *
		 * @param[in] func - Callback to invoke.
		 */
		void OnInputAccepted(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the callback invoked on every input change.
		 *
		 * @param[in] func - Callback to invoke.
		 */
		void OnInput(std::function<void(const EventArgs& args)> func);

		/** Optional description label drawn beside the field. */
		wi::SpriteFont font_description;

		/**
		 * Sets the description label text.
		 *
		 * @param[in] desc - Description text.
		 */
		void SetDescription(const std::string& desc) {
			font_description.SetText(desc);
		}

		/**
		 * Returns the description label text as an ASCII string.
		 */
		[[nodiscard]] std::string GetDescription() const {
			return font_description.GetTextA();
		}

		/**
		 * Sets the number of fractional digits used to format floats.
		 *
		 * @param[in] value - Digit count, or -1 for the default formatting.
		 */
		constexpr void SetFloatPrecision(int value) noexcept {
			float_precision = value;
		}
	};
}
