#pragma once

/**
 * @file
 * The `wi::gui::Image` widget.
 *
 * A non-interactive widget that displays its IDLE-state sprite (optionally with
 * a shadow), used to place a static image in the GUI.
 */

#include "wiGUI/Widget.h"

namespace wi::gui
{
	/**
	 * Non-interactive image-display widget.
	 *
	 * Draws its IDLE-state background sprite (see @ref Widget::sprites), with
	 * an optional drop shadow. Interaction is disabled on creation, so it does
	 * not participate in focus or input.
	 */
	class Image : public Widget
	{
	public:
		/**
		 * Initializes the image widget with a default 100x100 size.
		 *
		 * The widget is created disabled (non-interactive).
		 *
		 * @param[in] name - Widget name / identifier.
		 */
		void Create(const std::string& name);

		/**
		 * Draws the image's sprite (and its shadow, if enabled).
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Returns the widget's type name ("Image").
		 */
		const char* GetWidgetTypeName() const override { return "Image"; }
	};
}
