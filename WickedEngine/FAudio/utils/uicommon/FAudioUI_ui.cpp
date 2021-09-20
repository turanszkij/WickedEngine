/* FAudio - XAudio Reimplementation for FNA
 *
 * Copyright (c) 2011-2021 Ethan Lee, Luigi Auriemma, and the MonoGame Team
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Ethan "flibitijibibo" Lee <flibitijibibo@flibitijibibo.com>
 *
 */

/* Unless you're trying to do ImGui interop work, you probably don't want this!
 * Go to the other folders to look at the actual tools.
 * -flibit
 */

#include <stdint.h>
#include "imgui.h"

/* FAudioUI_main.cpp */

extern const char* main_getclipboardtext(void* userdata);
extern void main_setclipboardtext(void* userdata, const char *text);
extern void main_setupviewport(int fbw, int fbh, float dw, float dh);
extern void main_setupvertexattribs(
	int stride,
	const void *vtx,
	const void *txc,
	const void *clr
);
extern void main_draw(
	void *texture,
	int sx,
	int sy,
	int sw,
	int sh,
	int count,
	int idxSize,
	const void *idx
);

/* ImGui Callbacks */

void UI_RenderDrawLists(ImDrawData *draw_data)
{
	ImGuiIO& io = ImGui::GetIO();

	/* Set up viewport/scissor rects (based on display size/scale */
	int fb_width = (int) (io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int) (io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fb_width == 0 || fb_height == 0)
	{
		/* No point in rendering to nowhere... */
		return;
	}
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);
	main_setupviewport(
		fb_width,
		fb_height,
		io.DisplaySize.x,
		io.DisplaySize.y
	);

	/* Submit draw commands */
	#define OFFSETOF(TYPE, ELEMENT) ((size_t) &(((TYPE*) NULL)->ELEMENT))
	for (int cmd_l = 0; cmd_l < draw_data->CmdListsCount; cmd_l += 1)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[cmd_l];
		const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
		const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;

		main_setupvertexattribs(
			sizeof(ImDrawVert),
			((const char*) vtx_buffer + OFFSETOF(ImDrawVert, pos)),
			((const char*) vtx_buffer + OFFSETOF(ImDrawVert, uv)),
			((const char*) vtx_buffer + OFFSETOF(ImDrawVert, col))
		);

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i += 1)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			main_draw(
				pcmd->TextureId,
				(int) pcmd->ClipRect.x,
				(int) (fb_height - pcmd->ClipRect.w),
				(int) (pcmd->ClipRect.z - pcmd->ClipRect.x),
				(int) (pcmd->ClipRect.w - pcmd->ClipRect.y),
				pcmd->ElemCount,
				sizeof(ImDrawIdx),
				idx_buffer
			);
			idx_buffer += pcmd->ElemCount;
		}
	}
	#undef OFFSETOF
}

/* Public API */

static ImGuiContext *imContext = NULL;

void UI_Init(
	int tab,
	int left,
	int right,
	int up,
	int down,
	int pgup,
	int pgdown,
	int home,
	int end,
	int del,
	int backspace,
	int enter,
	int escape,
	int a,
	int c,
	int v,
	int x,
	int y,
	int z,
	unsigned char **pixels,
	int *tw,
	int *th
) {
	imContext = ImGui::CreateContext(NULL);
	ImGui::SetCurrentContext(imContext);

	ImGuiIO& io = ImGui::GetIO();

	/* Keyboard */
	io.KeyMap[ImGuiKey_Tab] = tab;
	io.KeyMap[ImGuiKey_LeftArrow] = left;
	io.KeyMap[ImGuiKey_RightArrow] = right;
	io.KeyMap[ImGuiKey_UpArrow] = up;
	io.KeyMap[ImGuiKey_DownArrow] = down;
	io.KeyMap[ImGuiKey_PageUp] = pgup;
	io.KeyMap[ImGuiKey_PageDown] = pgdown;
	io.KeyMap[ImGuiKey_Home] = home;
	io.KeyMap[ImGuiKey_End] = end;
	io.KeyMap[ImGuiKey_Delete] = del;
	io.KeyMap[ImGuiKey_Backspace] = backspace;
	io.KeyMap[ImGuiKey_Enter] = enter;
	io.KeyMap[ImGuiKey_Escape] = escape;
	io.KeyMap[ImGuiKey_A] = a;
	io.KeyMap[ImGuiKey_C] = c;
	io.KeyMap[ImGuiKey_V] = v;
	io.KeyMap[ImGuiKey_X] = x;
	io.KeyMap[ImGuiKey_Y] = y;
	io.KeyMap[ImGuiKey_Z] = z;

	/* Callbacks */
	io.RenderDrawListsFn = UI_RenderDrawLists;
	io.GetClipboardTextFn = main_getclipboardtext;
	io.SetClipboardTextFn = main_setclipboardtext;

	/* Create texture for text rendering */
	io.Fonts->GetTexDataAsAlpha8(pixels, tw, th);
}

uint8_t UI_Update(
	int ww,
	int wh,
	int dw,
	int dh,
	int mx,
	int my,
	uint8_t mouse1,
	uint8_t mouse2,
	uint8_t mouse3,
	int8_t wheel,
	float deltaTime
) {
	ImGuiIO& io = ImGui::GetIO();

	/* Set these every frame, we have a resizable window! */
	io.DisplaySize = ImVec2((float) ww, (float) wh);
	io.DisplayFramebufferScale = ImVec2(
		ww > 0 ? ((float) dw / ww) : 0,
		wh > 0 ? ((float) dh / wh) : 0
	);

	/* Time update */
	io.DeltaTime = deltaTime;
	if (io.DeltaTime == 0.0f)
	{
		io.DeltaTime = 0.01f;
	}

	/* Input updates not done via UI_Submit*() */
	io.MousePos = ImVec2((float) mx, (float) my);
	io.MouseDown[0] = mouse1;
	io.MouseDown[1] = mouse2;
	io.MouseDown[2] = mouse3;
	io.MouseWheel = wheel;

	/* BEGIN */
	ImGui::NewFrame();
	return io.MouseDrawCursor;
}

void UI_Quit()
{
	ImGui::DestroyContext(imContext);
}

void UI_Render()
{
	ImGui::Render();
}

void UI_SubmitKey(
	int key,
	uint8_t down,
	uint8_t shift,
	uint8_t ctrl,
	uint8_t alt,
	uint8_t gui
) {
	ImGuiIO& io = ImGui::GetIO();
	io.KeysDown[key] = down;
	io.KeyShift = shift;
	io.KeyCtrl = ctrl;
	io.KeyAlt = alt;
	io.KeySuper = gui;
}

void UI_SubmitText(char *text)
{
	ImGui::GetIO().AddInputCharactersUTF8(text);
}

void UI_SetFontTexture(void *texture)
{
	ImGui::GetIO().Fonts->TexID = texture;
}
