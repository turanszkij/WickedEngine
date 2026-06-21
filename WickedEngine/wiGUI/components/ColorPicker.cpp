#include "wiGUI/components/ColorPicker.h"
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
	struct rgb {
		float r;       // a fraction between 0 and 1
		float g;       // a fraction between 0 and 1
		float b;       // a fraction between 0 and 1
	};
	struct hsv {
		float h;       // angle in degrees
		float s;       // a fraction between 0 and 1
		float v;       // a fraction between 0 and 1
	};
	hsv rgb2hsv(rgb in)
	{
		hsv         out;
		float		min, max, delta;

		min = in.r < in.g ? in.r : in.g;
		min = min < in.b ? min : in.b;

		max = in.r > in.g ? in.r : in.g;
		max = max > in.b ? max : in.b;

		out.v = max;                                // v
		delta = max - min;
		if (delta < 0.00001f)
		{
			out.s = 0;
			out.h = 0; // undefined, maybe nan?
			return out;
		}
		if (max > 0.0f) { // NOTE: if Max is == 0, this divide would cause a crash
			out.s = (delta / max);                  // s
		}
		else {
			// if max is 0, then r = g = b = 0
			// s = 0, h is undefined
			out.s = 0.0f;
			out.h = NAN;                            // its now undefined
			return out;
		}
		if (in.r >= max)                           // > is bogus, just keeps compilor happy
			out.h = (in.g - in.b) / delta;        // between yellow & magenta
		else
			if (in.g >= max)
				out.h = 2.0f + (in.b - in.r) / delta;  // between cyan & yellow
			else
				out.h = 4.0f + (in.r - in.g) / delta;  // between magenta & cyan

		out.h *= 60.0f;                              // degrees

		if (out.h < 0.0f)
			out.h += 360.0f;

		return out;
	}
	rgb hsv2rgb(hsv in)
	{
		float		hh, p, q, t, ff;
		long        i;
		rgb         out;

		if (in.s <= 0.0f) {       // < is bogus, just shuts up warnings
			out.r = in.v;
			out.g = in.v;
			out.b = in.v;
			return out;
		}
		hh = in.h;
		if (hh >= 360.0f) hh = 0.0f;
		hh /= 60.0f;
		i = (long)hh;
		ff = hh - i;
		p = in.v * (1.0f - in.s);
		q = in.v * (1.0f - (in.s * ff));
		t = in.v * (1.0f - (in.s * (1.0f - ff)));

		switch (i) {
		case 0:
			out.r = in.v;
			out.g = t;
			out.b = p;
			break;
		case 1:
			out.r = q;
			out.g = in.v;
			out.b = p;
			break;
		case 2:
			out.r = p;
			out.g = in.v;
			out.b = t;
			break;

		case 3:
			out.r = p;
			out.g = q;
			out.b = in.v;
			break;
		case 4:
			out.r = t;
			out.g = p;
			out.b = in.v;
			break;
		case 5:
		default:
			out.r = in.v;
			out.g = p;
			out.b = q;
			break;
		}
		return out;
	}

	static const float cp_width = 300;
	static const float cp_height = 260;
	void ColorPicker::Create(const std::string& name, WindowControls window_controls)
	{
		Window::Create(name, window_controls);

		SetSize(XMFLOAT2(cp_width, cp_height));
		SetColor(wi::Color(100, 100, 100, 100));

		float x = 250;
		float y = 80;
		float step = 20;

		text_R.Create("R");
		text_R.SetLocalizationEnabled(LocalizationEnabled::Tooltip);
		text_R.SetPos(XMFLOAT2(x, y += step));
		text_R.SetSize(XMFLOAT2(40, 18));
		text_R.SetText("");
		text_R.SetTooltip("Enter value for RED channel (0-255)");
		text_R.SetDescription("R: ");
		text_R.OnInputAccepted([this](const EventArgs& args) {
			wi::Color color = GetPickColor();
			color.setR((uint8_t)args.iValue);
			SetPickColor(color);
			FireEvents();
			});
		AddWidget(&text_R);

		text_G.Create("G");
		text_G.SetLocalizationEnabled(LocalizationEnabled::Tooltip);
		text_G.SetPos(XMFLOAT2(x, y += step));
		text_G.SetSize(XMFLOAT2(40, 18));
		text_G.SetText("");
		text_G.SetTooltip("Enter value for GREEN channel (0-255)");
		text_G.SetDescription("G: ");
		text_G.OnInputAccepted([this](const EventArgs& args) {
			wi::Color color = GetPickColor();
			color.setG((uint8_t)args.iValue);
			SetPickColor(color);
			FireEvents();
			});
		AddWidget(&text_G);

		text_B.Create("B");
		text_B.SetLocalizationEnabled(LocalizationEnabled::Tooltip);
		text_B.SetPos(XMFLOAT2(x, y += step));
		text_B.SetSize(XMFLOAT2(40, 18));
		text_B.SetText("");
		text_B.SetTooltip("Enter value for BLUE channel (0-255)");
		text_B.SetDescription("B: ");
		text_B.OnInputAccepted([this](const EventArgs& args) {
			wi::Color color = GetPickColor();
			color.setB((uint8_t)args.iValue);
			SetPickColor(color);
			FireEvents();
			});
		AddWidget(&text_B);


		text_H.Create("H");
		text_H.SetLocalizationEnabled(LocalizationEnabled::Tooltip);
		text_H.SetPos(XMFLOAT2(x, y += step));
		text_H.SetSize(XMFLOAT2(40, 18));
		text_H.SetText("");
		text_H.SetTooltip("Enter value for HUE channel (0-360)");
		text_H.SetDescription("H: ");
		text_H.OnInputAccepted([this](const EventArgs& args) {
			hue = wi::math::Clamp(args.fValue, 0, 360.0f);
			FireEvents();
			});
		AddWidget(&text_H);

		text_S.Create("S");
		text_S.SetLocalizationEnabled(LocalizationEnabled::Tooltip);
		text_S.SetPos(XMFLOAT2(x, y += step));
		text_S.SetSize(XMFLOAT2(40, 18));
		text_S.SetText("");
		text_S.SetTooltip("Enter value for SATURATION channel (0-100)");
		text_S.SetDescription("S: ");
		text_S.OnInputAccepted([this](const EventArgs& args) {
			saturation = wi::math::Clamp(args.fValue / 100.0f, 0, 1);
			FireEvents();
			});
		AddWidget(&text_S);

		text_V.Create("V");
		text_V.SetLocalizationEnabled(LocalizationEnabled::Tooltip);
		text_V.SetPos(XMFLOAT2(x, y += step));
		text_V.SetSize(XMFLOAT2(40, 18));
		text_V.SetText("");
		text_V.SetTooltip("Enter value for LUMINANCE channel (0-100)");
		text_V.SetDescription("V: ");
		text_V.OnInputAccepted([this](const EventArgs& args) {
			luminance = wi::math::Clamp(args.fValue / 100.0f, 0, 1);
			FireEvents();
			});
		AddWidget(&text_V);

		text_hex.Create("Hex");
		text_hex.SetLocalizationEnabled(LocalizationEnabled::Tooltip);
		text_hex.SetPos(XMFLOAT2(x, y += step));
		text_hex.SetSize(XMFLOAT2(80, 18));
		text_hex.SetText("");
		text_hex.SetTooltip("Enter RGBA hex value");
		text_hex.SetDescription("#");
		text_hex.font_description.params.scaling = 1.2f;
		text_hex.OnInputAccepted([this](const EventArgs& args) {
			wi::Color color(args.sValue.c_str());
			SetPickColor(color);
			FireEvents();
			});
		AddWidget(&text_hex);

		alphaSlider.Create(0, 255, 255, 255, "");
		alphaSlider.SetLocalizationEnabled(LocalizationEnabled::Tooltip);
		alphaSlider.SetPos(XMFLOAT2(20, y));
		alphaSlider.SetSize(XMFLOAT2(150, 18));
		alphaSlider.SetText("A: ");
		alphaSlider.SetTooltip("Value for ALPHA - TRANSPARENCY channel (0-255)");
		alphaSlider.OnSlide([this](const EventArgs& args) {
			FireEvents();
			});
		AddWidget(&alphaSlider);
	}
	static const float colorpicker_radius_triangle = 68;
	static const float colorpicker_radius = 75;
	static const float colorpicker_width = 22;
	void ColorPicker::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Window::Update(canvas, dt);

		if (IsEnabled() && dt > 0)
		{

			if (state == DEACTIVATING)
			{
				state = IDLE;
			}

			float sca = std::min(scale.x / cp_width, scale.y / cp_height);

			XMMATRIX W =
				XMMatrixScaling(sca, sca, 1) *
				XMMatrixTranslation(translation.x + scale.x * 0.4f, translation.y + scale.y * 0.5f, 0)
				;

			XMFLOAT2 center = XMFLOAT2(translation.x + scale.x * 0.4f, translation.y + scale.y * 0.5f);
			XMFLOAT2 pointer = GetPointerHitbox().pos;
			float distance = wi::math::Distance(center, pointer);
			bool hover_hue = (distance > colorpicker_radius * sca) && (distance < (colorpicker_radius + colorpicker_width)* sca);

			float distTri = 0;
			XMFLOAT4 A, B, C;
			wi::math::ConstructTriangleEquilateral(colorpicker_radius_triangle, A, B, C);
			XMVECTOR _A = XMLoadFloat4(&A);
			XMVECTOR _B = XMLoadFloat4(&B);
			XMVECTOR _C = XMLoadFloat4(&C);
			XMMATRIX _triTransform = XMMatrixRotationZ(-hue / 360.0f * XM_2PI) * W;
			_A = XMVector4Transform(_A, _triTransform);
			_B = XMVector4Transform(_B, _triTransform);
			_C = XMVector4Transform(_C, _triTransform);
			XMVECTOR O = XMVectorSet(pointer.x, pointer.y, 0, 0);
			XMVECTOR D = XMVectorSet(0, 0, 1, 0);
			bool hover_saturation = TriangleTests::Intersects(O, D, _A, _B, _C, distTri);

			bool dragged = false;

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				if (hover_hue)
				{
					colorpickerstate = CPS_HUE;
					dragged = true;
				}
				else if (hover_saturation)
				{
					colorpickerstate = CPS_SATURATION;
					dragged = true;
				}
			}

			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				if (colorpickerstate > CPS_IDLE)
				{
					// continue drag if already grabbed whether it is intersecting or not
					dragged = true;
				}
			}
			else
			{
				colorpickerstate = CPS_IDLE;
			}

			if (colorpickerstate == CPS_HUE && dragged)
			{
				//hue pick
				const float angle = wi::math::GetAngle(XMFLOAT2(pointer.x - center.x, pointer.y - center.y), XMFLOAT2(colorpicker_radius, 0));
				hue = angle / XM_2PI * 360.0f;
				Activate();
			}
			else if (colorpickerstate == CPS_SATURATION && dragged)
			{
				// saturation pick
				float u, v, w;
				wi::math::GetBarycentric(O, _A, _B, _C, u, v, w, true);
				// u = saturated corner (color)
				// v = desaturated corner (white)
				// w = no luminosity corner (black)

				hsv source;
				source.h = hue;
				source.s = 1;
				source.v = 1;
				rgb result = hsv2rgb(source);

				XMVECTOR color_corner = XMVectorSet(result.r, result.g, result.b, 1);
				XMVECTOR white_corner = XMVectorSet(1, 1, 1, 1);
				XMVECTOR black_corner = XMVectorSet(0, 0, 0, 1);
				XMVECTOR inner_point = u * color_corner + v * white_corner + w * black_corner;

				result.r = XMVectorGetX(inner_point);
				result.g = XMVectorGetY(inner_point);
				result.b = XMVectorGetZ(inner_point);
				source = rgb2hsv(result);

				saturation = source.s;
				luminance = source.v;

				Activate();
			}
			else if (state != IDLE)
			{
				Deactivate();
			}

			wi::Color color = GetPickColor();
			text_R.SetValue((int)color.getR());
			text_G.SetValue((int)color.getG());
			text_B.SetValue((int)color.getB());
			text_H.SetValue(int(hue));
			text_S.SetValue(int(saturation * 100));
			text_V.SetValue(int(luminance * 100));
			text_hex.SetText(color.to_hex());

			if (dragged)
			{
				FireEvents();
			}
		}
	}
	void ColorPicker::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		Window::Render(canvas, cmd);

		if (!IsVisible() || IsMinimized())
		{
			return;
		}

		GraphicsDevice* device = wi::graphics::GetDevice();

		struct Vertex
		{
			XMFLOAT4 pos;
			XMFLOAT4 col;
		};
		static wi::graphics::GPUBuffer vb_hue;
		static wi::graphics::GPUBuffer vb_picker_saturation;
		static wi::graphics::GPUBuffer vb_picker_hue;
		static wi::graphics::GPUBuffer vb_preview;

		static wi::vector<Vertex> vertices_saturation;

		static bool buffersComplete = false;
		if (!buffersComplete)
		{
			buffersComplete = true;

			// saturation
			{
				vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,0,0,1) });	// hue
				vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,1,1,1) });	// white
				vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(0,0,0,1) });	// black
				wi::math::ConstructTriangleEquilateral(colorpicker_radius_triangle, vertices_saturation[0].pos, vertices_saturation[1].pos, vertices_saturation[2].pos);

				// create alpha blended edge:
				vertices_saturation.push_back(vertices_saturation[0]); // outer
				vertices_saturation.push_back(vertices_saturation[0]); // inner
				vertices_saturation.push_back(vertices_saturation[1]); // outer
				vertices_saturation.push_back(vertices_saturation[1]); // inner
				vertices_saturation.push_back(vertices_saturation[2]); // outer
				vertices_saturation.push_back(vertices_saturation[2]); // inner
				vertices_saturation.push_back(vertices_saturation[0]); // outer
				vertices_saturation.push_back(vertices_saturation[0]); // inner
				wi::math::ConstructTriangleEquilateral(colorpicker_radius_triangle + 4, vertices_saturation[3].pos, vertices_saturation[5].pos, vertices_saturation[7].pos); // extrude outer
				vertices_saturation[9].pos = vertices_saturation[3].pos; // last outer
			}
			// hue
			{
				const float edge = 2.0f;
				wi::vector<Vertex> vertices;
				uint32_t segment_count = 100;
				// inner alpha blended edge
				for (uint32_t i = 0; i <= segment_count; ++i)
				{
					float p = float(i) / segment_count;
					float t = p * XM_2PI;
					float x = cos(t);
					float y = -sin(t);
					hsv source;
					source.h = p * 360.0f;
					source.s = 1;
					source.v = 1;
					rgb result = hsv2rgb(source);
					XMFLOAT4 color = XMFLOAT4(result.r, result.g, result.b, 1);
					XMFLOAT4 coloralpha = XMFLOAT4(result.r, result.g, result.b, 0);
					vertices.push_back({ XMFLOAT4((colorpicker_radius - edge) * x, (colorpicker_radius - edge) * y, 0, 1), coloralpha });
					vertices.push_back({ XMFLOAT4(colorpicker_radius * x, colorpicker_radius * y, 0, 1), color });
				}
				// middle hue
				for (uint32_t i = 0; i <= segment_count; ++i)
				{
					float p = float(i) / segment_count;
					float t = p * XM_2PI;
					float x = cos(t);
					float y = -sin(t);
					hsv source;
					source.h = p * 360.0f;
					source.s = 1;
					source.v = 1;
					rgb result = hsv2rgb(source);
					XMFLOAT4 color = XMFLOAT4(result.r, result.g, result.b, 1);
					vertices.push_back({ XMFLOAT4(colorpicker_radius * x, colorpicker_radius * y, 0, 1), color });
					vertices.push_back({ XMFLOAT4((colorpicker_radius + colorpicker_width) * x, (colorpicker_radius + colorpicker_width) * y, 0, 1), color });
				}
				// outer alpha blended edge
				for (uint32_t i = 0; i <= segment_count; ++i)
				{
					float p = float(i) / segment_count;
					float t = p * XM_2PI;
					float x = cos(t);
					float y = -sin(t);
					hsv source;
					source.h = p * 360.0f;
					source.s = 1;
					source.v = 1;
					rgb result = hsv2rgb(source);
					XMFLOAT4 color = XMFLOAT4(result.r, result.g, result.b, 1);
					XMFLOAT4 coloralpha = XMFLOAT4(result.r, result.g, result.b, 0);
					vertices.push_back({ XMFLOAT4((colorpicker_radius + colorpicker_width) * x, (colorpicker_radius + colorpicker_width) * y, 0, 1), color });
					vertices.push_back({ XMFLOAT4((colorpicker_radius + colorpicker_width + edge) * x, (colorpicker_radius + colorpicker_width + edge) * y, 0, 1), coloralpha });
				}

				GPUBufferDesc desc;
				desc.bind_flags = BindFlag::VERTEX_BUFFER;
				desc.size = vertices.size() * sizeof(Vertex);
				desc.stride = 0;
				device->CreateBuffer(&desc, vertices.data(), &vb_hue);
			}
			// saturation picker (small circle)
			{
				float _radius = 3;
				float _width = 3;
				wi::vector<Vertex> vertices;
				uint32_t segment_count = 100;
				for (uint32_t i = 0; i <= segment_count; ++i)
				{
					float p = float(i) / 100;
					float t = p * XM_2PI;
					float x = cos(t);
					float y = -sin(t);
					vertices.push_back({ XMFLOAT4(_radius * x, _radius * y, 0, 1), XMFLOAT4(1,1,1,1) });
					vertices.push_back({ XMFLOAT4((_radius + _width) * x, (_radius + _width) * y, 0, 1), XMFLOAT4(1,1,1,1) });
				}

				GPUBufferDesc desc;
				desc.bind_flags = BindFlag::VERTEX_BUFFER;
				desc.size = vertices.size() * sizeof(Vertex);
				desc.stride = 0;
				device->CreateBuffer(&desc, vertices.data(), &vb_picker_saturation);
			}
			// hue picker (rectangle)
			{
				float boldness = 4.0f;
				float halfheight = 8.0f;
				Vertex vertices[] = {
					// left side:
					{ XMFLOAT4(colorpicker_radius - boldness, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius - boldness, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },

					// bottom side:
					{ XMFLOAT4(colorpicker_radius - boldness, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius - boldness, halfheight - boldness, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, halfheight - boldness, 0, 1),XMFLOAT4(1,1,1,1) },

					// right side:
					{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius + colorpicker_width, halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius + colorpicker_width, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },

					// top side:
					{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius + colorpicker_width + boldness, -halfheight + boldness, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius - boldness, -halfheight, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(colorpicker_radius - boldness, -halfheight + boldness, 0, 1),XMFLOAT4(1,1,1,1) },
				};

				GPUBufferDesc desc;
				desc.bind_flags = BindFlag::VERTEX_BUFFER;
				desc.size = sizeof(vertices);
				desc.stride = 0;
				device->CreateBuffer(&desc, vertices, &vb_picker_hue);
			}
			// preview
			{
				float _width = 50;
				Vertex vertices[] = {
					{ XMFLOAT4(-_width, _width, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(0, _width, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(-_width, 0, 0, 1),XMFLOAT4(1,1,1,1) },
					{ XMFLOAT4(0, 0, 0, 1),XMFLOAT4(1,1,1,1) },
				};

				GPUBufferDesc desc;
				desc.bind_flags = BindFlag::VERTEX_BUFFER;
				desc.size = sizeof(vertices);
				device->CreateBuffer(&desc, vertices, &vb_preview);
			}

		}

		const wi::Color final_color = GetPickColor();
		const float angle = hue / 360.0f * XM_2PI;

		const XMMATRIX Projection = canvas.GetProjection();

		device->BindPipelineState(&gui_internal().PSO_colored, cmd);

		ApplyScissor(canvas, scissorRect, cmd);

		float sca = std::min(scale.x / cp_width, scale.y / cp_height);

		XMMATRIX W =
			XMMatrixScaling(sca, sca, 1) *
			XMMatrixTranslation(translation.x + scale.x * 0.4f, translation.y + scale.y * 0.5f, 0)
			;

		MiscCB cb;

		// render saturation triangle
		{
			hsv source;
			source.h = hue;
			source.s = 1;
			source.v = 1;
			rgb result = hsv2rgb(source);
			vertices_saturation[0].col = XMFLOAT4(result.r, result.g, result.b, 1);

			vertices_saturation[3].col = vertices_saturation[0].col; vertices_saturation[3].col.w = 0;
			vertices_saturation[4].col = vertices_saturation[0].col;
			vertices_saturation[5].col = vertices_saturation[1].col; vertices_saturation[5].col.w = 0;
			vertices_saturation[6].col = vertices_saturation[1].col;
			vertices_saturation[7].col = vertices_saturation[2].col; vertices_saturation[7].col.w = 0;
			vertices_saturation[8].col = vertices_saturation[2].col;
			vertices_saturation[9].col = vertices_saturation[0].col; vertices_saturation[9].col.w = 0;
			vertices_saturation[10].col = vertices_saturation[0].col;

			size_t alloc_size = sizeof(Vertex) * vertices_saturation.size();
			GraphicsDevice::GPUAllocation vb_saturation = device->AllocateGPU(alloc_size, cmd);
			memcpy(vb_saturation.data, vertices_saturation.data(), alloc_size);

			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixRotationZ(-angle) *
				W *
				Projection
			);
			cb.g_xColor = IsEnabled() ? float4(1, 1, 1, 1) : float4(0.5f, 0.5f, 0.5f, 1);
			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_saturation.buffer,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			const uint64_t offsets[] = {
				vb_saturation.offset,
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
			device->Draw((uint32_t)vertices_saturation.size(), 0, cmd);
		}

		// render hue circle
		{
			XMStoreFloat4x4(&cb.g_xTransform,
				W *
				Projection
			);
			cb.g_xColor = IsEnabled() ? float4(1, 1, 1, 1) : float4(0.5f, 0.5f, 0.5f, 1);
			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_hue,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
			device->Draw((uint32_t)(vb_hue.GetDesc().size / sizeof(Vertex)), 0, cmd);
		}

		// render hue picker
		if (IsEnabled())
		{
			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixRotationZ(-hue / 360.0f * XM_2PI) *
				W *
				Projection
			);

			hsv source;
			source.h = hue;
			source.s = 1;
			source.v = 1;
			rgb result = hsv2rgb(source);
			cb.g_xColor = float4(1 - result.r, 1 - result.g, 1 - result.b, 1);

			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_picker_hue,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
			device->Draw((uint32_t)(vb_picker_hue.GetDesc().size / sizeof(Vertex)), 0, cmd);
		}

		// render saturation picker
		if (IsEnabled())
		{
			XMFLOAT4 A, B, C;
			wi::math::ConstructTriangleEquilateral(colorpicker_radius_triangle, A, B, C);
			XMVECTOR _A = XMLoadFloat4(&A);
			XMVECTOR _B = XMLoadFloat4(&B);
			XMVECTOR _C = XMLoadFloat4(&C);
			XMMATRIX _triTransform = XMMatrixRotationZ(-hue / 360.0f * XM_2PI);
			_A = XMVector4Transform(_A, _triTransform);
			_B = XMVector4Transform(_B, _triTransform);
			_C = XMVector4Transform(_C, _triTransform);

			hsv source;
			source.h = hue;
			source.s = 1;
			source.v = 1;
			rgb result = hsv2rgb(source);

			XMVECTOR color_corner = XMVectorSet(result.r, result.g, result.b, 1);
			XMVECTOR white_corner = XMVectorSet(1, 1, 1, 1);
			XMVECTOR black_corner = XMVectorSet(0, 0, 0, 1);

			source.h = hue;
			source.s = saturation;
			source.v = luminance;
			result = hsv2rgb(source);
			XMVECTOR inner_point = XMVectorSet(result.r, result.g, result.b, 1);

			float u, v, w;
			wi::math::GetBarycentric(inner_point, color_corner, white_corner, black_corner, u, v, w, true);

			XMVECTOR picker_center = u * _A + v * _B + w * _C;

			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixTranslationFromVector(picker_center) *
				W *
				Projection
			);
			cb.g_xColor = float4(1 - final_color.toFloat3().x, 1 - final_color.toFloat3().y, 1 - final_color.toFloat3().z, 1);
			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_picker_saturation,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
			device->Draw((uint32_t)(vb_picker_saturation.GetDesc().size / sizeof(Vertex)), 0, cmd);
		}

		// render preview
		{
			XMStoreFloat4x4(&cb.g_xTransform,
				XMMatrixTranslation(translation.x + scale.x - sca - 2, translation.y + control_size + 4 + 22, 0) *
				Projection
			);
			cb.g_xColor = final_color.toFloat4();
			device->BindDynamicConstantBuffer(cb, CBSLOT_RENDERER_MISC, cmd);
			const GPUBuffer* vbs[] = {
				&vb_preview,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
			device->Draw((uint32_t)(vb_preview.GetDesc().size / sizeof(Vertex)), 0, cmd);
		}
	}
	void ColorPicker::ResizeLayout()
	{
		wi::gui::Window::ResizeLayout();
		const float padding = 4;
		const float width = GetWidgetAreaSize().x;
		float y = GetWidgetAreaSize().y - control_size;
		float jump = 20;

		auto add = [&](wi::gui::Widget& widget) {
			if (!widget.IsVisible())
				return;
			const float margin_left = 20;
			const float margin_right = 80;
			y -= widget.GetSize().y;
			y -= padding;
			widget.SetPos(XMFLOAT2(margin_left, y));
			widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetSize().y));
			};
		auto add_right = [&](wi::gui::Widget& widget) {
			if (!widget.IsVisible())
				return;
			const float margin_right = padding;
			y -= widget.GetSize().y;
			y -= padding;
			widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
			};

		add(alphaSlider);
		y += alphaSlider.GetSize().y;
		y += padding;

		add_right(text_V);
		add_right(text_S);
		add_right(text_H);

		y -= jump;

		add_right(text_B);
		add_right(text_G);
		add_right(text_R);

		y = control_size + 4;
		add_right(text_hex);
	}
	void ColorPicker::SetColor(wi::Color color, int id)
	{
		Window::SetColor(color, id);

		if (id == WIDGET_ID_COLORPICKER_BASE)
		{
			sprites[IDLE].params.color = color;
		}
	}
	void ColorPicker::SetImage(wi::Resource resource, int id)
	{
		Window::SetImage(resource, id);

		if (id == WIDGET_ID_COLORPICKER_BASE)
		{
			sprites[IDLE].textureResource = resource;
		}
	}
	wi::Color ColorPicker::GetPickColor() const
	{
		hsv source;
		source.h = hue;
		source.s = saturation;
		source.v = luminance;
		rgb result = hsv2rgb(source);
		return wi::Color::fromFloat4(XMFLOAT4(result.r, result.g, result.b, alphaSlider.GetValue() / 255.0f));
	}
	void ColorPicker::SetPickColor(wi::Color value)
	{
		if (colorpickerstate != CPS_IDLE)
		{
			// Only allow setting the pick color when picking is not active, the RGB and HSV precision combat each other
			return;
		}

		rgb source;
		source.r = value.toFloat3().x;
		source.g = value.toFloat3().y;
		source.b = value.toFloat3().z;
		hsv result = rgb2hsv(source);
		if (value.getR() != value.getG() || value.getG() != value.getB() || value.getR() != value.getB())
		{
			hue = result.h; // only change the hue when not pure greyscale because those don't retain the saturation value
		}
		saturation = result.s;
		luminance = result.v;
		alphaSlider.SetValue((float)value.getA());
	}
	void ColorPicker::FireEvents()
	{
		if (onColorChanged == nullptr)
			return;
		EventArgs args = {};
		args.color = GetPickColor();
		onColorChanged(args);
	}
	void ColorPicker::OnColorChanged(std::function<void(const EventArgs& args)> func)
	{
		onColorChanged = std::move(func);
	}
}
