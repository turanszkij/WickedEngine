/**
 * @file
 * Implementation of @ref wi::gui::GUI and the module's shared
 * @ref wi::gui::InternalState (render state + cross-widget flags).
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include <Utility/DirectXMath/DirectXMath.h>

#include <wiCanvas.h>
#include <wiColor.h>
#include <wiEnums.h>
#include <wiGraphics.h>
#include <wiGraphicsDevice.h>
#include <wiGUI/GUICommon.h>
#include <wiGUI/Widget.h>
#include <wiLocalization.h>
#include <wiResourceManager.h>

#include "wiBacklog.h"
#include "wiEventHandler.h"
#include "wiInput.h"
#include "wiPrimitive.h"
#include "wiProfiler.h"
#include "wiRenderer.h"
#include "wiTimer.h"

#include "wiGUI/GUI.h"
#include "wiGUI/GUIInternal.h"

using namespace wi::graphics;
using namespace wi::primitive;

namespace wi::gui
{
	InternalState::InternalState()
	{
		wi::Timer timer;

		static const wi::eventhandler::Handle handle =
			wi::eventhandler::Subscribe(
				wi::eventhandler::EVENT_RELOAD_SHADERS,
				[this](uint64_t userdata) { LoadShaders(); }
			);
		LoadShaders();

		wilog("wi::gui Initialized (%d ms)", (int)std::round(timer.elapsed()));
	}

	void InternalState::LoadShaders()
	{
		PipelineStateDesc desc;
		desc.vs  = wi::renderer::GetShader(           wi::enums::VSTYPE_VERTEXCOLOR);
		desc.ps  = wi::renderer::GetShader(           wi::enums::PSTYPE_VERTEXCOLOR);
		desc.il  = wi::renderer::GetInputLayout(      wi::enums::ILTYPE_VERTEXCOLOR);
		desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHDISABLED);
		desc.bs  = wi::renderer::GetBlendState(       wi::enums::BSTYPE_TRANSPARENT);
		desc.rs  = wi::renderer::GetRasterizerState(  wi::enums::RSTYPE_DOUBLESIDED);
		desc.pt  = PrimitiveTopology::TRIANGLESTRIP;

		wi::graphics::GetDevice()->CreatePipelineState(&desc, &PSO_colored);
	}

	InternalState& gui_internal()
	{
		static InternalState internal_state;

		return internal_state;
	}

	void GUI::Update(const wi::Canvas& canvas, const float dt)
	{
		if (!visible || wi::backlog::isActive())
		{
			return;
		}

		const auto range = wi::profiler::BeginRangeCPU("GUI Update");

		scroll_allowed = true;

		const XMFLOAT4 pointer = wi::input::GetPointer();
		const Hitbox2D pointerHitbox = Hitbox2D(
			XMFLOAT2(pointer.x, pointer.y),
			XMFLOAT2(1,         1)
		);

		uint32_t priority = 0;

		focus = false;
		bool force_disable = false;

		for (size_t i = 0; i < widgets.size(); ++i)
		{
			// Re-fetch by index every iteration: a widget's Update (or an
			// event callback it triggers) may AddWidget and reallocate the
			// vector. widgets holds non-owning pointers, so the Widget itself
			// stays valid, but a cached iterator (range-for) would dangle.
			Widget* widget = widgets[i];

			widget->force_disable = force_disable;
			widget->Update(canvas, dt);
			widget->force_disable = false;

			if (widget->priority_change)
			{
				force_disable           = true;
				widget->priority_change = false;
				widget->priority        = priority++;
			}
			else
			{
				widget->priority = ~0u;
			}

			const bool widget_visible = widget->IsVisible();
			const WIDGETSTATE state   = widget->GetState();

			if (widget_visible && widget->hitBox.intersects(pointerHitbox))
			{
				focus = true;
			}

			if (widget_visible && state > IDLE)
			{
				focus         = true;
				force_disable = true;
			}
		}

		if (priority > 0)
		{
			// Sort only if there are priority changes. Use std::stable_sort
			// (not std::sort) to preserve the relative order of UI elements
			// that have equal priority.
			std::stable_sort(
				widgets.begin(),
				widgets.end(),
				[](const Widget* a, const Widget* b) {
					return a->priority < b->priority;
				}
			);
		}

		wi::profiler::EndRange(range);
	}

	void GUI::Render(const wi::Canvas& canvas, const CommandList cmd) const
	{
		if (!visible || widgets.empty())
		{
			return;
		}

		const auto range_cpu = wi::profiler::BeginRangeCPU("GUI Render");
		const auto range_gpu = wi::profiler::BeginRangeGPU("GUI Render", cmd);

		wi::graphics::Rect scissorRect;
		scissorRect.bottom = static_cast<int32_t>(canvas.GetPhysicalHeight());
		scissorRect.left   = static_cast<int32_t>(0);
		scissorRect.right  = static_cast<int32_t>(canvas.GetPhysicalWidth());
		scissorRect.top    = static_cast<int32_t>(0);

		GraphicsDevice* const device = wi::graphics::GetDevice();

		device->EventBegin("GUI", cmd);

		// Rendering is back to front:
		for (size_t i = 0; i < widgets.size(); ++i)
		{
			const Widget* widget = widgets[widgets.size() - i - 1];

			device->BindScissorRects(1, &scissorRect, cmd);
			widget->Render(canvas, cmd);
		}

		device->BindScissorRects(1, &scissorRect, cmd);

		for (const auto& x : widgets)
		{
			x->RenderTooltip(canvas, cmd);
		}

		device->EventEnd(cmd);

		wi::profiler::EndRange(range_cpu);
		wi::profiler::EndRange(range_gpu);
	}

	void GUI::AddWidget(Widget* const widget)
	{
		if (widget != nullptr)
		{
			assert(std::find(
				widgets.begin(),
				widgets.end(),
				widget) == widgets.end()
			); // don't attach one widget twice!

			widgets.push_back(widget);
		}
	}

	void GUI::RemoveWidget(Widget* const widget)
	{
		for (auto& x : widgets)
		{
			if (x == widget)
			{
				x = widgets.back();
				widgets.pop_back();

				break;
			}
		}
	}

	Widget* GUI::GetWidget(const std::string& name) const
	{
		for (const auto& x : widgets)
		{
			if (x->GetName() == name)
			{
				return x;
			}
		}

		return nullptr;
	}

	bool GUI::HasFocus() const
	{
		if (!visible)
		{
			return false;
		}

		if (IsTyping())
		{
			// When typing is active but the user clicks outside all GUI
			// widgets, allow the click to pass through so it can deactivate
			// typing and reach the viewport.
			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT)
			 || wi::input::Press(wi::input::MOUSE_BUTTON_RIGHT)
			) {
				const XMFLOAT4 pointer = wi::input::GetPointer();
				const auto pointerHitbox = Hitbox2D(
					XMFLOAT2(pointer.x, pointer.y),
					XMFLOAT2(1, 1)
				);

				bool pointer_on_widget = false;
				for (const auto* widget : widgets)
				{
					if (widget->IsVisible()
					 && pointerHitbox.intersects(widget->hitBox))
					{
						pointer_on_widget = true;

						break;
					}
				}

				if (!pointer_on_widget)
				{
					return false;
				}
			}

			return true;
		}

		return focus;
	}

	bool GUI::IsTyping() const noexcept
	{
		if (!visible)
		{
			return false;
		}

		return typing_active;
	}

	void GUI::SetColor(const wi::Color color, const int id)
	{
		for (auto& widget : widgets)
		{
			widget->SetColor(color, id);
		}
	}

	void GUI::SetImage(const wi::Resource& resource, const int id)
	{
		for (auto& widget : widgets)
		{
			widget->SetImage(resource, id);
		}
	}

	void GUI::SetShadowColor(const wi::Color color)
	{
		for (auto& widget : widgets)
		{
			widget->SetShadowColor(color);
		}
	}

	void GUI::SetTheme(const Theme& theme, const int id)
	{
		for (auto& widget : widgets)
		{
			widget->SetTheme(theme, id);
		}
	}

	void GUI::ExportLocalization(wi::Localization& localization) const
	{
		wi::Localization& section = localization.GetSection("gui");

		for (const auto& widget : widgets)
		{
			widget->ExportLocalization(section);
		}
	}

	void GUI::ImportLocalization(const wi::Localization& localization)
	{
		const wi::Localization* section = localization.CheckSection("gui");

		if (section == nullptr) {
			return;
		}

		for (auto& widget : widgets)
		{
			widget->ImportLocalization(*section);
		}
	}
}
