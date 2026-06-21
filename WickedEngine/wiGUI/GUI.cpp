#include "wiGUI/GUI.h"
#include "wiGUI/GUIInternal.h"

#include "wiInput.h"
#include "wiPrimitive.h"
#include "wiProfiler.h"
#include "wiRenderer.h"
#include "wiTimer.h"
#include "wiEventHandler.h"
#include "wiFont.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiBacklog.h"
#include "wiHelper.h"

#include <sstream>
#include <iomanip> // setprecision
#include <utility>

using namespace wi::graphics;
using namespace wi::primitive;

namespace wi::gui
{
	InternalState::InternalState()
	{
		wi::Timer timer;

		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [this](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		wilog("wi::gui Initialized (%d ms)", (int)std::round(timer.elapsed()));
	}
	void InternalState::LoadShaders()
	{
		PipelineStateDesc desc;
		desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
		desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
		desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
		desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHDISABLED);
		desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
		desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
		desc.pt = PrimitiveTopology::TRIANGLESTRIP;
		wi::graphics::GetDevice()->CreatePipelineState(&desc, &PSO_colored);
	}
	InternalState& gui_internal()
	{
		static InternalState internal_state;
		return internal_state;
	}

	void GUI::Update(const wi::Canvas& canvas, float dt)
	{
		if (!visible || wi::backlog::isActive())
		{
			return;
		}

		auto range = wi::profiler::BeginRangeCPU("GUI Update");

		scroll_allowed = true;

		XMFLOAT4 pointer = wi::input::GetPointer();
		Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointer.x, pointer.y), XMFLOAT2(1, 1));

		uint32_t priority = 0;

		focus = false;
		bool force_disable = false;
		for (size_t i = 0; i < widgets.size(); ++i)
		{
			Widget* widget = widgets[i]; // re index in loop, because widgets can be realloced while updating!
			widget->force_disable = force_disable;
			widget->Update(canvas, dt);
			widget->force_disable = false;

			if (widget->priority_change)
			{
				force_disable = true;
				widget->priority_change = false;
				widget->priority = priority++;
			}
			else
			{
				widget->priority = ~0u;
			}

			const bool visible = widget->IsVisible();
			const WIDGETSTATE state = widget->GetState();

			if (visible && widget->hitBox.intersects(pointerHitbox))
			{
				focus = true;
			}
			if (visible && state > IDLE)
			{
				focus = true;
				force_disable = true;
			}
		}

		if (priority > 0)
		{
			// Sort only if there are priority changes
			//	Use std::stable_sort instead of std::sort to preserve UI element order with equal priorities
			std::stable_sort(widgets.begin(), widgets.end(), [](const Widget* a, const Widget* b) {
				return a->priority < b->priority;
			});
		}

		wi::profiler::EndRange(range);
	}
	void GUI::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!visible || widgets.empty())
		{
			return;
		}

		auto range_cpu = wi::profiler::BeginRangeCPU("GUI Render");
		auto range_gpu = wi::profiler::BeginRangeGPU("GUI Render", cmd);

		wi::graphics::Rect scissorRect;
		scissorRect.bottom = (int32_t)(canvas.GetPhysicalHeight());
		scissorRect.left = (int32_t)(0);
		scissorRect.right = (int32_t)(canvas.GetPhysicalWidth());
		scissorRect.top = (int32_t)(0);

		GraphicsDevice* device = wi::graphics::GetDevice();

		device->EventBegin("GUI", cmd);
		// Rendering is back to front:
		for (size_t i = 0; i < widgets.size(); ++i)
		{
			const Widget* widget = widgets[widgets.size() - i - 1];
			device->BindScissorRects(1, &scissorRect, cmd);
			widget->Render(canvas, cmd);
		}

		device->BindScissorRects(1, &scissorRect, cmd);
		for (auto& x : widgets)
		{
			x->RenderTooltip(canvas, cmd);
		}

		device->EventEnd(cmd);

		wi::profiler::EndRange(range_cpu);
		wi::profiler::EndRange(range_gpu);
	}
	void GUI::AddWidget(Widget* widget)
	{
		if (widget != nullptr)
		{
			assert(std::find(widgets.begin(), widgets.end(), widget) == widgets.end()); // don't attach one widget twice!
			widgets.push_back(widget);
		}
	}
	void GUI::RemoveWidget(Widget* widget)
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
	Widget* GUI::GetWidget(const std::string& name)
	{
		for (auto& x : widgets)
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
			// When typing is active but the user clicks outside all GUI widgets,
			//	allow the click to pass through so it can deactivate typing and reach the viewport
			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT) || wi::input::Press(wi::input::MOUSE_BUTTON_RIGHT))
			{
				const XMFLOAT4 pointer = wi::input::GetPointer();
				const auto pointerHitbox = Hitbox2D(XMFLOAT2(pointer.x, pointer.y), XMFLOAT2(1, 1));
				bool pointer_on_widget = false;
				for (const auto* widget : widgets)
				{
					if (widget->IsVisible() && pointerHitbox.intersects(widget->hitBox))
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
	bool GUI::IsTyping() const
	{
		if (!visible)
		{
			return false;
		}

		return typing_active;
	}
	void GUI::SetColor(wi::Color color, int id)
	{
		for (auto& widget : widgets)
		{
			widget->SetColor(color, id);
		}
	}
	void GUI::SetImage(const wi::Resource& resource, int id)
	{
		for (auto& widget : widgets)
		{
			widget->SetImage(resource, id);
		}
	}
	void GUI::SetShadowColor(wi::Color color)
	{
		for (auto& widget : widgets)
		{
			widget->SetShadowColor(color);
		}
	}
	void GUI::SetTheme(const Theme& theme, int id)
	{
		for (auto& widget : widgets)
		{
			widget->SetTheme(theme, id);
		}
	}
	void GUI::ExportLocalization(wi::Localization& localization) const
	{
		wi::Localization& section = localization.GetSection("gui");
		for (auto& widget : widgets)
		{
			widget->ExportLocalization(section);
		}
	}
	void GUI::ImportLocalization(const wi::Localization& localization)
	{
		const wi::Localization* section = localization.CheckSection("gui");
		if (section == nullptr)
			return;
		for (auto& widget : widgets)
		{
			widget->ImportLocalization(*section);
		}
	}
}
