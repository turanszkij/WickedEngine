#pragma once
/**
 * @file
 * The `wi::gui::GUI` widget manager.
 *
 * @ref wi::gui::GUI owns a flat list of top-level @ref wi::gui::Widget
 * pointers and drives them: it forwards the per-frame Update / Render
 * lifecycle, manages focus and draw ordering (priority), and fans out styling
 * and localization calls to every widget.
 */

#include "wiGUI/Widget.h"

namespace wi::gui
{
	/**
	 * Manages a collection of top-level widgets.
	 *
	 * Widgets are stored as non-owning pointers; the caller keeps ownership and
	 * must keep each widget alive while it is registered. Each frame
	 * @ref Update sorts widgets by interaction priority and tracks focus, and
	 * @ref Render draws them back-to-front followed by their tooltips. Styling
	 * (@ref SetColor / @ref SetImage / @ref SetTheme) and localization are
	 * applied to all managed widgets.
	 */
	class GUI
	{
	private:
		/** Top-level widgets, in draw/update order (non-owning pointers). */
		wi::vector<Widget*> widgets;

		/** Whether any widget currently holds focus (recomputed in Update). */
		bool focus = false;

		/** Whether the GUI is updated and drawn. */
		bool visible = true;
	public:

		/**
		 * Updates all widgets, then sorts them by interaction priority.
		 *
		 * No-op while the GUI is hidden or the backlog is active.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt);

		/**
		 * Draws all widgets back-to-front, then their tooltips on top.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const;

		/**
		 * Registers a top-level widget.
		 *
		 * @param[in] widget - Widget to add; ignored if null. The same widget
		 *                     must not be added twice (asserted in debug).
		 */
		void AddWidget(Widget* widget);

		/**
		 * Unregisters a previously added widget.
		 *
		 * @param[in] widget - Widget to remove; no-op if not present.
		 */
		void RemoveWidget(Widget* widget);

		/**
		 * Finds a managed widget by name.
		 *
		 * @param[in] name - Widget name to search for.
		 *
		 * @return The first matching widget, or nullptr if none matches.
		 */
		Widget* GetWidget(const std::string& name);

		/**
		 * Returns whether any GUI element currently has focus.
		 *
		 * While typing is active this also returns true, except when a click
		 * lands outside every widget (so the click can pass through to
		 * deactivate typing and reach the viewport).
		 */
		bool HasFocus() const;

		/**
		 * Returns whether text input is currently happening.
		 */
		bool IsTyping() const;

		/**
		 * Shows or hides the whole GUI.
		 *
		 * @param[in] value - true to update and draw the GUI, false to skip it.
		 */
		void SetVisible(bool value) { visible = value; }

		/**
		 * Returns whether the GUI is visible.
		 */
		bool IsVisible() const { return visible; }

		/**
		 * Sets a sprite color on every managed widget.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetColor(wi::Color color, int id = -1);

		/**
		 * Sets a sprite texture on every managed widget.
		 *
		 * @param[in] resource - Texture resource to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetImage(const wi::Resource& resource, int id = -1);

		/**
		 * Sets the shadow color on every managed widget.
		 *
		 * @param[in] color - New shadow color.
		 */
		void SetShadowColor(wi::Color color);

		/**
		 * Applies a theme to every managed widget.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetTheme(const Theme& theme, int id = -1);

		/**
		 * Exports localizable strings from all widgets into the "gui" section.
		 *
		 * @param[in,out] localization - Localization to add entries to.
		 */
		void ExportLocalization(wi::Localization& localization) const;

		/**
		 * Imports localized strings from the "gui" section into all widgets.
		 *
		 * @param[in] localization - Localization to read entries from.
		 */
		void ImportLocalization(const wi::Localization& localization);
	};
}
