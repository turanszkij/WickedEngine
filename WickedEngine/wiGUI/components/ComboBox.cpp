#include "wiGUI/components/ComboBox.h"
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
	static constexpr float combo_height() { return 20; };
	void ComboBox::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnSelect([](const EventArgs& args) {});
		SetSize(XMFLOAT2(100, 20));

		font.params.h_align = wi::font::WIFALIGN_RIGHT;
		font.params.v_align = wi::font::WIFALIGN_CENTER;

		selected_font = font;
		selected_font.params.h_align = wi::font::WIFALIGN_CENTER;
		selected_font.params.v_align = wi::font::WIFALIGN_CENTER;

		filter.Create("");
		filter.SetCancelInputEnabled(false);
	}
	float ComboBox::GetDropOffset(const wi::Canvas& canvas) const
	{
		float screenheight = canvas.GetLogicalHeight();
		int visible_items = std::min(maxVisibleItemCount, filteredItemCount - firstItemVisible);
		if (!filterText.empty())
		{
			visible_items = 0;
			for (int i = firstItemVisible; (i < (int)items.size()) && (visible_items < maxVisibleItemCount); ++i)
			{
				if (wi::helper::toUpper(items[i].name).find(filterText) != std::string::npos)
				{
					visible_items++;
				}
			}
		}
		float total_height = scale.y + visible_items * combo_height();
		if (translation.y + total_height > screenheight)
		{
			return -total_height - 1;
		}
		return combo_height(); // space for filter sub-widget above
	}
	float ComboBox::GetDropX(const wi::Canvas& canvas) const
	{
		if (fixed_drop_width && translation.x > canvas.GetLogicalWidth() * 0.5f)
		{
			// in this case, left-align the drop down:
			float x = translation.x + scale.x - fixed_drop_width;
			if (HasScrollbar())
			{
				x -= 1 + scale.y;
			}
			x = std::max(0.0f, x);
			return x;
		}
		return translation.x;
	}
	float ComboBox::GetItemOffset(const wi::Canvas& canvas, int index) const
	{
		if (!filterText.empty())
		{
			int invisible_items = 0;
			for (int i = firstItemVisible; (i < (int)items.size()) && (i < index); ++i)
			{
				if (wi::helper::toUpper(items[i].name).find(filterText) == std::string::npos)
				{
					invisible_items++;
				}
			}
			index -= invisible_items;
		}
		index = std::max(firstItemVisible, index) - firstItemVisible;
		return scale.y + combo_height() * index + GetDropOffset(canvas);
	}
	bool ComboBox::HasScrollbar() const
	{
		return maxVisibleItemCount < filteredItemCount;
	}
	void ComboBox::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		const float drop_width = fixed_drop_width > 0 ? fixed_drop_width : (scale.x - 1 - scale.y);
		const float drop_x = GetDropX(canvas);

		if (dt > 0)
		{
			if (IsEnabled())
			{
				float drop_offset = GetDropOffset(canvas);

				if (state == FOCUS)
				{
					state = IDLE;
				}
				if (state == DEACTIVATING)
				{
					state = IDLE;
				}
				if (state == ACTIVE && combostate == COMBOSTATE_SELECTING)
				{
					hovered = -1;
					Deactivate();
				}
				if (state == IDLE)
				{
					combostate = COMBOSTATE_INACTIVE;
				}

				hitBox.pos.x = translation.x;
				hitBox.pos.y = translation.y;
				hitBox.siz.x = scale.x;
				if (drop_arrow)
				{
					hitBox.siz.x += scale.y + 1; // + drop-down indicator arrow + little offset
				}
				hitBox.siz.y = scale.y;

				Hitbox2D pointerHitbox = GetPointerHitbox();

				bool clicked = false;
				// hover the button
				if (pointerHitbox.intersects(hitBox))
				{
					if (state == IDLE)
					{
						state = FOCUS;
					}
				}

				if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
				{
					// activate
					clicked = true;
				}

				bool click_down = false;
				if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
				{
					click_down = true;
					if (state == DEACTIVATING)
					{
						// Keep pressed until mouse is released
						Activate();
					}
				}


				if (clicked && state == FOCUS)
				{
					Activate();
				}

				if (state == ACTIVE)
				{
					filteredItemCount = int(items.size());
					if (!filterText.empty())
					{
						filteredItemCount = 0;
						for (int i = 0; i < (int)items.size(); ++i)
						{
							if (wi::helper::toUpper(items[i].name).find(filterText) == std::string::npos)
								continue;
							filteredItemCount++;
						}
					}

					const float scrollbar_begin = translation.y + scale.y + drop_offset + scale.y * 0.5f;
					const float scrollbar_end = scrollbar_begin + std::max(0.0f, (float)std::min(maxVisibleItemCount, filteredItemCount) - 1) * combo_height();

					pointerHitbox = GetPointerHitbox(false); // get the hitbox again, but this time it won't be constrained to parent
					if (HasScrollbar())
					{
						if (combostate != COMBOSTATE_SELECTING && combostate != COMBOSTATE_INACTIVE)
						{
							if (combostate == COMBOSTATE_SCROLLBAR_GRABBED || pointerHitbox.intersects(Hitbox2D(XMFLOAT2(drop_x + drop_width + 1, translation.y + scale.y + drop_offset), XMFLOAT2(scale.y, (float)std::min(maxVisibleItemCount, filteredItemCount) * combo_height()))))
							{
								if (click_down)
								{
									filter.SetAsActive();
									combostate = COMBOSTATE_SCROLLBAR_GRABBED;
									scrollbar_delta = wi::math::Clamp(pointerHitbox.pos.y, scrollbar_begin, scrollbar_end) - scrollbar_begin;
									const float scrollbar_value = wi::math::InverseLerp(scrollbar_begin, scrollbar_end, scrollbar_begin + scrollbar_delta);
									firstItemVisible = int(float(std::max(0, filteredItemCount - maxVisibleItemCount)) * scrollbar_value + 0.5f);
									firstItemVisible = std::max(0, std::min(filteredItemCount - maxVisibleItemCount, firstItemVisible));
								}
								else
								{
									combostate = COMBOSTATE_SCROLLBAR_HOVER;
								}
							}
							else if (!click_down)
							{
								combostate = COMBOSTATE_HOVER;
							}
						}
					}

					if (combostate == COMBOSTATE_INACTIVE)
					{
						combostate = COMBOSTATE_HOVER;
						filter.SetAsActive();
					}
					else if (combostate == COMBOSTATE_SELECTING || wi::input::Press(wi::input::KEYBOARD_BUTTON_ESCAPE))
					{
						Deactivate();
						combostate = COMBOSTATE_INACTIVE;
					}
					else if (combostate == COMBOSTATE_HOVER && scroll_allowed)
					{
						scroll_allowed = false;

						if (HasScrollbar())
						{
							int scroll = int(wi::input::GetPointer().z - wi::input::GetTouchPan().y * 0.1f);
							firstItemVisible -= scroll;
							firstItemVisible = std::max(0, std::min(filteredItemCount - maxVisibleItemCount, firstItemVisible));
							if (scroll)
							{
								const float scrollbar_value = wi::math::InverseLerp(0, float(std::max(0, filteredItemCount - maxVisibleItemCount)), float(firstItemVisible));
								scrollbar_delta = wi::math::Lerp(scrollbar_begin, scrollbar_end, scrollbar_value) - scrollbar_begin;
							}
						}

						hovered = -1;
						int visible_items = 0;
						for (int i = firstItemVisible; (i < (int)items.size()) && (visible_items < maxVisibleItemCount); ++i)
						{
							if (!filterText.empty() && wi::helper::toUpper(items[i].name).find(filterText) == std::string::npos)
								continue;
							visible_items++;
							Hitbox2D itembox;
							itembox.pos.x = drop_x;
							itembox.pos.y = translation.y + GetItemOffset(canvas, i);
							itembox.siz.x = drop_width;
							itembox.siz.y = combo_height();
							if (pointerHitbox.intersects(itembox))
							{
								hovered = i;
								break;
							}
						}

						if (clicked)
						{
							if (pointerHitbox.intersects(filter.hitBox))
							{
								combostate = COMBOSTATE_FILTER_INTERACT;
							}
							else
							{
								combostate = COMBOSTATE_SELECTING;
								if (hovered >= 0)
								{
									SetSelected(hovered);
								}
							}
						}
					}
					else if (combostate == COMBOSTATE_FILTER_INTERACT)
					{
						// nothing here, but this holds main widget active while filter interaction is detected
						if (clicked && !pointerHitbox.intersects(filter.hitBox))
						{
							combostate = COMBOSTATE_INACTIVE;
						}
					}
				}

				if (state == ACTIVE) // intentionally checks base state again!
				{
					filter.Activate();
					filter.scale_local.x = drop_width;
					filter.scale_local.y = combo_height() - filter.GetShadowRadius() * 2;
					filter.translation_local.x = drop_x;
					filter.translation_local.y = translation.y + scale.y + drop_offset - combo_height() + filter.GetShadowRadius();
					filter.SetDirty();
					std::string newFilterText = wi::helper::toUpper(filter.GetText());
					if (newFilterText != filterText)
					{
						firstItemVisible = 0;
						scrollbar_delta = 0;
					}
					filterText = newFilterText;
					filter.font.params.size = int(filter.scale_local.y - 4);
				}
				else
				{
					filter.Deactivate();
					filter.SetText("");
					filterText = "";
				}
			}
			else
			{
				filter.Deactivate();
			}
		}

		filter.SetEnabled(enabled);
		filter.force_disable = force_disable;
		filter.Update(canvas, dt);

		sprites[state].params.siz.x = scale.x;
		if (IsDropArrowEnabled())
		{
			sprites[state].params.siz.x -= scale.y + 1;
		}

		font.params.posY = translation.y + sprites[state].params.siz.y * 0.5f;

		selected = std::min(selected, (int)items.size() - 1);

		if (selected >= 0 && selected < (int)items.size())
		{
			selected_font.SetText(items[selected].name);
		}
		else
		{
			selected_font.SetText(invalid_selection_text);
		}
		selected_font.params.posX = translation.x + sprites[state].params.siz.x * 0.5f;
		selected_font.params.posY = translation.y + scale.y * 0.5f;
		selected_font.Update(dt);

		scissorRect.bottom = (int32_t)std::ceil(translation.y + scale.y);
		scissorRect.left = (int32_t)std::floor(translation.x);
		scissorRect.right = (int32_t)std::ceil(translation.x + sprites[state].params.siz.x);
		scissorRect.top = (int32_t)std::floor(translation.y);

		left_text_width = font.TextWidth();
	}
	void ComboBox::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		Widget::Render(canvas, cmd);
		if (!IsVisible())
		{
			return;
		}
		GraphicsDevice* device = wi::graphics::GetDevice();

		const float drop_width = fixed_drop_width > 0 ? fixed_drop_width : (scale.x - 1 - scale.y);
		const float drop_x = GetDropX(canvas);

		// shadow:
		if (shadow > 0)
		{
			wi::image::Params fx = sprites[state].params;
			fx.gradient = wi::image::Params::Gradient::None;
			fx.pos.x -= shadow;
			fx.pos.y -= shadow;
			fx.siz.x = scale.x;
			fx.siz.x += shadow * 2;
			fx.siz.y += shadow * 2;
			fx.color = shadow_color;
			if (fx.isCornerRoundingEnabled())
			{
				for (auto& corner_rounding : fx.corners_rounding)
				{
					if (corner_rounding.radius > 0)
					{
						corner_rounding.radius += shadow;
					}
				}
			}
			if (shadow_highlight)
			{
				fx.enableHighlight();
				fx.highlight_pos = GetPointerHighlightPos(canvas);
				fx.highlight_color = shadow_highlight_color;
				fx.highlight_spread = shadow_highlight_spread;
			}
			else
			{
				fx.disableHighlight();
			}
			wi::image::Draw(nullptr, fx, cmd);
		}

		wi::Color color = GetColor();
		if (combostate != COMBOSTATE_INACTIVE)
		{
			color = wi::Color::fromFloat4(sprites[FOCUS].params.color);
		}

		font.Draw(cmd);

		const float drop_offset = GetDropOffset(canvas);

		if (drop_arrow)
		{
			struct Vertex
			{
				XMFLOAT4 pos;
				XMFLOAT4 col;
			};
			static GPUBuffer vb_triangle;
			if (!vb_triangle.IsValid())
			{
				Vertex vertices[3];
				vertices[0].col = XMFLOAT4(1, 1, 1, 1);
				vertices[1].col = XMFLOAT4(1, 1, 1, 1);
				vertices[2].col = XMFLOAT4(1, 1, 1, 1);
				wi::math::ConstructTriangleEquilateral(1, vertices[0].pos, vertices[1].pos, vertices[2].pos);

				GPUBufferDesc desc;
				desc.bind_flags = BindFlag::VERTEX_BUFFER;
				desc.size = sizeof(vertices);
				device->CreateBuffer(&desc, &vertices, &vb_triangle);
			}
			const XMMATRIX Projection = canvas.GetProjection();

			// control-arrow-background
			wi::image::Params fx = sprites[state].params;
			fx.disableCornerRounding();
			fx.pos = XMFLOAT3(translation.x + scale.x - scale.y, translation.y, 0);
			fx.siz = XMFLOAT2(scale.y, scale.y);
			wi::image::Draw(nullptr, fx, cmd);

			// control-arrow-triangle
			{
				device->BindPipelineState(&gui_internal().PSO_colored, cmd);

				MiscCB cb;
				cb.g_xColor = font.params.color;
				XMStoreFloat4x4(&cb.g_xTransform, XMMatrixScaling(scale.y * 0.25f, scale.y * 0.25f, 1) *
					XMMatrixRotationZ(drop_offset < 0 ? -XM_PIDIV2 : XM_PIDIV2) *
					XMMatrixTranslation(translation.x + scale.x - scale.y * 0.5f, translation.y + scale.y * 0.5f, 0) *
					Projection
				);
				device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
				const GPUBuffer* vbs[] = {
					&vb_triangle,
				};
				const uint32_t strides[] = {
					sizeof(Vertex),
				};
				device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);

				device->Draw(3, 0, cmd);
			}
		}

		ApplyScissor(canvas, scissorRect, cmd);

		// control-base
		sprites[state].Draw(cmd);

		selected_font.Draw(cmd);

		// drop-down
		if (state == ACTIVE)
		{
			{
				wi::graphics::Rect fullscissorRect;
				fullscissorRect.bottom = (int32_t)(canvas.GetPhysicalHeight());
				fullscissorRect.left = (int32_t)(0);
				fullscissorRect.right = (int32_t)(canvas.GetPhysicalWidth());
				fullscissorRect.top = (int32_t)(0);
				ApplyScissor(canvas, fullscissorRect, cmd);
				filter.Render(canvas, cmd);
			}

			if (HasScrollbar())
			{
				wi::graphics::Rect rect;
				rect.left = int(drop_x + drop_width + 1);
				rect.right = int(drop_x + drop_width + 1 + scale.y);
				rect.top = int(translation.y + scale.y + drop_offset);
				rect.bottom = int(translation.y + scale.y + drop_offset + combo_height() * maxVisibleItemCount);
				ApplyScissor(canvas, rect, cmd, false);

				// control-scrollbar-base
				{
					wi::image::Params fx = sprites[state].params;
					fx.disableCornerRounding();
					fx.pos = XMFLOAT3(drop_x + drop_width + 1, translation.y + scale.y + drop_offset, 0);
					fx.siz = XMFLOAT2(scale.y, combo_height() * maxVisibleItemCount);
					fx.color = drop_color;
					wi::image::Draw(nullptr, fx, cmd);
				}

				// control-scrollbar-grab
				{
					wi::Color col = wi::Color::fromFloat4(sprites[IDLE].params.color);
					if (combostate == COMBOSTATE_SCROLLBAR_HOVER)
					{
						col = wi::Color::fromFloat4(sprites[FOCUS].params.color);
					}
					else if (combostate == COMBOSTATE_SCROLLBAR_GRABBED)
					{
						col = wi::Color::fromFloat4(sprites[ACTIVE].params.color);
					}
					wi::image::Draw(
						nullptr,
						wi::image::Params(
							drop_x + drop_width + 1,
							translation.y + scale.y + drop_offset + scrollbar_delta,
							scale.y,
							combo_height(),
							col
						),
						cmd
					);
				}
			}

			wi::graphics::Rect rect;
			rect.left = int(drop_x);
			rect.right = rect.left + int(drop_width);
			rect.top = int(translation.y + scale.y + drop_offset);
			rect.bottom = rect.top + int(combo_height() * maxVisibleItemCount);
			ApplyScissor(canvas, rect, cmd, false);

			// control-list
			int visible_items = 0;
			for (int i = firstItemVisible; (i < (int)items.size()) && (visible_items < maxVisibleItemCount); ++i)
			{
				if (!filterText.empty() && wi::helper::toUpper(items[i].name).find(filterText) == std::string::npos)
					continue;
				visible_items++;
				wi::image::Params fx = sprites[state].params;
				fx.disableCornerRounding();
				fx.pos = XMFLOAT3(drop_x, translation.y + GetItemOffset(canvas, i), 0);
				fx.siz = XMFLOAT2(drop_width, combo_height());
				fx.color = drop_color;
				if (hovered == i)
				{
					if (combostate == COMBOSTATE_HOVER)
					{
						fx.color = sprites[FOCUS].params.color;
					}
					else if (combostate == COMBOSTATE_SELECTING)
					{
						fx.color = sprites[ACTIVE].params.color;
					}
				}
				wi::image::Draw(nullptr, fx, cmd);

				wi::font::Params fp = wi::font::Params(
					drop_x + drop_width * 0.5f,
					translation.y + combo_height() * 0.5f + GetItemOffset(canvas, i),
					wi::font::WIFONTSIZE_DEFAULT,
					wi::font::WIFALIGN_CENTER,
					wi::font::WIFALIGN_CENTER,
					font.params.color,
					font.params.shadowColor
				);
				fp.style = font.params.style;
				wi::font::Draw(items[i].name, fp, cmd);
			}
		}
	}
	void ComboBox::OnSelect(std::function<void(const EventArgs& args)> func)
	{
		onSelect = std::move(func);
	}
	void ComboBox::AddItem(const std::string& name, uint64_t userdata)
	{
		items.emplace_back();
		items.back().name = name;
		items.back().userdata = userdata;

		if (selected < 0 && invalid_selection_text.empty())
		{
			selected = 0;
		}
	}
	void ComboBox::RemoveItem(int index)
	{
		if (index < 0 || (size_t)index >= items.size()) {
			return;
		}

		items.erase(items.begin() + index);

		if (items.empty())
		{
			selected = -1;
		}
		else if (selected > index)
		{
			selected--;
		}
	}
	void ComboBox::ClearItems()
	{
		items.clear();

		selected = -1;
		//firstItemVisible = 0;
	}
	void ComboBox::SetMaxVisibleItemCount(int value)
	{
		maxVisibleItemCount = value;
	}
	void ComboBox::SetSelected(int index)
	{
		SetSelectedWithoutCallback(index);

		if (onSelect != nullptr)
		{
			EventArgs args;
			args.iValue = selected;
			args.sValue = GetItemText(selected);
			args.userdata = GetItemUserData(selected);
			onSelect(args);
		}
	}
	void ComboBox::SetSelectedWithoutCallback(int index)
	{
		selected = index;
	}
	void ComboBox::SetSelectedByUserdata(uint64_t userdata)
	{
		for (int i = 0; i < GetItemCount(); ++i)
		{
			if (userdata == GetItemUserData(i))
			{
				SetSelected(i);
				return;
			}
		}
	}
	void ComboBox::SetSelectedByUserdataWithoutCallback(uint64_t userdata)
	{
		for (int i = 0; i < GetItemCount(); ++i)
		{
			if (userdata == GetItemUserData(i))
			{
				SetSelectedWithoutCallback(i);
				return;
			}
		}
	}
	void ComboBox::SetItemText(int index, const std::string& text)
	{
		if (index >= 0 && index < items.size())
		{
			items[index].name = text;
		}
	}
	void ComboBox::SetItemUserdata(int index, uint64_t userdata)
	{
		if (index >= 0 && index < items.size())
		{
			items[index].userdata = userdata;
		}
	}
	void ComboBox::SetInvalidSelectionText(const std::string& text)
	{
		wi::helper::StringConvert(text, invalid_selection_text);
	}
	std::string ComboBox::GetItemText(int index) const
	{
		if (index >= 0 && index < items.size())
		{
			return items[index].name;
		}
		return "";
	}
	uint64_t ComboBox::GetItemUserData(int index) const
	{
		if (index >= 0 && index < items.size())
		{
			return items[index].userdata;
		}
		return 0;
	}
	int ComboBox::GetSelected() const
	{
		return selected;
	}
	uint64_t ComboBox::GetSelectedUserdata() const
	{
		return GetItemUserData(GetSelected());
	}
	void ComboBox::SetColor(wi::Color color, int id)
	{
		Widget::SetColor(color, id);
		filter.SetColor(color, id);

		if (id == WIDGET_ID_COMBO_DROPDOWN)
		{
			drop_color = color;
		}
	}
	void ComboBox::SetTheme(const Theme& theme, int id)
	{
		Widget::SetTheme(theme, id);
		filter.SetTheme(theme, id);

		if (id == WIDGET_ID_COMBO_DROPDOWN)
		{
			drop_color = wi::Color::fromFloat4(theme.image.color);
		}
		theme.font.Apply(selected_font.params);
	}
	void ComboBox::ExportLocalization(wi::Localization& localization) const
	{
		Widget::ExportLocalization(localization);
		if (has_flag(localization_enabled, LocalizationEnabled::Items))
		{
			wi::Localization& section = localization.GetSection(GetName()).GetSection("items");
			for (size_t i = 0; i < items.size(); ++i)
			{
				section.Add(i, items[i].name.c_str());
			}
		}
	}
	void ComboBox::ImportLocalization(const wi::Localization& localization)
	{
		Widget::ImportLocalization(localization);
		if (has_flag(localization_enabled, LocalizationEnabled::Items))
		{
			const wi::Localization* section = localization.CheckSection(GetName());
			if (section == nullptr)
				return;
			section = section->CheckSection("items");
			if (section == nullptr)
				return;
			for (size_t i = 0; i < items.size(); ++i)
			{
				const char* localized_item_name = section->Get(i);
				if (localized_item_name != nullptr)
				{
					items[i].name = localized_item_name;
				}
			}
		}
	}
}
