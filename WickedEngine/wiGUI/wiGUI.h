#pragma once

/**
 * @file
 * Umbrella header for the `wi::gui` module.
 *
 * Aggregates the framework pieces (@ref wi::gui::Widget, @ref wi::gui::GUI and
 * the shared types in `GUICommon.h`) together with every widget component, so
 * that a single include pulls in the whole module. Code that previously did
 * `#include "wiGUI.h"` should now include `"wiGUI/wiGUI.h"`.
 */

#include "wiGUI/GUICommon.h"
#include "wiGUI/Widget.h"
#include "wiGUI/GUI.h"

#include "wiGUI/components/Button.h"
#include "wiGUI/components/Image.h"
#include "wiGUI/components/ScrollBar.h"
#include "wiGUI/components/Label.h"
#include "wiGUI/components/TextInputField.h"
#include "wiGUI/components/Slider.h"
#include "wiGUI/components/CheckBox.h"
#include "wiGUI/components/ComboBox.h"
#include "wiGUI/components/Window.h"
#include "wiGUI/components/ColorPicker.h"
#include "wiGUI/components/TreeList.h"
