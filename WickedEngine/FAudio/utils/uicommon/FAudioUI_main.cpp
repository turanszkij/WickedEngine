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

/* Unless you're trying to do SDL/OpenGL work, you probably don't want this!
 * Go to the other folders to look at the actual tools.
 * -flibit
 */

#include <SDL.h>
#include <SDL_opengl.h>

/* GL Function typedefs */
#define GL_PROC(ret, func, parms) \
	typedef ret (GLAPIENTRY *glfntype_##func) parms;
#include "glfuncs.h"
#undef GL_PROC
/* GL Function declarations */
#define GL_PROC(ret, func, parms) \
	glfntype_##func INTERNAL_##func;
#include "glfuncs.h"
#undef GL_PROC
/* Remap GL function names to internal entry points */
#include "glmacros.h"

/* Defined by the tools using this UI framework */

extern const char* TOOL_NAME;
extern int TOOL_WIDTH;
extern int TOOL_HEIGHT;
extern void FAudioTool_Init();
extern void FAudioTool_Update();
extern void FAudioTool_Quit();

/* FAudioUI_ui.cpp */

extern void UI_Init(
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
);
extern void UI_Quit();
extern uint8_t UI_Update(
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
);
extern void UI_Render();
extern void UI_SubmitKey(
	int key,
	uint8_t down,
	uint8_t shift,
	uint8_t ctrl,
	uint8_t alt,
	uint8_t gui
);
extern void UI_SubmitText(char *text);
extern void UI_SetFontTexture(void* texture);

/* FAudioUI_ui.cpp Callbacks */

void main_setupviewport(int fbw, int fbh, float dw, float dh)
{
	glViewport(0, 0, fbw, fbh);
	glLoadIdentity();
	glOrtho(0.0f, dw, dh, 0.0f, -1.0f, 1.0f);
}

void main_setupvertexattribs(
	int stride,
	const void *vtx,
	const void *txc,
	const void *clr
) {
	glVertexPointer(
		2,
		GL_FLOAT,
		stride,
		vtx
	);
	glTexCoordPointer(
		2,
		GL_FLOAT,
		stride,
		txc
	);
	glColorPointer(
		4,
		GL_UNSIGNED_BYTE,
		stride,
		clr
	);
}

void main_draw(
	void *texture,
	int sx,
	int sy,
	int sw,
	int sh,
	int count,
	int idxSize,
	const void *idx
) {
	glBindTexture(GL_TEXTURE_2D, (GLuint) (intptr_t) texture);
	glScissor(sx, sy, sw, sh);
	glDrawElements(
		GL_TRIANGLES,
		count,
		idxSize == 2 ?
			GL_UNSIGNED_SHORT :
			GL_UNSIGNED_INT,
		idx
	);
}

const char* main_getclipboardtext(void* userdata)
{
	return SDL_GetClipboardText();
}

void main_setclipboardtext(void* userdata, const char *text)
{
	SDL_SetClipboardText(text);
}

/* Entry Point */

int main(int argc, char **argv)
{
	/* Basic stuff */
	SDL_Window *window;
	SDL_GLContext context;
	SDL_Event evt;
	uint8_t run = 1;

	/* ImGui interop */
	SDL_Keymod kmod;
	uint8_t mouseClicked[3];
	int8_t mouseWheel;
	int mx, my;
	uint32_t mouseState;
	int ww, wh, dw, dh;
	Uint32 tCur, tLast = 0;

	/* ImGui texture */
	unsigned char *pixels;
	int tw, th;
	GLuint fontTexture;

	/* Create window/context */
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	window = SDL_CreateWindow(
		TOOL_NAME,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		TOOL_WIDTH,
		TOOL_HEIGHT,
		(
			SDL_WINDOW_OPENGL |
			SDL_WINDOW_RESIZABLE |
			SDL_WINDOW_ALLOW_HIGHDPI
		)
	);
	context = SDL_GL_CreateContext(window);
	SDL_GL_SetSwapInterval(1);

	/* GL function loading */
	#define GL_PROC(ret, func, parms) \
		INTERNAL_##func = (glfntype_##func) SDL_GL_GetProcAddress(#func);
	#include "glfuncs.h"
	#undef GL_PROC

	/* Initialize GL state */
	glClearColor(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);

	/* ImGui setup */
	UI_Init(
		SDLK_TAB,
		SDL_SCANCODE_LEFT,
		SDL_SCANCODE_RIGHT,
		SDL_SCANCODE_UP,
		SDL_SCANCODE_DOWN,
		SDL_SCANCODE_PAGEUP,
		SDL_SCANCODE_PAGEDOWN,
		SDL_SCANCODE_HOME,
		SDL_SCANCODE_END,
		SDLK_DELETE,
		SDLK_BACKSPACE,
		SDLK_RETURN,
		SDLK_ESCAPE,
		SDLK_a,
		SDLK_c,
		SDLK_v,
		SDLK_x,
		SDLK_y,
		SDLK_z,
		&pixels,
		&tw,
		&th
	);
	glGenTextures(1, &fontTexture);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_ALPHA,
		tw,
		th,
		0,
		GL_ALPHA,
		GL_UNSIGNED_BYTE,
		pixels
	);
	UI_SetFontTexture((void*) (intptr_t) fontTexture);

	while (run)
	{
		while (SDL_PollEvent(&evt) == 1)
		{
			if (evt.type == SDL_QUIT)
			{
				run = 0;
			}
			else if (	evt.type == SDL_KEYDOWN ||
					evt.type == SDL_KEYUP	)
			{
				kmod = SDL_GetModState();
				UI_SubmitKey(
					evt.key.keysym.sym & ~SDLK_SCANCODE_MASK,
					evt.type == SDL_KEYDOWN,
					(kmod & KMOD_SHIFT) != 0,
					(kmod & KMOD_CTRL) != 0,
					(kmod & KMOD_ALT) != 0,
					(kmod & KMOD_GUI) != 0
				);
			}
			else if (evt.type == SDL_MOUSEBUTTONDOWN)
			{
				if (evt.button.button < 4)
				{
					mouseClicked[evt.button.button - 1] = 1;
				}
			}
			else if (evt.type == SDL_MOUSEWHEEL)
			{
				if (evt.wheel.y > 0) mouseWheel = 1;
				if (evt.wheel.y < 0) mouseWheel = -1;
			}
			else if (evt.type == SDL_TEXTINPUT)
			{
				UI_SubmitText(evt.text.text);
			}
		}

		/* SDL-related updates */
		SDL_GetWindowSize(window, &ww, &wh);
		SDL_GL_GetDrawableSize(window, &dw, &dh);
		mouseState = SDL_GetMouseState(&mx, &my); /* TODO: Focus */
		mouseClicked[0] |= (mouseState * SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
		mouseClicked[1] |= (mouseState * SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
		mouseClicked[2] |= (mouseState * SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
		tCur = SDL_GetTicks();

		/* Update ImGui input, prep for the new frame */
		SDL_ShowCursor(UI_Update(
			ww, wh, dw, dh,
			mx, my,
			mouseClicked[0],
			mouseClicked[1],
			mouseClicked[2],
			mouseWheel,
			(tCur - tLast) / 1000.0f
		) ? 0 : 1);

		/* Reset some things now that input's updated */
		tLast = tCur;
		mouseClicked[0] = 0;
		mouseClicked[1] = 0;
		mouseClicked[2] = 0;
		mouseWheel = 0;

		/* The actual meat of the audition tool */
		FAudioTool_Update();

		/* Draw, draw, draw! */
		glDisable(GL_SCISSOR_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_SCISSOR_TEST);
		UI_Render();
		SDL_GL_SwapWindow(window);
	}

	/* Clean up if we need to */
	FAudioTool_Quit();

	/* Clean up. We out. */
	UI_Quit();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &fontTexture);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
