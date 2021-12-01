#include "wiGUI.h"
#include "wiInput.h"
#include "wiPrimitive.h"
#include "wiProfiler.h"

using namespace wi::graphics;
using namespace wi::primitive;

namespace wi
{

	void GUI::Update(const wi::Canvas& canvas, float dt)
	{
		if (!visible)
		{
			return;
		}

		auto range = wi::profiler::BeginRangeCPU("GUI Update");

		XMFLOAT4 pointer = wi::input::GetPointer();
		Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointer.x, pointer.y), XMFLOAT2(1, 1));

		uint32_t priority = 0;

		focus = false;
		for (auto& widget : widgets)
		{
			widget->force_disable = focus;
			widget->Update(canvas, dt);
			widget->force_disable = false;

			if (widget->priority_change)
			{
				widget->priority_change = false;
				widget->priority = priority++;
			}
			else
			{
				widget->priority = ~0u;
			}

			if (widget->IsVisible() && widget->hitBox.intersects(pointerHitbox))
			{
				focus = true;
			}
			if (widget->GetState() > wi::widget::IDLE)
			{
				focus = true;
			}
		}

		//Sort only if there are priority changes
		if (priority > 0)
		{
			//Use std::stable_sort instead of std::sort to preserve UI element order with equal priorities
			std::stable_sort(widgets.begin(), widgets.end(), [](const wi::widget::Widget* a, const wi::widget::Widget* b) {
				return a->priority < b->priority;
				});
		}

		wi::profiler::EndRange(range);
	}

	void GUI::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!visible)
		{
			return;
		}

		auto range_cpu = wi::profiler::BeginRangeCPU("GUI Render");
		auto range_gpu = wi::profiler::BeginRangeGPU("GUI Render", cmd);

		Rect scissorRect;
		scissorRect.bottom = (int32_t)(canvas.GetPhysicalHeight());
		scissorRect.left = (int32_t)(0);
		scissorRect.right = (int32_t)(canvas.GetPhysicalWidth());
		scissorRect.top = (int32_t)(0);

		GraphicsDevice* device = wi::graphics::GetDevice();

		device->EventBegin("GUI", cmd);
		// Rendering is back to front:
		for (auto it = widgets.rbegin(); it != widgets.rend(); ++it)
		{
			const wi::widget::Widget* widget = (*it);
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

	void GUI::AddWidget(wi::widget::Widget* widget)
	{
		if (widget != nullptr)
		{
			assert(std::find(widgets.begin(), widgets.end(), widget) == widgets.end()); // don't attach one widget twice!
			widgets.push_back(widget);
		}
	}

	void GUI::RemoveWidget(wi::widget::Widget* widget)
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

	wi::widget::Widget* GUI::GetWidget(const std::string& name)
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

	bool GUI::HasFocus()
	{
		if (!visible)
		{
			return false;
		}

		return focus;
	}

}
