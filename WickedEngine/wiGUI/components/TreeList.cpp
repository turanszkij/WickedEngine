#include "wiGUI/components/TreeList.h"
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
	constexpr float item_height() { return 20.0f; }
	void TreeList::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnSelect([](const EventArgs& args) {});

		SetColor(wi::Color(100, 100, 100, 100), wi::gui::IDLE);
		for (int i = FOCUS + 1; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites[i].params.color = sprites[FOCUS].params.color;
			scrollbar.sprites[i].params.color = sprites[FOCUS].params.color;
		}
		font.params.v_align = wi::font::WIFALIGN_CENTER;

		scrollbar.SetColor(wi::Color(80, 80, 80, 100), wi::gui::IDLE);
		scrollbar.sprites_knob[ScrollBar::SCROLLBAR_INACTIVE].params.color = wi::Color(140, 140, 140, 140);
		scrollbar.sprites_knob[ScrollBar::SCROLLBAR_HOVER].params.color = wi::Color(180, 180, 180, 180);
		scrollbar.sprites_knob[ScrollBar::SCROLLBAR_GRABBED].params.color = wi::Color::White();
		scrollbar.SetOverScroll(0.25f);

		SetSize(XMFLOAT2(100, 200));
	}
	bool TreeList::DoesItemHaveChildren(int index) const
	{
		if (items.size() <= size_t(index + 1)) // if item doesn't exist or last then no children
			return false;
		if (items[index].level + 1 == items[index + 1].level) // if item after index is exactly one level down, then it is its child
			return true;
		return false;
	}
	float TreeList::GetItemOffset(int index) const
	{
		return 2 + scrollbar.GetOffset() + index * item_height();
	}
	Hitbox2D TreeList::GetHitbox_ListArea() const
	{
		Hitbox2D retval = Hitbox2D(XMFLOAT2(translation.x, translation.y + item_height() + 1), XMFLOAT2(scale.x, scale.y - item_height() - 1));
		if (scrollbar.IsScrollbarRequired())
		{
			retval.siz.x -= scrollbar.scale.x + 1;
		}
		return retval;
	}
	Hitbox2D TreeList::GetHitbox_Item(int visible_count, int level) const
	{
		XMFLOAT2 pos = XMFLOAT2(translation.x + 2 + level * item_height(), translation.y + GetItemOffset(visible_count) + item_height() * 0.5f);
		Hitbox2D hitbox;
		hitbox.pos = XMFLOAT2(pos.x + item_height() * 0.5f + 2, pos.y - item_height() * 0.5f);
		hitbox.siz = XMFLOAT2(scale.x - 2 - item_height() * 0.5f - 2 - level * item_height() - 2, item_height());
		if (HasScrollbar())
		{
			hitbox.siz.x -= scrollbar.scale.x;
		}
		return hitbox;
	}
	Hitbox2D TreeList::GetHitbox_ItemOpener(int visible_count, int level) const
	{
		XMFLOAT2 pos = XMFLOAT2(translation.x + 2 + level * item_height(), translation.y + GetItemOffset(visible_count) + item_height() * 0.5f);
		Hitbox2D hb = Hitbox2D(XMFLOAT2(pos.x, pos.y - item_height() * 0.25f), XMFLOAT2(item_height() * 0.5f, item_height() * 0.5f));
		HitboxConstrain(hb);
		return hb;
	}
	bool TreeList::HasScrollbar() const
	{
		return scale.y < (int)items.size() * item_height();
	}
	void TreeList::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		// Resizer updates:
		if (IsEnabled())
		{
			Hitbox2D pointerHitbox = GetPointerHitbox(false);

			float vscale = scale.y;
			Hitbox2D bottomhitbox = Hitbox2D(XMFLOAT2(translation.x, translation.y + vscale), XMFLOAT2(scale.x, resizehitboxwidth));

			if (resize_state == RESIZE_STATE_NONE && (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanStarting()))
			{
				if (pointerHitbox.intersects(bottomhitbox))
				{
					resize_state = RESIZE_STATE_BOTTOM;
					Activate();
					force_disable = true;
				}
				resize_begin = pointerHitbox.pos;
			}
			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				if (resize_state != RESIZE_STATE_NONE)
				{
					auto saved_parent = this->parent;
					this->Detach();
					float deltaX = pointerHitbox.pos.x - resize_begin.x;
					float deltaY = pointerHitbox.pos.y - resize_begin.y;

					switch (resize_state)
					{
					case RESIZE_STATE_BOTTOM:
						this->Scale(XMFLOAT3(1, (scale.y + deltaY) / scale.y, 1));
						break;
					default:
						break;
					}

					this->scale_local = wi::math::Max(this->scale_local, XMFLOAT3(item_height() * 3, item_height() * 3, 1)); // don't allow resize to negative or too small
					this->scale = this->scale_local;
					this->AttachTo(saved_parent);
					resize_begin = pointerHitbox.pos;
				}
			}
			else
			{
				resize_state = RESIZE_STATE_NONE;
			}

			if (
				resize_state != RESIZE_STATE_NONE ||
				pointerHitbox.intersects(bottomhitbox)
				)
			{
				resize_blink_timer += dt;
			}
			else
			{
				resize_blink_timer = 0;
			}
		}

		active_area.pos.x = float(scissorRect.left);
		active_area.pos.y = float(scissorRect.top);
		active_area.siz.x = float(scissorRect.right) - float(scissorRect.left);
		active_area.siz.y = float(scissorRect.bottom) - float(scissorRect.top);

		Hitbox2D pointerHitbox = GetPointerHitbox();

		ComputeScrollbarLength();

		const float scrollbar_width = 12;
		scrollbar.SetSize(XMFLOAT2(scrollbar_width - 1, scale.y - 1 - item_height()));
		scrollbar.SetPos(XMFLOAT2(translation.x + 1 + scale.x - scrollbar_width, translation.y + 1 + item_height()));
		scrollbar.Update(canvas, dt);
		if (scrollbar.GetState() > IDLE)
		{
			Deactivate();
		}

		if (IsEnabled() && dt > 0)
		{
			if (state == FOCUS)
			{
				state = IDLE;
			}
			if (state == DEACTIVATING)
			{
				state = IDLE;
			}
			if (state == ACTIVE)
			{
				Deactivate();
			}

			Hitbox2D hitbox = Hitbox2D(XMFLOAT2(translation.x, translation.y), XMFLOAT2(scale.x, scale.y));

			if (state == IDLE && hitbox.intersects(pointerHitbox))
			{
				state = FOCUS;
			}

			bool clicked = false;
			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				clicked = true;
			}

			if (onDelete && state == FOCUS && wi::input::Press(wi::input::KEYBOARD_BUTTON_DELETE) && !typing_active)
			{
				int index = 0;
				for (auto& item : items)
				{
					if (item.selected)
					{
						EventArgs args;
						args.iValue = index;
						args.sValue = items[index].name;
						args.userdata = items[index].userdata;
						onDelete(args);
					}
					index++;
				}
			}

			bool click_down = false;
			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				click_down = true;
				if (state == FOCUS || state == DEACTIVATING)
				{
					// Keep pressed until mouse is released
					Activate();
				}
			}

			const float wheel_delta = wi::input::GetPointer().z;
			if (wheel_delta != 0.0f && scroll_allowed && scrollbar.IsScrollbarRequired() && pointerHitbox.intersects(hitBox))
			{
				const bool at_begin = (wheel_delta > 0 && scrollbar.IsScrolledToBegin());
				if (!at_begin)
				{
					scroll_allowed = false;
					// This is outside scrollbar code, because it can also be scrolled if parent widget is only in focus
					scrollbar.Scroll(wheel_delta * 40.0f);
				}
			}

			Hitbox2D itemlist_box = GetHitbox_ListArea();

			// Finalize drag-and-drop when mouse is released
			if (onReorder && dragging && !click_down)
			{
				if (drag_source >= 0)
				{
					if (drag_reparent_target >= 0 && drag_reparent_target != drag_source)
					{
						// Re-parent: drop ON another item
						EventArgs args;
						args.iValue = drag_source;
						args.iValue2 = drag_reparent_target;
						args.userdata = items[drag_source].userdata;
						args.bValue = true;
						onReorder(args);
					}
					else if (!hitbox.intersects(GetPointerHitbox(false)))
					{
						// Dropped outside the widget -> go one level up in hierarchy
						EventArgs args;
						args.iValue = drag_source;
						args.iValue2 = -1;
						args.userdata = items[drag_source].userdata;
						args.bValue = true;
						onReorder(args);
					}
					else if (drag_reparent_target < 0 && drag_target >= 0 &&
						((drag_target != drag_source && drag_target != drag_source + 1) ||
							(drag_source >= 0 && drag_source < (int)items.size() && drag_target_level != items[drag_source].level)))
					{
						// Reorder: drop BETWEEN items
						EventArgs args;
						args.iValue = drag_source;
						args.iValue2 = drag_target;
						args.fValue = (float)drag_target_level;
						args.userdata = items[drag_source].userdata;
						args.bValue = false;
						onReorder(args);
					}
				}
				dragging = false;
				drag_source = -1;
				drag_target = -1;
				drag_reparent_target = -1;
				drag_target_level = 0;
			}
			if (!click_down && !clicked)
			{
				// Reset potential drag if mouse was released without dragging
				if (!dragging)
					drag_source = -1;
			}

			tooltipFont.text.clear();

			// control-list
			item_highlight = -1;
			opener_highlight = -1;
			int first_selected = -1;
			if (scrollbar.GetState() == IDLE)
			{
				int i = -1;
				int visible_count = 0;
				int parent_level = 0;
				bool parent_open = true;
				for (Item& item : items)
				{
					i++;
					if (item.selected && first_selected < 0)
					{
						first_selected = i;
					}
					if (!parent_open && item.level > parent_level)
					{
						continue;
					}
					visible_count++;
					parent_open = item.open;
					parent_level = item.level;

					Hitbox2D open_box = GetHitbox_ItemOpener(visible_count, item.level);
					if (!open_box.intersects(itemlist_box))
					{
						continue;
					}

					if (open_box.intersects(pointerHitbox))
					{
						// Opened flag box:
						opener_highlight = i;
						if (clicked)
						{
							item.open = !item.open;
							Activate();
						}
					}
					else
					{
						// Item name box:
						Hitbox2D name_box = GetHitbox_Item(visible_count, item.level);
						if (name_box.intersects(pointerHitbox))
						{
							item_highlight = i;
							SetTooltip(item.name);
							if (clicked)
							{
								// Start potential drag (activated on movement threshold)
								if (onReorder)
								{
									drag_source = i;
									drag_start_pos = pointerHitbox.pos;
								}

								if (wi::input::IsDoubleClicked() && onDoubleClick != nullptr)
								{
									EventArgs args;
									args.iValue = i;
									args.sValue = items[i].name;
									args.userdata = items[i].userdata;
									onDoubleClick(args);
								}
								else
								{
									if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL) || wi::input::Down(wi::input::KEYBOARD_BUTTON_RCONTROL))
									{
										// control: nothing
									}
									else if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) || wi::input::Down(wi::input::KEYBOARD_BUTTON_RSHIFT))
									{
										if (first_selected >= 0)
										{
											// shift: select range from first selected up to current:
											for (int j = first_selected; j < i; ++j)
											{
												Select(j, false);
											}
										}
									}
									else
									{
										// no control, no shift then only this will be selected
										ClearSelection();
									}
									Select(i);
								}
								Activate();
							}
						}
					}
				}
			}
			// Activate drag mode after movement threshold
			if (onReorder && drag_source >= 0 && click_down && !dragging)
			{
				float dx = pointerHitbox.pos.x - drag_start_pos.x;
				float dy = pointerHitbox.pos.y - drag_start_pos.y;
				if (dx * dx + dy * dy > 16.0f) // 4 px threshold
				{
					dragging = true;
				}
			}

			// Compute drag_target, drag_reparent_target, and drag_indicator_y while dragging
			if (onReorder && dragging && click_down)
			{
				item_highlight = drag_source;
				drag_target = (int)items.size(); // default: insert after all items
				drag_reparent_target = -1;
				drag_target_level = 0;
				drag_indicator_y = 0;
				auto compute_drag_target_level = [&](int max_level) {
					int source_level = 0;
					if (drag_source >= 0 && drag_source < (int)items.size())
					{
						source_level = items[drag_source].level;
					}
					const float level_delta = (pointerHitbox.pos.x - drag_start_pos.x) / item_height();
					const int level = source_level + (int)std::round(level_delta);
					return std::max(0, std::min(level, max_level));
				};

				int dc = 0;
				int dp = 0;
				bool dpo = true;
				int di = -1;
				int last_visible = 0;
				int last_visible_level = 0;
				for (const Item& item : items)
				{
					di++;
					if (!dpo && item.level > dp)
						continue;
					dc++;
					dpo = item.open;
					dp = item.level;
					last_visible = dc;
					last_visible_level = item.level;

					float item_top_y = translation.y + GetItemOffset(dc);
					float item_bot_y = item_top_y + item_height();
					if (pointerHitbox.pos.y < item_top_y + item_height() * 0.25f)
					{
						// Top 25%: insert BEFORE this item
						drag_target = di;
						drag_reparent_target = -1;
						drag_target_level = item.level;
						drag_indicator_y = item_top_y;
						break;
					}
					else if (pointerHitbox.pos.y < item_bot_y - item_height() * 0.25f)
					{
						// Middle 50%: drop ON this item (re-parent)
						if (di != drag_source)
						{
							drag_reparent_target = di;
							drag_target = (int)items.size();
							drag_target_level = item.level + 1;
							drag_indicator_y = 0;
						}
						else
						{
							drag_target = drag_source;
							drag_target_level = item.level;
							drag_indicator_y = 0;
						}
						break;
					}
					else if (pointerHitbox.pos.y < item_bot_y)
					{
						// Bottom 25%: insert AFTER this item
						drag_target = di + 1;
						drag_reparent_target = -1;
						drag_target_level = item.level;
						drag_indicator_y = item_bot_y;
						break;
					}
				}
				if (drag_reparent_target < 0 && drag_target == (int)items.size())
				{
					drag_target_level = compute_drag_target_level(last_visible_level);
					drag_indicator_y = translation.y + GetItemOffset(last_visible) + item_height();
				}
				// Clamp indicator line to list area (only when not re-parenting)
				if (drag_reparent_target < 0)
				{
					drag_indicator_y = std::max(drag_indicator_y, itemlist_box.pos.y);
					drag_indicator_y = std::min(drag_indicator_y, itemlist_box.pos.y + itemlist_box.siz.y - 2.0f);
				}
				drag_pointer_pos = pointerHitbox.pos;
			}

			if (state == IDLE && resize_blink_timer > 0)
				state = FOCUS;
		}

		sprites[state].params.siz.y = item_height();
		font.params.posX = translation.x + 2;
		font.params.posY = translation.y + sprites[state].params.siz.y * 0.5f;
	}
	void TreeList::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		Widget::Render(canvas, cmd);
		if (!IsVisible())
		{
			return;
		}
		GraphicsDevice* device = wi::graphics::GetDevice();

		// shadow:
		if (shadow > 0)
		{
			wi::image::Params fx = sprites[state].params;
			fx.gradient = wi::image::Params::Gradient::None;
			fx.pos.x -= shadow;
			fx.pos.y -= shadow;
			fx.siz.x += shadow * 2;
			fx.siz.y = scale.y + shadow * 2;
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

		// resize indicator:
		{
			// hitboxes are recomputed because window transform might have changed since update!!
			float vscale = scale.y;
			Hitbox2D bottomhitbox = Hitbox2D(XMFLOAT2(translation.x, translation.y + vscale), XMFLOAT2(scale.x, resizehitboxwidth));

			const Hitbox2D pointerHitbox = GetPointerHitbox(false);

			wi::image::Params fx = sprites[state].params;
			fx.blendFlag = wi::enums::BLENDMODE_ALPHA;
			fx.pos.x -= resizehitboxwidth;
			fx.pos.y -= resizehitboxwidth;
			fx.siz.x += resizehitboxwidth * 2;
			fx.siz.y = scale.y + resizehitboxwidth * 2;
			fx.color = resize_state == RESIZE_STATE_NONE ? sprites[FOCUS].params.color : sprites[ACTIVE].params.color;
			if (fx.isCornerRoundingEnabled())
			{
				for (auto& corner_rounding : fx.corners_rounding)
				{
					if (corner_rounding.radius > 0)
					{
						corner_rounding.radius += resizehitboxwidth;
					}
				}
			}
			//fx.border_soften = 0.01f;

			if (resize_state == RESIZE_STATE_BOTTOM || pointerHitbox.intersects(bottomhitbox))
			{
				fx.angular_softness_outer_angle = XM_PI * 0.25f;
				fx.angular_softness_inner_angle = XM_PI * wi::math::Lerp(0.0f, 0.24f, std::abs(std::sin(resize_blink_timer * 4)));
				fx.angular_softness_direction = XMFLOAT2(0, 1);
				wi::image::Draw(nullptr, fx, cmd);
				wi::input::SetCursor(wi::input::CURSOR_RESIZE_NS);
			}
		}

		// control-base
		sprites[state].Draw(cmd);

		ApplyScissor(canvas, scissorRect, cmd);

		font.Draw(cmd);

		scrollbar.Render(canvas, cmd);

		// list background
		Hitbox2D itemlist_box = GetHitbox_ListArea();
		wi::image::Params fx = sprites[state].params;
		fx.color = sprites[IDLE].params.color;
		fx.pos = XMFLOAT3(itemlist_box.pos.x, itemlist_box.pos.y, 0);
		fx.siz = XMFLOAT2(itemlist_box.siz.x, itemlist_box.siz.y);
		wi::image::Draw(nullptr, fx, cmd);

		wi::graphics::Rect rect_without_scrollbar;
		rect_without_scrollbar.left = (int)itemlist_box.pos.x;
		rect_without_scrollbar.right = (int)(itemlist_box.pos.x + itemlist_box.siz.x);
		rect_without_scrollbar.top = (int)itemlist_box.pos.y;
		rect_without_scrollbar.bottom = (int)(itemlist_box.pos.y + itemlist_box.siz.y);
		ApplyScissor(canvas, rect_without_scrollbar, cmd);

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
			device->CreateBuffer(&desc, vertices, &vb_triangle);
		}
		const XMMATRIX Projection = canvas.GetProjection();

		// control-list
		int i = -1;
		int visible_count = 0;
		int parent_level = 0;
		bool parent_open = true;
		for (const Item& item : items)
		{
			i++;
			if (!parent_open && item.level > parent_level)
			{
				continue;
			}
			visible_count++;
			parent_open = item.open;
			parent_level = item.level;

			Hitbox2D open_box = GetHitbox_ItemOpener(visible_count, item.level);
			if (!open_box.intersects(itemlist_box))
			{
				continue;
			}

			Hitbox2D name_box = GetHitbox_Item(visible_count, item.level);

			// selected box:
			if (item.selected || item_highlight == i)
			{
				wi::image::Draw(nullptr
					, wi::image::Params(name_box.pos.x, name_box.pos.y, name_box.siz.x, name_box.siz.y,
						sprites[item.selected ? FOCUS : IDLE].params.color), cmd);
			}

			// Re-parent drop target highlight
			if (drag_reparent_target == i)
			{
				wi::image::Params rp_fx = sprites[ACTIVE].params;
				rp_fx.pos = XMFLOAT3(name_box.pos.x, name_box.pos.y, 0);
				rp_fx.siz = XMFLOAT2(name_box.siz.x, name_box.siz.y);
				rp_fx.color.w *= 0.5f;
				wi::image::Draw(nullptr, rp_fx, cmd);
			}

			// opened flag triangle:
			if(DoesItemHaveChildren(i))
			{
				device->BindPipelineState(&gui_internal().PSO_colored, cmd);

				MiscCB cb;
				cb.g_xColor = opener_highlight == i ? wi::Color::White().toFloat4() : sprites[FOCUS].params.color;
				XMStoreFloat4x4(&cb.g_xTransform, XMMatrixScaling(item_height() * 0.3f, item_height() * 0.3f, 1) *
					XMMatrixRotationZ(item.open ? XM_PIDIV2 : 0) *
					XMMatrixTranslation(open_box.pos.x + open_box.siz.x * 0.5f, open_box.pos.y + open_box.siz.y * 0.25f, 0) *
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

			// Item name text:
			wi::font::Params fp = wi::font::Params(
				name_box.pos.x + 1,
				name_box.pos.y + name_box.siz.y * 0.5f,
				wi::font::WIFONTSIZE_DEFAULT,
				wi::font::WIFALIGN_LEFT,
				wi::font::WIFALIGN_CENTER,
				font.params.color,
				font.params.shadowColor
			);
			fp.style = font.params.style;
			wi::font::Draw(item.name, fp, cmd);
			// Drag handle dots on the right side of each item
			if (onReorder)
			{
				float ds = std::max(2.0f, std::round(item_height() * 0.12f));
				float dg = ds * 1.5f;
				float hx = name_box.pos.x + name_box.siz.x - ds * 2 - dg - ds;
				float hy = name_box.pos.y + (name_box.siz.y - (ds * 3 + dg * 2)) * 0.5f;
				XMFLOAT4 dot_color = drag_source == i ? sprites[ACTIVE].params.color : sprites[FOCUS].params.color;
				if (drag_source != i)
					dot_color.w *= 0.45f;
				for (int row = 0; row < 3; ++row)
				{
					for (int col = 0; col < 2; ++col)
					{
						wi::image::Draw(nullptr,
						    wi::image::Params(hx + col * (ds + dg), hy + row * (ds + dg), ds, ds, dot_color),
						    cmd);
					}
				}
			}
		}

		// Drop indicator line for drag-and-drop reorder (not shown when re-parenting)
		if (dragging && drag_source >= 0 && drag_target >= 0 && drag_reparent_target < 0)
		{
			wi::image::Params indicator_fx = sprites[ACTIVE].params;
			const float indicator_indent = std::max(0, drag_target_level) * item_height();
			indicator_fx.pos = XMFLOAT3(itemlist_box.pos.x + indicator_indent, drag_indicator_y - 1.0f, 0);
			indicator_fx.siz = XMFLOAT2(std::max(2.0f, itemlist_box.siz.x - indicator_indent), 2.0f);
			wi::image::Draw(nullptr, indicator_fx, cmd);
		}
		// Floating ghost: semi-transparent label following the cursor while dragging
		if (dragging && drag_source >= 0 && drag_source < (int)items.size())
		{
			const Item& src_item = items[drag_source];
			float ghost_h = item_height();
			float ghost_w = itemlist_box.siz.x * 0.65f;
			float ghost_x = drag_pointer_pos.x + 18.0f;
			float ghost_y = drag_pointer_pos.y - ghost_h * 0.5f;
			wi::image::Params ghost_bg = sprites[ACTIVE].params;
			ghost_bg.pos = XMFLOAT3(ghost_x, ghost_y, 0);
			ghost_bg.siz = XMFLOAT2(ghost_w, ghost_h);
			ghost_bg.color.w *= 0.75f;
			wi::image::Draw(nullptr, ghost_bg, cmd);
			wi::font::Params gfp(
				ghost_x + 6.0f,
				ghost_y + ghost_h * 0.5f,
				wi::font::WIFONTSIZE_DEFAULT,
				wi::font::WIFALIGN_LEFT,
				wi::font::WIFALIGN_CENTER,
				font.params.color,
				font.params.shadowColor);
			gfp.style = font.params.style;
			wi::font::Draw(src_item.name, gfp, cmd);
		}
	}
	void TreeList::OnSelect(std::function<void(const EventArgs& args)> func)
	{
		onSelect = std::move(func);
	}
	void TreeList::OnDelete(std::function<void(const EventArgs& args)> func)
	{
		onDelete = std::move(func);
	}
	void TreeList::OnDoubleClick(std::function<void(const EventArgs& args)> func)
	{
		onDoubleClick = std::move(func);
	}
	void TreeList::OnReorder(std::function<void(const EventArgs& args)> func)
	{
		onReorder = std::move(func);
	}
	void TreeList::AddItem(const Item& item)
	{
		items.push_back(item);
	}
	void TreeList::AddItem(const std::string& name)
	{
		Item item;
		item.name = name;
		AddItem(item);
	}
	void TreeList::ClearItems()
	{
		items.clear();
	}
	void TreeList::ClearSelection()
	{
		for (Item& item : items)
		{
			item.selected = false;
		}

		EventArgs args = {};
		args.iValue = -1;
		onSelect(args);
	}
	void TreeList::Select(int index, bool allow_deselect)
	{
		int selected_count = 0;
		if (allow_deselect)
		{
			for (auto& x : items)
			{
				if (x.selected)
					selected_count++;
			}
		}

		if (selected_count > 1)
		{
			// If multiple are selected, then we can deselect:
			items[index].selected = !items[index].selected;
		}
		else
		{
			items[index].selected = true;
		}

		EventArgs args;
		args.iValue = index;
		args.sValue = items[index].name;
		args.userdata = items[index].userdata;
		onSelect(args);
	}
	void TreeList::ComputeScrollbarLength()
	{
		float scroll_length = 0;
		{
			int parent_level = 0;
			bool parent_open = true;
			for (const Item& item : items)
			{
				if (!parent_open && item.level > parent_level)
				{
					continue;
				}
				parent_open = item.open;
				parent_level = item.level;
				scroll_length += item_height();
			}
		}
		scrollbar.SetListLength(scroll_length);
		scrollbar.Update({}, 0);
	}
	void TreeList::FocusOnItem(int index)
	{
		if (index < 0 || index >= items.size())
			return;

		// Open parent items of target:
		int target = index;
		int target_level = items[target].level;
		while (target_level > 0)
		{
			if (items[target - 1].level == target_level - 1)
			{
				items[target - 1].open = true;
				target_level--;
			}
			target--;
		}

		// Recompute scrollbar after opened tree leading to target item:
		ComputeScrollbarLength();

		// Count visible items before target:
		int visible_count = 0;
		int parent_level = 0;
		bool parent_open = true;
		for (int i = 0; i <= index; ++i)
		{
			auto& item = items[i];
			if (!parent_open && item.level > parent_level)
			{
				continue;
			}
			visible_count++;
			parent_open = item.open;
			parent_level = item.level;
		}

		// Set scrollbar offset:
		float offset = visible_count * item_height();
		scrollbar.SetOffset(offset);
	}
	void TreeList::FocusOnItemByUserdata(uint64_t userdata)
	{
		for (size_t i = 0; i < items.size(); ++i)
		{
			if (items[i].userdata == userdata)
			{
				FocusOnItem(int(i));
				break;
			}
		}
	}
	const TreeList::Item& TreeList::GetItem(int index) const
	{
		return items[index];
	}
	void TreeList::SetColor(wi::Color color, int id)
	{
		Widget::SetColor(color, id);
		scrollbar.SetColor(color, id);
	}
	void TreeList::SetTheme(const Theme& theme, int id)
	{
		Widget::SetTheme(theme, id);
		scrollbar.SetTheme(theme, id);
	}
}
