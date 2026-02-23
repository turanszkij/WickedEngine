#include "stdafx.h"
#include "AIWindow.h"

#include "stable-diffusion/stable-diffusion.h"
#include "miniz/miniz.h"
#include "miniz/miniz.c"

#include <cstdlib>

using namespace wi::graphics;

enum HardwareSelection
{
	GPU,
	CPU,
};

void AIWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("AIWindow", wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::RESIZE_RIGHT);
	SetText("Artificial Intelligence Tools " ICON_AI);
	SetSize(XMFLOAT2(300, 800));

	infoLabel.Create("AIInfoLabel");
	infoLabel.SetSize(XMFLOAT2(100, 100));
	infoLabel.SetText("Here you can use the Stable Diffusion AI to generate images from text prompts, then use them as resources. Stable Diffusion and training datasets will be downloaded separately on demand, so first use can increase generation time.");
	AddWidget(&infoLabel);

	preview.Create("AIImagePreview");
	preview.SetSize(XMFLOAT2(256, 256));
	AddWidget(&preview);

	prompt.Create("AIPrompt");
	prompt.SetText("a cat wearing a hat");
	prompt.SetDescription("Prompt: ");
	prompt.OnInputAccepted([this](const wi::gui::EventArgs& args) { GenerateImage(); });
	AddWidget(&prompt);

	generate.Create("AIGenerate");
	generate.SetTooltip("Run the AI image generation.");
	generate.SetText(ICON_REFRESH);
	generate.OnClick([this](const wi::gui::EventArgs& args) { GenerateImage(); });
	AddWidget(&generate);

	hardwareCombo.Create("AIHardwareCombo");
	hardwareCombo.SetTooltip("Choose GPU or GPU to exacute AI task on. GPU is recommended for speed, CPU is recommended if GPU is not supported or doesn't have enough memory.");
	hardwareCombo.SetText("Hardware: ");
	hardwareCombo.AddItem("GPU", GPU);
	hardwareCombo.AddItem("CPU", CPU);
	AddWidget(&hardwareCombo);

	modelCombo.Create("AIModelCombo");
	modelCombo.SetTooltip("Choose AI training model dataset. To add new models, put .safetensors files into stable-diffusion/models folder.\nIf there are no model files, a smaller example model will be downloaded before first image generation.");
	modelCombo.SetText("Model: ");
	wi::helper::GetFileNamesInDirectory(wi::helper::GetCurrentPath() + "/stable-diffusion/models/", [this](std::string filename) {
		if (wi::helper::GetExtensionFromFileName(filename) != "safetensors")
			return;
		modelCombo.AddItem(wi::helper::RemoveExtension(wi::helper::GetFileNameFromPath(filename)));
	});
	AddWidget(&modelCombo);

	widthInput.Create("AIGenerateWidth");
	widthInput.SetCancelInputEnabled(false);
	widthInput.SetText("512");
	widthInput.SetDescription("Width: ");
	widthInput.SetSize(XMFLOAT2(40, widthInput.GetSize().y));
	AddWidget(&widthInput);

	heightInput.Create("AIGenerateHeight");
	heightInput.SetCancelInputEnabled(false);
	heightInput.SetText("512");
	heightInput.SetDescription("Height: ");
	heightInput.SetSize(XMFLOAT2(40, heightInput.GetSize().y));
	AddWidget(&heightInput);

	SetVisible(false);
}

void AIWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	if (wi::jobsystem::IsBusy(generation_status))
	{
		generate.font.params.rotation += XM_PI * 0.016f;
		preview.SetColor(wi::Color(255, 255, 255, 100));
		preview.SetText(ICON_REFRESH);
		preview.font.params.scaling = 8;
		preview.font.params.rotation = generate.font.params.rotation;
	}
	else
	{
		generate.font.params.rotation = 0;
		preview.SetImage(imageResource);
		preview.SetColor(wi::Color::White());
		preview.SetText("");
	}

	layout.add_fullwidth(infoLabel);
	layout.add_fullwidth_aspect(preview);
	layout.add_fullwidth(prompt);
	layout.add_fullwidth(generate);
	layout.add_fullwidth(hardwareCombo);
	layout.add_fullwidth(modelCombo);

	layout.add(widthInput, heightInput);
	widthInput.SetPos(XMFLOAT2(widthInput.GetPos().x + 10, widthInput.GetPos().y)); // hm
}

void AIWindow::GenerateImage()
{
	wi::jobsystem::Wait(generation_status);
	generation_status.priority = wi::jobsystem::Priority::Low;

	wi::jobsystem::Execute(generation_status, [this](wi::jobsystem::JobArgs args) {

		const bool is_cpu = (HardwareSelection)hardwareCombo.GetSelectedUserdata() == CPU;
		const std::string hw_folder = is_cpu ? "stable-diffusion/cpu" : "stable-diffusion/gpu";

#if defined(PLATFORM_WINDOWS_DESKTOP)
		const std::string dll_filename = hw_folder + "/stable-diffusion.dll";
#elif defined(PLATFORM_LINUX)
		const std::string dll_filename = hw_folder + "/libstable-diffusion.so";
#elif defined(PLATFORM_MACOS)
		const std::string dll_filename = hw_folder + "/libstable-diffusion.dylib";
#endif // PLATFORM

		if (!wi::helper::FileExists(dll_filename))
		{
#if defined(PLATFORM_WINDOWS_DESKTOP)
			const std::string zip_name = is_cpu ? "sd-master-c5eb1e4-bin-win-avx2-x64.zip" : "sd-master-c5eb1e4-bin-win-vulkan-x64.zip";
#elif defined(PLATFORM_LINUX)
			const std::string zip_name = is_cpu ? "sd-master-c5eb1e4-bin-Linux-Ubuntu-24.04-x86_64.zip" : "sd-master-c5eb1e4-bin-Linux-Ubuntu-24.04-x86_64-vulkan.zip";
#elif defined(PLATFORM_MACOS)
			const std::string zip_name = "sd-master-c5eb1e4-bin-Darwin-macOS-15.7.3-arm64.zip";
#endif // PLATFORM
			wi::helper::DirectoryCreate(hw_folder);
			std::string command = is_cpu ? "cd stable-diffusion/cpu && " : "cd stable-diffusion/gpu && ";
			command += "curl -L -O https://github.com/leejet/stable-diffusion.cpp/releases/download/master-505-c5eb1e4/" + zip_name;
			int result = system(command.c_str());
			if (result != 0)
			{
				wilog_error("Stable diffusion download command failed: %s", command.c_str());
				return;
			}
			wilog("Downloaded Stable Diffusion");

			const std::string zip_path = hw_folder + "/" + zip_name;
			const std::string out_dir = hw_folder + "/";
			wi::helper::DirectoryCreate(out_dir);
			mz_zip_archive zip_archive = {};
			mz_zip_reader_init_file(&zip_archive, zip_path.c_str(), 0);
			mz_uint file_count = mz_zip_reader_get_num_files(&zip_archive);
			for (mz_uint i = 0; i < file_count; ++i)
			{
				char filename_in_zip[512] = {};
				mz_zip_archive_file_stat file_stat;
				mz_zip_reader_file_stat(&zip_archive, i, &file_stat);
				std::memcpy(filename_in_zip, file_stat.m_filename, sizeof(filename_in_zip) - 1);
				if (mz_zip_reader_is_file_a_directory(&zip_archive, i))
					continue;

				char out_path[512] = {};
				snprintf(out_path, sizeof(out_path), "%s/%s", out_dir.c_str(), filename_in_zip);

				size_t uncompressed_size = 0;
				void* p = mz_zip_reader_extract_to_heap(&zip_archive, i, &uncompressed_size, 0);
				if (p == nullptr)
				{
					mz_free(p);
					mz_zip_reader_end(&zip_archive);
				}
				wi::helper::FileWrite(out_path, (const uint8_t*)p, uncompressed_size);
				mz_free(p);

				wilog("Extracted: %s (uncompressed size: %llu)\n", filename_in_zip, (unsigned long long)uncompressed_size);
			}
			mz_zip_reader_end(&zip_archive);
		}

		HMODULE stable_diffusion = wiLoadLibrary(dll_filename.c_str());
		assert(stable_diffusion);
		LINK_DLL_FUNCTION(sd_ctx_params_init, stable_diffusion);
		LINK_DLL_FUNCTION(new_sd_ctx, stable_diffusion);
		LINK_DLL_FUNCTION(sd_img_gen_params_init, stable_diffusion);
		LINK_DLL_FUNCTION(generate_image, stable_diffusion);
		LINK_DLL_FUNCTION(sd_sample_params_init, stable_diffusion);
		LINK_DLL_FUNCTION(free_sd_ctx, stable_diffusion);

		std::string model_name;

		if (modelCombo.GetItemCount() == 0)
		{
			// Download sample model if there are none:
			static const std::string default_modelname = "v1-5-pruned-emaonly.safetensors";
			static const std::string command = "cd stable-diffusion/models && curl -L -O https://huggingface.co/runwayml/stable-diffusion-v1-5/resolve/main/v1-5-pruned-emaonly.safetensors";
			int result = system(command.c_str());
			if (result != 0)
			{
				wilog_error("Stable diffusion default model download command failed: %s", command.c_str());
				return;
			}
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [this](uint64_t userdata) {
				modelCombo.AddItem(wi::helper::RemoveExtension(default_modelname));
			});
			model_name = wi::helper::RemoveExtension(default_modelname);
		}
		else
		{
			model_name = modelCombo.GetItemText(modelCombo.GetSelected());
		}

		const std::string model_path = "stable-diffusion/models/" + model_name + ".safetensors";

		const std::string prompt_text = prompt.GetText();

		sd_ctx_params_t sd_params;
		sd_ctx_params_init(&sd_params);
		sd_params.model_path = model_path.c_str();
		sd_params.wtype = SD_TYPE_F16;
		sd_params.n_threads = -1;
		sd_params.rng_type = STD_DEFAULT_RNG;
		sd_params.keep_clip_on_cpu = false;
		sd_params.keep_vae_on_cpu = false;

		sd_ctx_t* sd_ctx = new_sd_ctx(&sd_params);
		if (sd_ctx != nullptr)
		{
			sd_img_gen_params_t img_params;
			sd_img_gen_params_init(&img_params);
			img_params.width = atoi(widthInput.GetValue().c_str());
			img_params.height = atoi(heightInput.GetValue().c_str());
			img_params.prompt = prompt_text.c_str();
			img_params.strength = 0.0f;
			img_params.batch_count = 1;

			sd_sample_params_init(&img_params.sample_params);
			img_params.sample_params.sample_method = EULER_A_SAMPLE_METHOD;
			img_params.sample_params.sample_steps = 28;
			img_params.sample_params.scheduler = KARRAS_SCHEDULER;
			img_params.sample_params.guidance.txt_cfg = 7.5f;
			img_params.sample_params.eta = 1.0f;

			sd_image_t* image = generate_image(sd_ctx, &img_params);
			if (image != nullptr)
			{
				TextureDesc desc;
				desc.width = image->width;
				desc.height = image->height;
				desc.format = Format::R8G8B8A8_UNORM;

				struct Color3
				{
					uint8_t r, g, b;
				};
				const Color3* rgb = (const Color3*)image->data;
				const uint32_t count = desc.width * desc.height;
				wi::vector<uint8_t> rgba(count * sizeof(wi::Color));
				wi::Color* dst = (wi::Color*)rgba.data();
				for (size_t i = 0; i < count; ++i)
				{
					dst[i] = wi::Color(rgb[i].r, rgb[i].g, rgb[i].b, 255);
				}
				wi::vector<uint8_t> filedata;
				if (wi::helper::saveTextureToMemoryFile(rgba, desc, "PNG", filedata))
				{
					static int counter = 0;
					imageResource = wi::resourcemanager::Load(prompt_text + std::to_string(counter++) + ".png", wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA, filedata.data(), filedata.size());
				}
				else
				{
					wilog_error("Failed saving generated image to PNG from prompt: %s", prompt_text.c_str());
				}
			}
			else
			{
				wilog_error("Stable diffusion image generation failed");
			}

			free_sd_ctx(sd_ctx);
		}
		else
		{
			wilog_error("Stable diffusion context creation failed");
		}
	});
}
