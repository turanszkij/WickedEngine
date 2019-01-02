#include "stdafx.h"
#include "Tests.h"

#include <string>
#include <sstream>
#include <fstream>

using namespace wiSceneSystem;

Tests::Tests()
{
}
Tests::~Tests()
{
}

void Tests::Initialize()
{
	// Call this before Maincomponent::Initialize if you want to load shaders from an other directory!
	// otherwise, shaders will be loaded from the working directory
	wiRenderer::GetShaderPath() = "../WickedEngine/shaders/";
	wiFont::GetFontPath() = "../WickedEngine/fonts/"; // search for fonts elsewhere
	MainComponent::Initialize();

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.resolution = true;

	ActivatePath(new TestsRenderer);
}


TestsRenderer::TestsRenderer()
{
	setSSREnabled(false);
	setSSAOEnabled(false);
	setReflectionsEnabled(true);
	setFXAAEnabled(false);

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	wiLabel* label = new wiLabel("Label1");
	label->SetText("Wicked Engine Test Framework");
	label->SetSize(XMFLOAT2(200,15));
	label->SetPos(XMFLOAT2(screenW / 2.f - label->scale.x / 2.f, screenH*0.95f));
	GetGUI().AddWidget(label);


	wiButton* audioTest = new wiButton("AudioTest");
	audioTest->SetText("Play Test Audio");
	audioTest->SetSize(XMFLOAT2(200, 20));
	audioTest->SetPos(XMFLOAT2(10, 80));
	audioTest->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	audioTest->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	audioTest->OnClick([=](wiEventArgs args) {
		static wiMusic music("sound/music.wav");
		static bool playing = false;

		if (playing)
		{
			music.Stop();
			audioTest->SetText("Play Test Audio");
		}
		else
		{
			music.Play();
			audioTest->SetText("Stop Test Audio");
		}

		playing = !playing;
	});
	GetGUI().AddWidget(audioTest);


	wiSlider* volume = new wiSlider(0, 100, 50, 100, "Volume");
	volume->SetText("Volume: ");
	volume->SetSize(XMFLOAT2(95, 20));
	volume->SetPos(XMFLOAT2(65, 110));
	volume->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	volume->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	volume->OnSlide([](wiEventArgs args) {
		wiMusic::SetVolume(args.fValue / 100.0f);
	});
	GetGUI().AddWidget(volume);


	wiComboBox* testSelector = new wiComboBox("TestSelector");
	testSelector->SetText("Demo: ");
	testSelector->SetSize(XMFLOAT2(140, 20));
	testSelector->SetPos(XMFLOAT2(50, 140));
	testSelector->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	testSelector->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	testSelector->AddItem("HelloWorld");
	testSelector->AddItem("Model");
	testSelector->AddItem("EmittedParticle 1");
	testSelector->AddItem("EmittedParticle 2");
	testSelector->AddItem("HairParticle");
	testSelector->AddItem("Lua Script");
	testSelector->AddItem("Water Test");
	testSelector->AddItem("Shadows Test");
	testSelector->AddItem("Physics Test");
	testSelector->AddItem("Cloth Physics Test");
	testSelector->AddItem("Job System Test");
	testSelector->AddItem("Font Test");
	testSelector->AddItem("Volumetric Test");
	testSelector->AddItem("Sprite Test");
	testSelector->AddItem("Lightmap Bake Test");
	testSelector->SetMaxVisibleItemCount(100);
	testSelector->OnSelect([=](wiEventArgs args) {

		// Reset all state that tests might have modified:
		wiRenderer::GetDevice()->SetVSyncEnabled(true);
		wiRenderer::SetToDrawGridHelper(false);
		wiRenderer::SetTemporalAAEnabled(false);
		wiRenderer::ClearWorld();
		wiRenderer::GetScene().weather = WeatherComponent();
		this->clearSprites();
		this->clearFonts();
		wiLua::GetGlobal()->KillProcesses();

		// Reset camera position:
		TransformComponent transform;
		transform.Translate(XMFLOAT3(0, 2.f, -4.5f));
		transform.UpdateTransform();
		wiRenderer::GetCamera().TransformCamera(transform);

		// Based on combobox selection, start the appropriate test:
		switch (args.iValue)
		{
		case 0:
		{
			static wiSprite sprite;
			sprite = wiSprite("images/HelloWorld.png");
			sprite.params.pos = XMFLOAT3(screenW / 2, screenH / 2, 0);
			sprite.params.siz = XMFLOAT2(200, 100);
			sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite.anim.rot = XM_PI / 4.0f;
			this->addSprite(&sprite);
			break;
		}
		case 1:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/teapot.wiscene");
			break;
		case 2:
			wiRenderer::LoadModel("../models/emitter_smoke.wiscene");
			break;
		case 3:
			wiRenderer::LoadModel("../models/emitter_skinned.wiscene");
			break;
		case 4:
			wiRenderer::LoadModel("../models/hairparticle_torus.wiscene", XMMatrixTranslation(0, 1, 0));
			break;
		case 5:
			wiRenderer::SetToDrawGridHelper(true);
			wiLua::GetGlobal()->RunFile("test_script.lua");
			break;
		case 6:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/water_test.wiscene", XMMatrixTranslation(0, 1, 0));
			break;
		case 7:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/shadows_test.wiscene", XMMatrixTranslation(0, 1, 0));
			break;
		case 8:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/physics_test.wiscene");
			break;
		case 9:
			wiRenderer::LoadModel("../models/cloth_test.wiscene", XMMatrixTranslation(0, 3, 4));
			break;
		case 10:
			RunJobSystemTest();
			break;
		case 11:
			RunFontTest();
			break;
		case 12:
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/volumetric_test.wiscene", XMMatrixTranslation(0, 0, 4));
			break;
		case 13:
			RunSpriteTest();
			break;
		case 14:
			wiRenderer::GetDevice()->SetVSyncEnabled(false); // turn off vsync if we can to accelerate the baking
			wiRenderer::SetTemporalAAEnabled(true);
			wiRenderer::LoadModel("../models/lightmap_bake_test.wiscene", XMMatrixTranslation(0, 0, 4));
			break;
		default:
			assert(0);
			break;
		}

	});
	testSelector->SetSelected(0);
	GetGUI().AddWidget(testSelector);

}
TestsRenderer::~TestsRenderer()
{
}

void TestsRenderer::RunJobSystemTest()
{
	wiTimer timer;

	// This will simulate going over a big dataset first in a simple loop, then with the Job System and compare timings
	uint32_t itemCount = 1000000;
	std::stringstream ss("");
	ss << "Job System performance test:" << std::endl;
	ss << "You can find out more in Tests.cpp, RunJobSystemTest() function." << std::endl << std::endl;

	ss << "wiJobSystem was created with " << wiJobSystem::GetThreadCount() << " worker threads." << std::endl << std::endl;

	ss << "1) Execute() test:" << std::endl;

	// Serial test
	{
		timer.record();
		wiHelper::Spin(100);
		wiHelper::Spin(100);
		wiHelper::Spin(100);
		wiHelper::Spin(100);
		double time = timer.elapsed();
		ss << "Serial took " << time << " milliseconds" << std::endl;
	}
	// Execute test
	{
		timer.record();
		wiJobSystem::Execute([]{ wiHelper::Spin(100); });
		wiJobSystem::Execute([]{ wiHelper::Spin(100); });
		wiJobSystem::Execute([]{ wiHelper::Spin(100); });
		wiJobSystem::Execute([]{ wiHelper::Spin(100); });
		wiJobSystem::Wait();
		double time = timer.elapsed();
		ss << "wiJobSystem::Execute() took " << time << " milliseconds" << std::endl;
	}

	ss << std::endl;
	ss << "2) Dispatch() test:" << std::endl;

	// Simple loop test:
	{
		std::vector<wiSceneSystem::CameraComponent> dataSet(itemCount);
		timer.record();
		for (uint32_t i = 0; i < itemCount; ++i)
		{
			dataSet[i].UpdateCamera();
		}
		double time = timer.elapsed();
		ss << "Simple loop took " << time << " milliseconds" << std::endl;
	}

	// Dispatch test:
	{
		std::vector<wiSceneSystem::CameraComponent> dataSet(itemCount);
		timer.record();
		wiJobSystem::Dispatch(itemCount, 1000, [&](wiJobDispatchArgs args) {
			dataSet[args.jobIndex].UpdateCamera();
		});
		wiJobSystem::Wait();
		double time = timer.elapsed();
		ss << "wiJobSystem::Dispatch() took " << time << " milliseconds" << std::endl;
	}

	static wiFont font;
	font = wiFont(ss.str());
	font.params.posX = wiRenderer::GetDevice()->GetScreenWidth() / 2;
	font.params.posY = wiRenderer::GetDevice()->GetScreenHeight() / 2;
	font.params.h_align = WIFALIGN_CENTER;
	font.params.v_align = WIFALIGN_CENTER;
	font.params.size = 24;
	this->addFont(&font);
}
void TestsRenderer::RunFontTest()
{
	static wiFont font;
	static wiFont font_upscaled;
	int arial = wiFont::AddFontStyle(wiFont::GetFontPath() + "arial.ttf");

	font.SetText("This is Arial, size 32 wiFont");
	font_upscaled.SetText("This is Arial, size 14 wiFont, but upscaled to 32");

	font.params.posX = wiRenderer::GetDevice()->GetScreenWidth() / 2;
	font.params.posY = wiRenderer::GetDevice()->GetScreenHeight() / 6;
	font.params.size = 32;

	font_upscaled.params = font.params;
	font_upscaled.params.posY += font.textHeight();

	font.style = arial;
	font_upscaled.style = arial;
	font_upscaled.params.size = 14;
	font_upscaled.params.scaling = 32.0f / 14.0f;

	addFont(&font);
	addFont(&font_upscaled);

	static wiFont font_aligned;
	font_aligned = font;
	font_aligned.params.posY += font.textHeight() * 2;
	font_aligned.params.size = 38;
	font_aligned.params.shadowColor = wiColor::Red();
	font_aligned.params.h_align = WIFALIGN_CENTER;
	font_aligned.SetText("Center aligned, red shadow, bigger");
	addFont(&font_aligned);

	static wiFont font_aligned2;
	font_aligned2 = font_aligned;
	font_aligned2.params.posY += font_aligned.textHeight();
	font_aligned2.params.shadowColor = wiColor::Purple();
	font_aligned2.params.h_align = WIFALIGN_RIGHT;
	font_aligned2.SetText("Right aligned, purple shadow");
	addFont(&font_aligned2);

	std::stringstream ss("");
	std::ifstream file("font_test.txt");
	if (file.is_open())
	{
		while (!file.eof())
		{
			std::string s;
			file >> s;
			ss << s << "\t";
		}
	}
	static wiFont font_japanese;
	font_japanese = font_aligned2;
	font_japanese.params.posY += font_aligned2.textHeight();
	font_japanese.style = wiFont::AddFontStyle("yumin.ttf");
	font_japanese.params.shadowColor = wiColor::Transparent();
	font_japanese.params.h_align = WIFALIGN_CENTER;
	font_japanese.params.size = 34;
	font_japanese.SetText(ss.str());
	addFont(&font_japanese);

	static wiFont font_colored;
	font_colored.params.color = wiColor::Cyan();
	font_colored.params.h_align = WIFALIGN_CENTER;
	font_colored.params.v_align = WIFALIGN_TOP;
	font_colored.params.size = 26;
	font_colored.params.posX = wiRenderer::GetDevice()->GetScreenWidth() / 2;
	font_colored.params.posY = font_japanese.params.posY + font_japanese.textHeight();
	font_colored.SetText("Colored font");
	addFont(&font_colored);
}
void TestsRenderer::RunSpriteTest()
{
	const float step = 30;
	const XMFLOAT3 startPos = XMFLOAT3(wiRenderer::GetDevice()->GetScreenWidth() * 0.3f, wiRenderer::GetDevice()->GetScreenHeight() * 0.2f, 0);
	wiImageParams params;
	params.pos = startPos;
	params.siz = XMFLOAT2(128, 128);
	params.pivot = XMFLOAT2(0.5f, 0.5f);
	params.quality = QUALITY_LINEAR;
	params.sampleFlag = SAMPLEMODE_CLAMP;
	params.blendFlag = BLENDMODE_ALPHA;

	// Info:
	{
		static wiFont font("For more information, please see \nTests.cpp, RunSpriteTest() function.");
		font.params.posX = 10;
		font.params.posY = 200;
		addFont(&font);
	}

	// Simple sprite, no animation:
	{
		static wiSprite sprite("../logo/logo_small.png");
		sprite.params = params;
		addSprite(&sprite);

		static wiFont font("No animation: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, fade animation:
	{
		static wiSprite sprite("../logo/logo_small.png");
		sprite.params = params;
		sprite.anim = wiSprite::Anim();
		sprite.anim.fad = 1.2f; // (you can also do opacity animation with sprite.anim.opa)
		sprite.anim.repeatable = true;
		addSprite(&sprite);

		static wiFont font("Fade animation: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, wobble animation:
	{
		static wiSprite sprite("../logo/logo_small.png");
		sprite.params = params;
		sprite.anim = wiSprite::Anim();
		sprite.anim.wobbleAnim.amount = XMFLOAT2(1.2, 0.8);
		addSprite(&sprite);

		static wiFont font("Wobble animation: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, rotate animation:
	{
		static wiSprite sprite("../logo/logo_small.png");
		sprite.params = params;
		sprite.anim = wiSprite::Anim();
		sprite.anim.rot = 2.8f;
		sprite.anim.repeatable = true;
		addSprite(&sprite);

		static wiFont font("Rotate animation: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Simple sprite, movingtex:
	{
		static wiSprite sprite("images/movingtex.png", "../logo/logo_small.png"); // first param is the texture we will display (and also scroll here). Second param is a mask texture
		// Don't overwrite all params for this, because we want to keep the mask...
		sprite.params.pos = params.pos;
		sprite.params.siz = params.siz;
		sprite.params.pivot = params.pivot;
		sprite.params.col = XMFLOAT4(2, 2, 2, 1); // increase brightness a bit
		sprite.params.sampleFlag = SAMPLEMODE_MIRROR; // texcoords will be scrolled out of bounds, so set up a wrap mode other than clamp
		sprite.anim = wiSprite::Anim();
		sprite.anim.movingTexAnim.speedX = 0;
		sprite.anim.movingTexAnim.speedY = 2; // scroll the texture vertically. This value is pixels/second. So because our texture here is 1x2 pixels, just scroll it once fully per second with a value of 2
		addSprite(&sprite);

		static wiFont font("MovingTex + mask: ");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x = startPos.x;
		params.pos.y += sprite.params.siz.y + step * 1.5f;
	}


	// Now the spritesheets:

	// Spritesheet, no anim:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params; // nothing extra, just display the full spritesheet
		addSprite(&sprite);

		static wiFont font("Spritesheet: \n(without animation)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, single line:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wiSprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 4; // animate only a single line horizontally
		addSprite(&sprite);

		static wiFont font("single line anim: \n(4 frames)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, single vertical line:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wiSprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 4; // again, specify 4 total frames to loop...
		sprite.anim.drawRectAnim.horizontalFrameCount = 1; // ...but this time, limit the horizontal frame count. This way, we can get it to only animate vertically
		addSprite(&sprite);

		static wiFont font("single line: \n(4 vertical frames)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, multiline:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wiSprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 16; // all frames
		sprite.anim.drawRectAnim.horizontalFrameCount = 4; // all horizontal frames
		addSprite(&sprite);

		static wiFont font("multiline: \n(all 16 frames)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	// Spritesheet, multiline, irregular:
	{
		static wiSprite sprite("images/spritesheet_grid.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 128, 128)); // drawrect cutout for a 0,0,128,128 pixel rect, this is also the first frame of animation
		sprite.anim = wiSprite::Anim();
		sprite.anim.repeatable = true; // enable looping
		sprite.anim.drawRectAnim.frameRate = 3; // 3 FPS, to be easily readable
		sprite.anim.drawRectAnim.frameCount = 14; // NOT all frames, which makes it irregular, so the last line will not contain all horizontal frames
		sprite.anim.drawRectAnim.horizontalFrameCount = 4; // all horizontal frames
		addSprite(&sprite);

		static wiFont font("irregular multiline: \n(14 frames)");
		font.params.h_align = WIFALIGN_CENTER;
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x = startPos.x;
		params.pos.y += sprite.params.siz.y + step * 1.5f;
	}




	// And the nice ones:

	{
		static wiSprite sprite("images/fire_001.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192)); // set the draw rect (texture cutout). This will also be the first frame of the animation
		sprite.anim = wiSprite::Anim(); // reset animation state
		sprite.anim.drawRectAnim.frameCount = 20; // set the total frame count of the animation
		sprite.anim.drawRectAnim.horizontalFrameCount = 5; // this is a multiline spritesheet, so also set how many maximum frames there are in a line
		sprite.anim.drawRectAnim.frameRate = 40; // animation frames per second
		sprite.anim.repeatable = true; // looping
		addSprite(&sprite);

		static wiFont font("For the following spritesheets, credits belong to: https://mrbubblewand.wordpress.com/download/");
		font.params.v_align = WIFALIGN_BOTTOM;
		font.params.posX = int(sprite.params.pos.x - sprite.params.siz.x * 0.5f);
		font.params.posY = int(sprite.params.pos.y - sprite.params.siz.y * 0.5f);
		addFont(&font);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wiSprite sprite("images/wind_002.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wiSprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 30;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 30;
		sprite.anim.repeatable = true;
		addSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wiSprite sprite("images/water_003.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wiSprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 50;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 27;
		sprite.anim.repeatable = true;
		addSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wiSprite sprite("images/earth_001.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wiSprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 20;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 27;
		sprite.anim.repeatable = true;
		addSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

	{
		static wiSprite sprite("images/special_001.png");
		sprite.params = params;
		sprite.params.enableDrawRect(XMFLOAT4(0, 0, 192, 192));
		sprite.anim = wiSprite::Anim();
		sprite.anim.drawRectAnim.frameCount = 40;
		sprite.anim.drawRectAnim.horizontalFrameCount = 5;
		sprite.anim.drawRectAnim.frameRate = 27;
		sprite.anim.repeatable = true;
		addSprite(&sprite);

		params.pos.x += sprite.params.siz.x + step;
	}

}
