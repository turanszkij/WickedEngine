#include "wiHelper.h"
#include "wiPlatform.h"
#include "wiBacklog.h"
#include "wiEventHandler.h"
#include "wiMath.h"
#include "wiImage.h"
#include "wiRenderer.h"

#include "Utility/lodepng.h"
#include "Utility/dds.h"
#include "Utility/stb_image_write.h"
#include "Utility/zstd/zstd.h"
#include "Utility/win32ico.h"

#ifndef __SCE__
#include "Utility/portable-file-dialogs.h"
#endif // __SCE__

#include <thread>
#include <locale>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <iostream>
#include <cstdlib>

#if defined(_WIN32)
#include <direct.h>
#include <Psapi.h> // GetProcessMemoryInfo
#include <Commdlg.h> // openfile
#include <WinBase.h>
#endif // _WIN32

#ifdef PLATFORM_LINUX
#include <sys/sysinfo.h>
#endif // PLATFORM_LINUX

#ifdef PLATFORM_WINDOWS_DESKTOP
#include <comdef.h> // com_error
#endif // PLATFORM_WINDOWS_DESKTOP

namespace wi::helper
{

	std::string toUpper(const std::string& s)
	{
		std::string result;
		std::locale loc;
		for (unsigned int i = 0; i < s.length(); ++i)
		{
			result += std::toupper(s.at(i), loc);
		}
		return result;
	}
	std::string toLower(const std::string& s)
	{
		std::string result;
		std::locale loc;
		for (unsigned int i = 0; i < s.length(); ++i)
		{
			result += std::tolower(s.at(i), loc);
		}
		return result;
	}

	void messageBox(const std::string& msg, const std::string& caption)
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		std::wstring wmsg;
		std::wstring wcaption;
		StringConvert(msg, wmsg);
		StringConvert(caption, wcaption);
		MessageBox(GetActiveWindow(), wmsg.c_str(), wcaption.c_str(), 0);
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef SDL2
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption.c_str(), msg.c_str(), NULL);
#endif // SDL2
	}

	MessageBoxResult messageBoxCustom(const std::string& msg, const std::string& caption, const std::string& buttons)
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		std::wstring wmsg;
		std::wstring wcaption;
		StringConvert(msg, wmsg);
		StringConvert(caption, wcaption);

		UINT type = MB_ICONQUESTION;
		if (buttons == "YesNoCancel")
		{
			type |= MB_YESNOCANCEL;
		}
		else if (buttons == "YesNo")
		{
			type |= MB_YESNO;
		}
		else if (buttons == "OKCancel")
		{
			type |= MB_OKCANCEL;
		}
		else if (buttons == "AbortRetryIgnore")
		{
			type |= MB_ABORTRETRYIGNORE;
		}
		else // default to OK
		{
			type |= MB_OK;
		}

		const int result = MessageBox(GetActiveWindow(), wmsg.c_str(), wcaption.c_str(), type);

		switch (result)
		{
		case IDOK: return MessageBoxResult::OK;
		case IDCANCEL: return MessageBoxResult::Cancel;
		case IDYES: return MessageBoxResult::Yes;
		case IDNO: return MessageBoxResult::No;
		case IDABORT: return MessageBoxResult::Abort;
		case IDRETRY: return MessageBoxResult::Retry;
		case IDIGNORE: return MessageBoxResult::Ignore;
		default: return MessageBoxResult::Cancel;
		}
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef SDL2
		const SDL_MessageBoxButtonData buttons_data[] = {
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Yes" },
			{ 0, 1, "No" },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Cancel" },
		};
		const SDL_MessageBoxData messageboxdata = {
			SDL_MESSAGEBOX_INFORMATION,
			NULL,
			caption.c_str(),
			msg.c_str(),
			SDL_arraysize(buttons_data),
			buttons_data,
			NULL
		};
		int buttonid;
		if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0)
		{
			return MessageBoxResult::Cancel;
		}
		switch (buttonid)
		{
		case 0: return MessageBoxResult::Yes;
		case 1: return MessageBoxResult::No;
		case 2: return MessageBoxResult::Cancel;
		default: return MessageBoxResult::Cancel;
		}
#endif // SDL2

		return MessageBoxResult::Cancel;
	}

	std::string screenshot(const wi::graphics::SwapChain& swapchain, const std::string& name)
	{
		return screenshot(wi::graphics::GetDevice()->GetBackBuffer(&swapchain));
	}

	std::string screenshot(const wi::graphics::Texture& texture, const std::string& name)
	{
		std::string directory;
		if (name.empty())
		{
			directory = std::filesystem::current_path().string() + "/screenshots";
		}
		else
		{
			directory = GetDirectoryFromPath(name);
		}

		if (!directory.empty()) //PE: Crash if only filename is used with no folder.
			DirectoryCreate(directory);

		std::string filename = name;
		if (filename.empty())
		{
			filename = directory + "/sc_" + getCurrentDateTimeAsString() + ".png";
		}

		bool result = saveTextureToFile(texture, filename);
		assert(result);

		if (result)
		{
			return filename;
		}
		return "";
	}

	bool saveTextureToMemory(const wi::graphics::Texture& texture, wi::vector<uint8_t>& texturedata)
	{
		using namespace wi::graphics;

		GraphicsDevice* device = wi::graphics::GetDevice();

		TextureDesc desc = texture.GetDesc();

		Texture stagingTex;
		TextureDesc staging_desc = desc;
		staging_desc.usage = Usage::READBACK;
		staging_desc.layout = ResourceState::COPY_DST;
		staging_desc.bind_flags = BindFlag::NONE;
		staging_desc.misc_flags = ResourceMiscFlag::NONE;
		bool success = device->CreateTexture(&staging_desc, nullptr, &stagingTex);
		assert(success);

		CommandList cmd = device->BeginCommandList();

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&texture,texture.desc.layout,ResourceState::COPY_SRC),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->CopyResource(&stagingTex, &texture, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&texture,ResourceState::COPY_SRC,texture.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->SubmitCommandLists();
		device->WaitForGPU();

		texturedata.clear();

		if (stagingTex.mapped_data != nullptr)
		{
			texturedata.resize(ComputeTextureMemorySizeInBytes(desc));

			const uint32_t data_stride = GetFormatStride(desc.format);
			const uint32_t block_size = GetFormatBlockSize(desc.format);
			size_t cpy_offset = 0;
			size_t subresourceIndex = 0;
			for (uint32_t layer = 0; layer < desc.array_size; ++layer)
			{
				for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
				{
					const uint32_t mip_width = std::max(1u, desc.width >> mip);
					const uint32_t mip_height = std::max(1u, desc.height >> mip);
					const uint32_t mip_depth = std::max(1u, desc.depth >> mip);
					const uint32_t num_blocks_x = (mip_width + block_size - 1) / block_size;
					const uint32_t num_blocks_y = (mip_height + block_size - 1) / block_size;

					assert(subresourceIndex < stagingTex.mapped_subresource_count);
					const SubresourceData& subresourcedata = stagingTex.mapped_subresources[subresourceIndex++];
					const size_t dst_rowpitch = num_blocks_x * data_stride;
					for (uint32_t z = 0; z < mip_depth; ++z)
					{
						uint8_t* dst_slice = texturedata.data() + cpy_offset;
						uint8_t* src_slice = (uint8_t*)subresourcedata.data_ptr + subresourcedata.slice_pitch * z;
						for (uint32_t i = 0; i < num_blocks_y; ++i)
						{
							std::memcpy(
								dst_slice + i * dst_rowpitch,
								src_slice + i * subresourcedata.row_pitch,
								dst_rowpitch
							);
						}
						cpy_offset += num_blocks_y * dst_rowpitch;
					}
				}
			}
		}
		else
		{
			assert(0);
		}

		return stagingTex.mapped_data != nullptr;
	}

	bool saveTextureToMemoryFile(const wi::graphics::Texture& texture, const std::string& fileExtension, wi::vector<uint8_t>& filedata)
	{
		using namespace wi::graphics;
		TextureDesc desc = texture.GetDesc();
		wi::vector<uint8_t> texturedata;
		if (saveTextureToMemory(texture, texturedata))
		{
			return saveTextureToMemoryFile(texturedata, desc, fileExtension, filedata);
		}
		return false;
	}

	bool saveTextureToMemoryFile(const wi::vector<uint8_t>& texturedata, const wi::graphics::TextureDesc& desc, const std::string& fileExtension, wi::vector<uint8_t>& filedata)
	{
		using namespace wi::graphics;
		const uint32_t data_stride = GetFormatStride(desc.format);

		std::string extension = wi::helper::toUpper(fileExtension);

		if (extension.compare("DDS") == 0)
		{
			filedata.resize(sizeof(dds::Header) + texturedata.size());
			dds::DXGI_FORMAT dds_format = dds::DXGI_FORMAT_UNKNOWN;
			switch (desc.format)
			{
			case wi::graphics::Format::R32G32B32A32_FLOAT:
				dds_format = dds::DXGI_FORMAT_R32G32B32A32_FLOAT;
				break;
			case wi::graphics::Format::R32G32B32A32_UINT:
				dds_format = dds::DXGI_FORMAT_R32G32B32A32_UINT;
				break;
			case wi::graphics::Format::R32G32B32A32_SINT:
				dds_format = dds::DXGI_FORMAT_R32G32B32A32_SINT;
				break;
			case wi::graphics::Format::R32G32B32_FLOAT:
				dds_format = dds::DXGI_FORMAT_R32G32B32_FLOAT;
				break;
			case wi::graphics::Format::R32G32B32_UINT:
				dds_format = dds::DXGI_FORMAT_R32G32B32_UINT;
				break;
			case wi::graphics::Format::R32G32B32_SINT:
				dds_format = dds::DXGI_FORMAT_R32G32B32_SINT;
				break;
			case wi::graphics::Format::R16G16B16A16_FLOAT:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_FLOAT;
				break;
			case wi::graphics::Format::R16G16B16A16_UNORM:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_UNORM;
				break;
			case wi::graphics::Format::R16G16B16A16_UINT:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_UINT;
				break;
			case wi::graphics::Format::R16G16B16A16_SNORM:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_SNORM;
				break;
			case wi::graphics::Format::R16G16B16A16_SINT:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_SINT;
				break;
			case wi::graphics::Format::R32G32_FLOAT:
				dds_format = dds::DXGI_FORMAT_R32G32_FLOAT;
				break;
			case wi::graphics::Format::R32G32_UINT:
				dds_format = dds::DXGI_FORMAT_R32G32_UINT;
				break;
			case wi::graphics::Format::R32G32_SINT:
				dds_format = dds::DXGI_FORMAT_R32G32_SINT;
				break;
			case wi::graphics::Format::R10G10B10A2_UNORM:
				dds_format = dds::DXGI_FORMAT_R10G10B10A2_UNORM;
				break;
			case wi::graphics::Format::R10G10B10A2_UINT:
				dds_format = dds::DXGI_FORMAT_R10G10B10A2_UINT;
				break;
			case wi::graphics::Format::R11G11B10_FLOAT:
				dds_format = dds::DXGI_FORMAT_R11G11B10_FLOAT;
				break;
			case wi::graphics::Format::R8G8B8A8_UNORM:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			case wi::graphics::Format::R8G8B8A8_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				break;
			case wi::graphics::Format::R8G8B8A8_UINT:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_UINT;
				break;
			case wi::graphics::Format::R8G8B8A8_SNORM:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_SNORM;
				break;
			case wi::graphics::Format::R8G8B8A8_SINT:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_SINT;
				break;
			case wi::graphics::Format::B8G8R8A8_UNORM:
				dds_format = dds::DXGI_FORMAT_B8G8R8A8_UNORM;
				break;
			case wi::graphics::Format::B8G8R8A8_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_R16G16_SINT;
				break;
			case wi::graphics::Format::R16G16_FLOAT:
				dds_format = dds::DXGI_FORMAT_R16G16_FLOAT;
				break;
			case wi::graphics::Format::R16G16_UNORM:
				dds_format = dds::DXGI_FORMAT_R16G16_UNORM;
				break;
			case wi::graphics::Format::R16G16_UINT:
				dds_format = dds::DXGI_FORMAT_R16G16_UINT;
				break;
			case wi::graphics::Format::R16G16_SNORM:
				dds_format = dds::DXGI_FORMAT_R16G16_SNORM;
				break;
			case wi::graphics::Format::R16G16_SINT:
				dds_format = dds::DXGI_FORMAT_R16G16_SINT;
				break;
			case wi::graphics::Format::D32_FLOAT:
			case wi::graphics::Format::R32_FLOAT:
				dds_format = dds::DXGI_FORMAT_R32_FLOAT;
				break;
			case wi::graphics::Format::R32_UINT:
				dds_format = dds::DXGI_FORMAT_R32_UINT;
				break;
			case wi::graphics::Format::R32_SINT:
				dds_format = dds::DXGI_FORMAT_R32_SINT;
				break;
			case wi::graphics::Format::R9G9B9E5_SHAREDEXP:
				dds_format = dds::DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
				break;
			case wi::graphics::Format::R8G8_UNORM:
				dds_format = dds::DXGI_FORMAT_R8G8_UNORM;
				break;
			case wi::graphics::Format::R8G8_UINT:
				dds_format = dds::DXGI_FORMAT_R8G8_UINT;
				break;
			case wi::graphics::Format::R8G8_SNORM:
				dds_format = dds::DXGI_FORMAT_R8G8_SNORM;
				break;
			case wi::graphics::Format::R8G8_SINT:
				dds_format = dds::DXGI_FORMAT_R8G8_SINT;
				break;
			case wi::graphics::Format::R16_FLOAT:
				dds_format = dds::DXGI_FORMAT_R16_FLOAT;
				break;
			case wi::graphics::Format::D16_UNORM:
			case wi::graphics::Format::R16_UNORM:
				dds_format = dds::DXGI_FORMAT_R16_UNORM;
				break;
			case wi::graphics::Format::R16_UINT:
				dds_format = dds::DXGI_FORMAT_R16_UINT;
				break;
			case wi::graphics::Format::R16_SNORM:
				dds_format = dds::DXGI_FORMAT_R16_SNORM;
				break;
			case wi::graphics::Format::R16_SINT:
				dds_format = dds::DXGI_FORMAT_R16_SINT;
				break;
			case wi::graphics::Format::R8_UNORM:
				dds_format = dds::DXGI_FORMAT_R8_UNORM;
				break;
			case wi::graphics::Format::R8_UINT:
				dds_format = dds::DXGI_FORMAT_R8_UINT;
				break;
			case wi::graphics::Format::R8_SNORM:
				dds_format = dds::DXGI_FORMAT_R8_SNORM;
				break;
			case wi::graphics::Format::R8_SINT:
				dds_format = dds::DXGI_FORMAT_R8_SINT;
				break;
			case wi::graphics::Format::BC1_UNORM:
				dds_format = dds::DXGI_FORMAT_BC1_UNORM;
				break;
			case wi::graphics::Format::BC1_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_BC1_UNORM_SRGB;
				break;
			case wi::graphics::Format::BC2_UNORM:
				dds_format = dds::DXGI_FORMAT_BC2_UNORM;
				break;
			case wi::graphics::Format::BC2_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_BC2_UNORM_SRGB;
				break;
			case wi::graphics::Format::BC3_UNORM:
				dds_format = dds::DXGI_FORMAT_BC3_UNORM;
				break;
			case wi::graphics::Format::BC3_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_BC3_UNORM_SRGB;
				break;
			case wi::graphics::Format::BC4_UNORM:
				dds_format = dds::DXGI_FORMAT_BC4_UNORM;
				break;
			case wi::graphics::Format::BC4_SNORM:
				dds_format = dds::DXGI_FORMAT_BC4_SNORM;
				break;
			case wi::graphics::Format::BC5_UNORM:
				dds_format = dds::DXGI_FORMAT_BC5_UNORM;
				break;
			case wi::graphics::Format::BC5_SNORM:
				dds_format = dds::DXGI_FORMAT_BC5_SNORM;
				break;
			case wi::graphics::Format::BC6H_UF16:
				dds_format = dds::DXGI_FORMAT_BC6H_UF16;
				break;
			case wi::graphics::Format::BC6H_SF16:
				dds_format = dds::DXGI_FORMAT_BC6H_SF16;
				break;
			case wi::graphics::Format::BC7_UNORM:
				dds_format = dds::DXGI_FORMAT_BC7_UNORM;
				break;
			case wi::graphics::Format::BC7_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_BC7_UNORM_SRGB;
				break;
			default:
				assert(0);
				return false;
			}
			dds::write_header(
				filedata.data(),
				dds_format,
				desc.width,
				desc.type == TextureDesc::Type::TEXTURE_1D ? 0 : desc.height,
				desc.mip_levels,
				desc.array_size,
				has_flag(desc.misc_flags, ResourceMiscFlag::TEXTURECUBE),
				desc.type == TextureDesc::Type::TEXTURE_3D ? desc.depth : 0
			);
			std::memcpy(filedata.data() + sizeof(dds::Header), texturedata.data(), texturedata.size());
			return true;
		}

		const bool is_png = extension.compare("PNG") == 0;

		if (is_png)
		{
			if (desc.format == Format::R16_UNORM || desc.format == Format::R16_UINT)
			{
				// Specialized handling for 16-bit single channel PNG:
				wi::vector<uint8_t> src_bigendian = texturedata;
				uint16_t* dest = (uint16_t*)src_bigendian.data();
				for (uint32_t i = 0; i < desc.width * desc.height; ++i)
				{
					uint16_t r = dest[i];
					r = (r >> 8) | ((r & 0xFF) << 8); // little endian to big endian
					dest[i] = r;
				}
				unsigned error = lodepng::encode(filedata, src_bigendian, desc.width, desc.height, LCT_GREY, 16);
				return error == 0;
			}
			if (desc.format == Format::R16G16_UNORM || desc.format == Format::R16G16_UINT)
			{
				// Specialized handling for 16-bit PNG:
				//	Two channel RG data is expanded to RGBA (2-channel PNG is not good because that is interpreted as red and alpha)
				wi::vector<uint8_t> src_bigendian = texturedata;
				const uint32_t* src_rg = (const uint32_t*)src_bigendian.data();
				wi::vector<wi::Color16> dest_rgba(desc.width * desc.height);
				for (uint32_t i = 0; i < desc.width * desc.height; ++i)
				{
					uint32_t rg = src_rg[i];
					wi::Color16& rgba = dest_rgba[i];
					uint16_t r = rg & 0xFFFF;
					r = (r >> 8) | ((r & 0xFF) << 8); // little endian to big endian
					uint16_t g = (rg >> 16u) & 0xFFFF;
					g = (g >> 8) | ((g & 0xFF) << 8); // little endian to big endian
					rgba = wi::Color16(r, g, 0xFFFF, 0xFFFF);
				}
				unsigned error = lodepng::encode(filedata, (const unsigned char*)dest_rgba.data(), desc.width, desc.height, LCT_RGBA, 16);
				return error == 0;
			}
			if (desc.format == Format::R16G16B16A16_UNORM || desc.format == Format::R16G16B16A16_UINT)
			{
				// Specialized handling for 16-bit PNG:
				wi::vector<uint8_t> src_bigendian = texturedata;
				wi::Color16* dest = (wi::Color16*)src_bigendian.data();
				for (uint32_t i = 0; i < desc.width * desc.height; ++i)
				{
					wi::Color16 rgba = dest[i];
					uint16_t r = rgba.getR();
					r = (r >> 8) | ((r & 0xFF) << 8); // little endian to big endian
					uint16_t g = rgba.getG();
					g = (g >> 8) | ((g & 0xFF) << 8); // little endian to big endian
					uint16_t b = rgba.getB();
					b = (b >> 8) | ((b & 0xFF) << 8); // little endian to big endian
					uint16_t a = rgba.getA();
					a = (a >> 8) | ((a & 0xFF) << 8); // little endian to big endian
					rgba = wi::Color16(r, g, b, a);
					dest[i] = rgba;
				}
				unsigned error = lodepng::encode(filedata, src_bigendian, desc.width, desc.height, LCT_RGBA, 16);
				return error == 0;
			}
		}

		struct MipDesc
		{
			const uint8_t* address = nullptr;
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t depth = 0;
		};
		wi::vector<MipDesc> mips;
		mips.reserve(desc.mip_levels);

		uint32_t data_count = 0;
		uint32_t mip_width = desc.width;
		uint32_t mip_height = desc.height;
		uint32_t mip_depth = desc.depth;
		for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
		{
			MipDesc& mipdesc = mips.emplace_back();
			mipdesc.address = texturedata.data() + data_count * data_stride;
			data_count += mip_width * mip_height * mip_depth;
			mipdesc.width = mip_width;
			mipdesc.height = mip_height;
			mipdesc.depth = mip_depth;
			mip_width = std::max(1u, mip_width / 2);
			mip_height = std::max(1u, mip_height / 2);
			mip_depth = std::max(1u, mip_depth / 2);
		}

		int dst_channel_count = 4;
		if (desc.format == Format::R10G10B10A2_UNORM)
		{
			// This will be converted first to rgba8 before saving to common format:
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				uint32_t pixel = data32[i];
				float r = ((pixel >> 0) & 1023) / 1023.0f;
				float g = ((pixel >> 10) & 1023) / 1023.0f;
				float b = ((pixel >> 20) & 1023) / 1023.0f;
				float a = ((pixel >> 30) & 3) / 3.0f;

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::R32G32B32A32_FLOAT)
		{
			// This will be converted first to rgba8 before saving to common format:
			XMFLOAT4* dataSrc = (XMFLOAT4*)texturedata.data();
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				XMFLOAT4 pixel = dataSrc[i];
				float r = std::max(0.0f, std::min(pixel.x, 1.0f));
				float g = std::max(0.0f, std::min(pixel.y, 1.0f));
				float b = std::max(0.0f, std::min(pixel.z, 1.0f));
				float a = std::max(0.0f, std::min(pixel.w, 1.0f));

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::R16G16B16A16_FLOAT)
		{
			// This will be converted first to rgba8 before saving to common format:
			XMHALF4* dataSrc = (XMHALF4*)texturedata.data();
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				XMHALF4 pixel = dataSrc[i];
				float r = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.x), 1.0f));
				float g = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.y), 1.0f));
				float b = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.z), 1.0f));
				float a = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.w), 1.0f));

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::R16G16B16A16_UNORM || desc.format == Format::R16G16B16A16_UINT)
		{
			// This will be converted first to rgba8 before saving to common format:
			wi::Color16* dataSrc = (wi::Color16*)texturedata.data();
			wi::Color* data32 = (wi::Color*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				wi::Color16 pixel16 = dataSrc[i];
				data32[i] = wi::Color::fromFloat4(pixel16.toFloat4());
			}
		}
		else if (desc.format == Format::R11G11B10_FLOAT)
		{
			// This will be converted first to rgba8 before saving to common format:
			XMFLOAT3PK* dataSrc = (XMFLOAT3PK*)texturedata.data();
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				XMFLOAT3PK pixel = dataSrc[i];
				XMVECTOR V = XMLoadFloat3PK(&pixel);
				XMFLOAT3 pixel3;
				XMStoreFloat3(&pixel3, V);
				float r = std::max(0.0f, std::min(pixel3.x, 1.0f));
				float g = std::max(0.0f, std::min(pixel3.y, 1.0f));
				float b = std::max(0.0f, std::min(pixel3.z, 1.0f));
				float a = 1;

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::R9G9B9E5_SHAREDEXP)
		{
			// This will be converted first to rgba8 before saving to common format:
			XMFLOAT3SE* dataSrc = (XMFLOAT3SE*)texturedata.data();
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				XMFLOAT3SE pixel = dataSrc[i];
				XMVECTOR V = XMLoadFloat3SE(&pixel);
				XMFLOAT3 pixel3;
				XMStoreFloat3(&pixel3, V);
				float r = std::max(0.0f, std::min(pixel3.x, 1.0f));
				float g = std::max(0.0f, std::min(pixel3.y, 1.0f));
				float b = std::max(0.0f, std::min(pixel3.z, 1.0f));
				float a = 1;

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::B8G8R8A8_UNORM || desc.format == Format::B8G8R8A8_UNORM_SRGB)
		{
			// This will be converted first to rgba8 before saving to common format:
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				uint32_t pixel = data32[i];
				uint8_t b = (pixel >> 0u) & 0xFF;
				uint8_t g = (pixel >> 8u) & 0xFF;
				uint8_t r = (pixel >> 16u) & 0xFF;
				uint8_t a = (pixel >> 24u) & 0xFF;
				data32[i] = r | (g << 8u) | (b << 16u) | (a << 24u);
			}
		}
		else if (desc.format == Format::R8_UNORM)
		{
			// This can be saved by reducing target channel count, no conversion needed
			dst_channel_count = 1;
		}
		else if (desc.format == Format::R8G8_UNORM)
		{
			// This can be saved by reducing target channel count, no conversion needed
			dst_channel_count = 2;
		}
		else
		{
			assert(desc.format == Format::R8G8B8A8_UNORM || desc.format == Format::R8G8B8A8_UNORM_SRGB); // If you need to save other texture format, implement data conversion for it
		}

		if (!extension.compare("ICO"))
		{
			const uint32_t minsize = 32;

			size_t filesize = sizeof(ico::ICONDIR);

			ico::ICONDIR icondir = { 0,1,0 };

			for (auto& mip : mips)
			{
				if (mip.width > 256 || mip.height > 256)
					continue;
				if (mip.width < minsize || mip.height < minsize)
					break;
				icondir.idCount++;
				filesize += sizeof(ico::ICONDIRENTRY);
			}

			if (icondir.idCount < 1)
			{
				wilog_assert(0, "No valid images were found that can be added to ICO file format!");
				return false;
			}

			uint32_t imageDataOffset = (uint32_t)filesize;

			for (auto& mip : mips)
			{
				if (mip.width > 256 || mip.height > 256)
					continue;
				if (mip.width < minsize || mip.height < minsize)
					break;
				const uint32_t pixelCount = mip.width * mip.height;
				const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
				const uint32_t maskSize = ((mip.width + 7) / 8) * mip.height; // 1-bit mask, padded to byte
				const uint32_t imageDataSize = sizeof(ico::BITMAPINFOHEADER) + rgbDataSize + maskSize;
				filesize += imageDataSize;
			}

			filedata.resize(filesize);
			uint8_t* ptr = filedata.data();

			std::memcpy(ptr, &icondir, sizeof(ico::ICONDIR));
			ptr += sizeof(ico::ICONDIR);

			for (auto& mip : mips)
			{
				if (mip.width > 256 || mip.height > 256)
					continue;
				if (mip.width < minsize || mip.height < minsize)
					break;

				const uint32_t pixelCount = mip.width * mip.height;
				const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
				const uint32_t maskSize = ((mip.width + 7) / 8) * mip.height; // 1-bit mask, padded to byte
				const uint32_t imageDataSize = sizeof(ico::BITMAPINFOHEADER) + rgbDataSize + maskSize;

				ico::ICONDIRENTRY iconEntry = {
					static_cast<uint8_t>(mip.width > 255 ? 0 : mip.width), // Width (0 for 256+)
					static_cast<uint8_t>(mip.height > 255 ? 0 : mip.height), // Height (0 for 256+)
					0, // Color count (0 for 32-bit)
					0, // Reserved
					1, // Color planes
					32, // Bits per pixel
					imageDataSize, // Size of image data
					imageDataOffset // Offset to image data
				};
				std::memcpy(ptr, &iconEntry, sizeof(ico::ICONDIRENTRY));
				ptr += sizeof(ico::ICONDIRENTRY);
				imageDataOffset += imageDataSize;
			}

			for (auto& mip : mips)
			{
				if (mip.width > 256 || mip.height > 256)
					continue;
				if (mip.width < minsize || mip.height < minsize)
					break;

				const uint32_t pixelCount = mip.width * mip.height;
				const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
				const uint32_t maskSize = ((mip.width + 7) / 8) * mip.height; // 1-bit mask, padded to byte

				ico::BITMAPINFOHEADER bmpHeader = {
					sizeof(ico::BITMAPINFOHEADER), // Size of header
					int32_t(mip.width), // Width
					int32_t(mip.height * 2), // Height (doubled for XOR + AND mask)
					1, // Planes
					32, // Bits per pixel
					0, // No compression
					rgbDataSize + maskSize, // Image size
					0, // X pixels per meter
					0, // Y pixels per meter
					0, // Colors used
					0  // Important colors
				};
				std::memcpy(ptr, &bmpHeader, sizeof(ico::BITMAPINFOHEADER));
				ptr += sizeof(ico::BITMAPINFOHEADER);

				// Convert RGBA to BGRA and write XOR mask (flipped vertically)
				for (uint32_t y = mip.height; y > 0; --y)
				{
					const uint8_t* src = mip.address + (y - 1) * mip.width * 4;
					for (uint32_t x = 0; x < mip.width; ++x)
					{
						// Convert RGBA to BGRA
						ptr[0] = src[2]; // B
						ptr[1] = src[1]; // G
						ptr[2] = src[0]; // R
						ptr[3] = src[3]; // A
						ptr += 4;
						src += 4;
					}
				}

				// Write AND mask (1-bit transparency mask)
				for (uint32_t y = mip.height; y > 0; --y)
				{
					const uint8_t* src = mip.address + (y - 1) * mip.width * 4;
					for (uint32_t x = 0; x < mip.width; x += 8)
					{
						uint8_t maskByte = 0;
						for (uint32_t bit = 0; bit < 8 && (x + bit) < mip.width; ++bit)
						{
							// Set bit to 0 if pixel is opaque (alpha > 0), 1 if transparent
							if (src[(x + bit) * 4 + 3] == 0)
							{
								maskByte |= (1 << (7 - bit));
							}
						}
						*ptr++ = maskByte;
					}
				}
			}

			return true;
		}

		int write_result = 0;

		filedata.clear();
		stbi_write_func* func = [](void* context, void* data, int size) {
			wi::vector<uint8_t>& filedata = *(wi::vector<uint8_t>*)context;
			for (int i = 0; i < size; ++i)
			{
				filedata.push_back(*((uint8_t*)data + i));
			}
		};

		static int mip_request = 0; // you can use this while debugging to write specific mip level to file (todo: option param?)
		const MipDesc& mip = mips[mip_request];

		if (is_png)
		{
			write_result = stbi_write_png_to_func(func, &filedata, (int)mip.width, (int)mip.height, dst_channel_count, mip.address, 0);
		}
		else if (!extension.compare("JPG") || !extension.compare("JPEG"))
		{
			write_result = stbi_write_jpg_to_func(func, &filedata, (int)mip.width, (int)mip.height, dst_channel_count, mip.address, 100);
		}
		else if (!extension.compare("TGA"))
		{
			write_result = stbi_write_tga_to_func(func, &filedata, (int)mip.width, (int)mip.height, dst_channel_count, mip.address);
		}
		else if (!extension.compare("BMP"))
		{
			write_result = stbi_write_bmp_to_func(func, &filedata, (int)mip.width, (int)mip.height, dst_channel_count, mip.address);
		}
		else if (!extension.compare("H"))
		{
			const size_t datasize = mip.width * mip.height * sizeof(wi::Color);
			std::string ss;
			ss += "struct EmbeddedImage {\n";
			ss += "const unsigned int width = " + std::to_string(mip.width) + ";\n";
			ss += "const unsigned int height = " + std::to_string(mip.height) + ";\n";
			ss += "const unsigned int bytes_per_pixel = 4;\n";
			ss += "const char comment[256] = \"RGBA Image Header Generated By Wicked Engine\";\n";
			ss += "const unsigned char pixel_data[" + std::to_string(mip.width) + " * " + std::to_string(mip.height) + " * 4] = {";
			for (size_t i = 0; i < datasize; ++i)
			{
				if (i % 32 == 0)
				{
					ss += "\n";
				}
				ss += std::to_string((uint32_t)mip.address[i]) + ",";
			}
			ss += "\n};\n};\n";
			filedata.resize(ss.size());
			std::memcpy(filedata.data(), ss.c_str(), ss.length());
			return true;
		}
		else if (!extension.compare("RAW"))
		{
			filedata.resize(mip.width* mip.height * sizeof(wi::Color));
			std::memcpy(filedata.data(), mip.address, filedata.size());
			return true;
		}
		else
		{
			assert(0 && "Unsupported extension");
		}

		return write_result != 0;
	}

	bool saveTextureToFile(const wi::graphics::Texture& texture, const std::string& fileName)
	{
		using namespace wi::graphics;
		TextureDesc desc = texture.GetDesc();
		wi::vector<uint8_t> data;
		if (saveTextureToMemory(texture, data))
		{
			return saveTextureToFile(data, desc, fileName);
		}
		return false;
	}

	bool saveTextureToFile(const wi::vector<uint8_t>& texturedata, const wi::graphics::TextureDesc& desc, const std::string& fileName)
	{
		using namespace wi::graphics;

		std::string ext = GetExtensionFromFileName(fileName);
		wi::vector<uint8_t> filedata;
		if (saveTextureToMemoryFile(texturedata, desc, ext, filedata))
		{
			return FileWrite(fileName, filedata.data(), filedata.size());
		}

		return false;
	}

	bool saveBufferToMemory(const wi::graphics::GPUBuffer& buffer, wi::vector<uint8_t>& data)
	{
		using namespace wi::graphics;
		GraphicsDevice* device = GetDevice();

		GPUBufferDesc desc = buffer.desc;
		desc.usage = wi::graphics::Usage::READBACK;
		desc.bind_flags = {};
		desc.misc_flags = {};
		GPUBuffer staging;
		bool success = device->CreateBuffer(&desc, nullptr, &staging);
		assert(success);

		if (success)
		{
			CommandList cmd = device->BeginCommandList();

			device->Barrier(GPUBarrier::Memory(), cmd);
			device->CopyResource(&staging, &buffer, cmd);
			device->Barrier(GPUBarrier::Memory(), cmd);

			device->SubmitCommandLists();
			device->WaitForGPU();

			data.reserve(staging.mapped_size);
			for (size_t i = 0; i < staging.mapped_size; ++i)
			{
				data.push_back(((uint8_t*)staging.mapped_data)[i]);
			}
		}

		return success;
	}

	bool CreateCursorFromTexture(const wi::graphics::Texture& texture, int hotspotX, int hotspotY, wi::vector<uint8_t>& data)
	{
		if (!saveTextureToMemoryFile(texture, "ico", data))
			return false;

		// rewrite ICO structure for CUR structure:
		ico::ICONDIR* icondir = (ico::ICONDIR*)data.data();
		icondir->idType = 2;

		uint32_t baseWidth = texture.desc.width;
		uint32_t baseHeight = texture.desc.height;

		for (uint16_t i = 0; i < icondir->idCount; ++i)
		{
			ico::ICONDIRENTRY* icondirentry = (ico::ICONDIRENTRY*)(data.data() + sizeof(ico::ICONDIR)) + i;
			icondirentry->wPlanes = (uint16_t)hotspotX / (baseWidth / (icondirentry->bWidth == 0 ? 256 : icondirentry->bWidth));
			icondirentry->wBitCount = (uint16_t)hotspotY / (baseHeight / (icondirentry->bHeight == 0 ? 256 : icondirentry->bHeight));
		}

		return true;
	}

	std::string getCurrentDateTimeAsString()
	{
		time_t t = std::time(nullptr);
		struct tm time_info;
#ifdef _WIN32
		localtime_s(&time_info, &t);
#else
		localtime(&t);
#endif
		std::stringstream ss("");
		ss << std::put_time(&time_info, "%d-%m-%Y %H-%M-%S");
		return ss.str();
	}

	void SplitPath(const std::string& fullPath, std::string& dir, std::string& fileName)
	{
		size_t found;
		found = fullPath.find_last_of("/\\");
		dir = fullPath.substr(0, found + 1);
		fileName = fullPath.substr(found + 1);
	}

	std::string GetFileNameFromPath(const std::string& fullPath)
	{
		if (fullPath.empty())
		{
			return fullPath;
		}

		std::string ret, empty;
		SplitPath(fullPath, empty, ret);
		return ret;
	}

	std::string GetDirectoryFromPath(const std::string& fullPath)
	{
		if (fullPath.empty())
		{
			return fullPath;
		}

		std::string ret, empty;
		SplitPath(fullPath, ret, empty);
		return ret;
	}

	std::string GetExtensionFromFileName(const std::string& filename)
	{
		size_t idx = filename.rfind('.');

		if (idx != std::string::npos)
		{
			std::string extension = filename.substr(idx + 1);
			return extension;
		}

		// No extension found
		return "";
	}

	std::string ReplaceExtension(const std::string& filename, const std::string& extension)
	{
		size_t idx = filename.rfind('.');

		if (idx == std::string::npos)
		{
			// extension not found, append it:
			return filename + "." + extension;
		}
		return filename.substr(0, idx + 1) + extension;
	}

	std::string ForceExtension(const std::string& filename, const std::string& extension)
	{
		std::string ext = "." + extension;
		if (ext.length() > filename.length())
			return filename + ext;

		if (filename.substr(filename.length() - ext.length()).compare(ext))
		{
			return filename + ext;
		}
		return filename;
	}

	std::string RemoveExtension(const std::string& filename)
	{
		size_t idx = filename.rfind('.');

		if (idx == std::string::npos)
		{
			// extension not found:
			return filename;
		}
		return filename.substr(0, idx);
	}

#ifdef _WIN32
	// On windows we need to expand UTF8 strings to UTF16 when passing it to WinAPI:
	std::wstring ToNativeString(const std::string& fileName)
	{
		std::wstring fileName_wide;
		StringConvert(fileName, fileName_wide);
		return fileName_wide;
	}
#else
#define ToNativeString(x) (x)
#endif // _WIN32

	std::string FromWString(const std::wstring& fileName)
	{
		std::string fileName_u8;
		StringConvert(fileName, fileName_u8);
		return fileName_u8;
	}

	std::string GetPathRelative(const std::string& rootdir, const std::string& path)
	{
		std::string ret = path;
		MakePathRelative(rootdir, ret);
		return ret;
	}

	void MakePathRelative(const std::string& rootdir, std::string& path)
	{
		if (rootdir.empty() || path.empty())
		{
			return;
		}

		std::filesystem::path filepath = ToNativeString(path);
		if (filepath.is_absolute())
		{
			std::filesystem::path rootpath = ToNativeString(rootdir);
			std::filesystem::path relative = std::filesystem::relative(filepath, rootpath);
			if (!relative.empty())
			{
				StringConvert(relative.generic_wstring(), path);
			}
		}

	}

	void MakePathAbsolute(std::string& path)
	{
		std::filesystem::path absolute = std::filesystem::absolute(ToNativeString(path));
		if (!absolute.empty())
		{
			StringConvert(absolute.generic_wstring(), path);
		}
	}

	std::string BackslashToForwardSlash(const std::string& str)
	{
		std::string ret = str;
		std::replace(ret.begin(), ret.end(), '\\', '/');
		return ret;
	}

	void DirectoryCreate(const std::string& path)
	{
		std::filesystem::create_directories(ToNativeString(path));
	}

	size_t FileSize(const std::string& fileName)
	{
#if defined(PLATFORM_LINUX) || defined(PLATFORM_PS5)
		std::string filepath = fileName;
		std::replace(filepath.begin(), filepath.end(), '\\', '/'); // Linux cannot handle backslash in file path, need to convert it to forward slash
		std::ifstream file(filepath, std::ios::binary | std::ios::ate);
#else
		std::ifstream file(ToNativeString(fileName), std::ios::binary | std::ios::ate);
#endif // PLATFORM_LINUX || PLATFORM_PS5

		if (file.is_open())
		{
			size_t dataSize = (size_t)file.tellg();
			file.close();
			return dataSize;
		}
		return 0;
	}

	template<template<typename T, typename A> typename vector_interface>
	bool FileRead_Impl(const std::string& fileName, vector_interface<uint8_t, std::allocator<uint8_t>>& data, size_t max_read, size_t offset)
	{
#if defined(PLATFORM_LINUX) || defined(PLATFORM_PS5)
		std::string filepath = fileName;
		std::replace(filepath.begin(), filepath.end(), '\\', '/'); // Linux cannot handle backslash in file path, need to convert it to forward slash
		std::ifstream file(filepath, std::ios::binary | std::ios::ate);
#else
		std::ifstream file(ToNativeString(fileName), std::ios::binary | std::ios::ate);
#endif // PLATFORM_LINUX || PLATFORM_PS5

		if (file.is_open())
		{
			size_t dataSize = (size_t)file.tellg() - offset;
			dataSize = std::min(dataSize, max_read);
			file.seekg((std::streampos)offset);
			data.resize(dataSize);
			file.read((char*)data.data(), dataSize);
			file.close();
			return true;
		}

		wi::backlog::post("File not found: " + fileName, wi::backlog::LogLevel::Warning);
		return false;
	}
	bool FileRead(const std::string& fileName, wi::vector<uint8_t>& data, size_t max_read, size_t offset)
	{
		return FileRead_Impl(fileName, data, max_read, offset);
	}
#if WI_VECTOR_TYPE
	bool FileRead(const std::string& fileName, std::vector<uint8_t>& data, size_t max_read, size_t offset)
	{
		return FileRead_Impl(fileName, data, max_read, offset);
	}
#endif // WI_VECTOR_TYPE

	bool FileWrite(const std::string& fileName, const uint8_t* data, size_t size)
	{
		if (size <= 0)
		{
			return false;
		}

		std::ofstream file(ToNativeString(fileName), std::ios::binary | std::ios::trunc);
		if (file.is_open())
		{
			file.write((const char*)data, (std::streamsize)size);
			file.close();
			return true;
		}

		return false;
	}

	bool FileExists(const std::string& fileName)
	{
		bool exists = std::filesystem::exists(ToNativeString(fileName));
		return exists;
	}

	bool DirectoryExists(const std::string& fileName)
	{
		bool exists = std::filesystem::exists(ToNativeString(fileName));
		return exists;
	}

	uint64_t FileTimestamp(const std::string& fileName)
	{
		if (!FileExists(fileName))
			return 0;
		auto tim = std::filesystem::last_write_time(ToNativeString(fileName));
		return std::chrono::duration_cast<std::chrono::duration<uint64_t>>(tim.time_since_epoch()).count();
	}

	bool FileCopy(const std::string& filename_src, const std::string& filename_dst)
	{
		return std::filesystem::copy_file(ToNativeString(filename_src), ToNativeString(filename_dst), std::filesystem::copy_options::overwrite_existing);
	}

	std::string GetTempDirectoryPath()
	{
#if defined(PLATFORM_XBOX) || defined(PLATFORM_PS5)
		return "";
#else
		auto path = std::filesystem::temp_directory_path();
		return FromWString(path.generic_wstring());
#endif // PLATFORM_XBOX || PLATFORM_PS5
	}

	std::string GetCacheDirectoryPath()
	{
		#ifdef PLATFORM_LINUX
			const char* xdg_cache = std::getenv("XDG_CACHE_HOME");
			if (xdg_cache == nullptr || *xdg_cache == '\0') {
				const char* home = std::getenv("HOME");
				if (home != nullptr) {
					return std::string(home) + "/.cache";
				} else {
					// shouldn't happen, just to be safe
					return GetTempDirectoryPath();
				}
			} else {
				return xdg_cache;
			}
		#else
			return GetTempDirectoryPath();
		#endif
	}

	std::string GetCurrentPath()
	{
#ifdef PLATFORM_PS5
		return "/app0";
#else
		auto path = std::filesystem::current_path();
		return FromWString(path.generic_wstring());
#endif // PLATFORM_PS5
	}

	std::string GetExecutablePath()
	{
#ifdef _WIN32
		wchar_t wstr[1024] = {};
		GetModuleFileName(NULL, wstr, arraysize(wstr));
		char str[1024] = {};
		StringConvert(wstr, str, arraysize(str));
		return str;
#else
		return std::filesystem::canonical("/proc/self/exe").string();
#endif // _WIN32
	}

	void FileDialog(const FileDialogParams& params, std::function<void(std::string fileName)> onSuccess)
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		std::thread([=] {

			wchar_t szFile[4096];

			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.nFilterIndex = 1;

			// Slightly convoluted way to create the filter.
			//	First string is description, ended by '\0'
			//	Second string is extensions, each separated by ';' and at the end of all, a '\0'
			//	Then the whole container string is closed with an other '\0'
			//		For example: "model files\0*.model;*.obj;\0"  <-- this string literal has "model files" as description and two accepted extensions "model" and "obj"
			wi::vector<wchar_t> filter;
			filter.reserve(256);
			{
				for (auto& x : params.description)
				{
					filter.push_back(x);
				}
				filter.push_back(0);

				for (auto& x : params.extensions)
				{
					filter.push_back('*');
					filter.push_back('.');
					for (auto& y : x)
					{
						filter.push_back(y);
					}
					filter.push_back(';');
				}
				filter.push_back(0);
				filter.push_back(0);
			}
			ofn.lpstrFilter = filter.data();


			BOOL ok = FALSE;
			switch (params.type)
			{
			case FileDialogParams::OPEN:
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				ofn.Flags |= OFN_EXPLORER;
				ofn.Flags |= OFN_NOCHANGEDIR;
				if (params.multiselect)
				{
					ofn.Flags |= OFN_ALLOWMULTISELECT;
				}
				ok = GetOpenFileName(&ofn) == TRUE;
				break;
			case FileDialogParams::SAVE:
				ofn.Flags = OFN_OVERWRITEPROMPT;
				ofn.Flags |= OFN_NOCHANGEDIR;
				ok = GetSaveFileName(&ofn) == TRUE;
				break;
			}

			if (ok)
			{
				const wchar_t* p = szFile;
				std::wstring directory = p;
				p += directory.length() + 1;
				if (*p == L'\0')
				{
					// Single file
					std::string result_filename;
					StringConvert(directory, result_filename);
					onSuccess(BackslashToForwardSlash(result_filename));
				}
				else
				{
					// Multiple files
					while (*p)
					{
						std::wstring fullPath = directory + L"\\" + p;
						p += wcslen(p) + 1;
						std::string result_filename;
						StringConvert(fullPath, result_filename);
						onSuccess(BackslashToForwardSlash(result_filename));
					}
				}
			}

			}).detach();
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef PLATFORM_LINUX
		if (!pfd::settings::available())
		{
			wilog_messagebox("[wi::helper::FileDialog()] No file dialog backend available!");
			return;
		}

		std::vector<std::string> extensions = {params.description, ""};
		int extcount = 0;
		for (auto& x : params.extensions)
		{
			extensions[1] += "*." + toLower(x);
			extensions[1] += " ";
			extensions[1] += "*." + toUpper(x);
			if (extcount < params.extensions.size() - 1)
			{
				extensions[1] += " ";
			}
			extcount++;
		}

		switch (params.type) {
			case FileDialogParams::OPEN:
			{
				pfd::opt options = pfd::opt::none;
				if (params.multiselect)
				{
					options = options | pfd::opt::multiselect;
				}
				std::vector<std::string> selection = pfd::open_file("Open file", std::filesystem::current_path().string(), extensions, options).result();
				if (!selection.empty())
				{
					for (auto& x : selection)
					{
						onSuccess(x);
					}
				}
				break;
			}
			case FileDialogParams::SAVE:
			{
				std::string destination = pfd::save_file("Save file", std::filesystem::current_path().string(), extensions).result();
				if (!destination.empty())
				{
					onSuccess(destination);
				}
				break;
			}
		}
#endif // PLATFORM_LINUX
	}

	std::string FolderDialog(const std::string& description)
	{
#ifdef __SCE__
		return "";
#else
		if (!pfd::settings::available())
		{
			messageBox("No dialog backend available!", "Error!");
			return "";
		}
		return pfd::select_folder(description).result();
#endif // __SCE__
	}

	void GetFileNamesInDirectory(const std::string& directory, std::function<void(std::string fileName)> onSuccess, const std::string& filter_extension)
	{
		std::filesystem::path directory_path = ToNativeString(directory);
		if (!std::filesystem::exists(directory_path))
			return;

		for (const auto& entry : std::filesystem::directory_iterator(directory_path))
		{
			if (entry.is_directory())
				continue;
			std::string filename = FromWString(entry.path().filename().generic_wstring());
			if (filter_extension.empty() || wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename)).compare(wi::helper::toUpper(filter_extension)) == 0)
			{
				onSuccess(directory + filename);
			}
		}
	}

	void GetFolderNamesInDirectory(const std::string& directory, std::function<void(std::string folderName)> onSuccess)
	{
		std::filesystem::path directory_path = ToNativeString(directory);
		if (!std::filesystem::exists(directory_path))
			return;

		for (const auto& entry : std::filesystem::directory_iterator(directory_path))
		{
			if (!entry.is_directory())
				continue;
			std::string filename = FromWString(entry.path().filename().generic_wstring());
			onSuccess(directory + filename);
		}
	}

	bool Bin2H(const uint8_t* data, size_t size, const std::string& dst_filename, const char* dataName)
	{
		std::string ss;
		ss += "const unsigned char ";
		ss += dataName ;
		ss += "[] = {";
		for (size_t i = 0; i < size; ++i)
		{
			if (i % 32 == 0)
			{
				ss += "\n";
			}
			ss += std::to_string((uint32_t)data[i]) + ",";
		}
		ss += "\n};\n";
		return FileWrite(dst_filename, (uint8_t*)ss.c_str(), ss.length());
	}

	bool Bin2CPP(const uint8_t* data, size_t size, const std::string& dst_filename, const char* dataName)
	{
		std::string ss;
		ss += "extern const unsigned char ";
		ss += dataName;
		ss += "[] = {";
		for (size_t i = 0; i < size; ++i)
		{
			if (i % 32 == 0)
			{
				ss += "\n";
			}
			ss += std::to_string((uint32_t)data[i]) + ",";
		}
		ss += "\n};\n";
		ss += "extern const unsigned long long ";
		ss += dataName;
		ss += "_size = sizeof(";
		ss += dataName;
		ss += ");";
		return FileWrite(dst_filename, (uint8_t*)ss.c_str(), ss.length());
	}

	void StringConvert(const std::string& from, std::wstring& to)
	{
		to.clear();
		size_t i = 0;
		while (i < from.size())
		{
			uint32_t codepoint = 0;
			unsigned char c = from[i];

			if (c < 0x80)
			{
				codepoint = c;
				i += 1;
			}
			else if ((c & 0xE0) == 0xC0)
			{
				if (i + 1 >= from.size())
					break;
				codepoint = ((c & 0x1F) << 6) | (from[i + 1] & 0x3F);
				i += 2;
			}
			else if ((c & 0xF0) == 0xE0)
			{
				if (i + 2 >= from.size())
					break;
				codepoint = ((c & 0x0F) << 12) | ((from[i + 1] & 0x3F) << 6) | (from[i + 2] & 0x3F);
				i += 3;
			}
			else if ((c & 0xF8) == 0xF0)
			{
				if (i + 3 >= from.size())
					break;
				codepoint = ((c & 0x07) << 18) | ((from[i + 1] & 0x3F) << 12) | ((from[i + 2] & 0x3F) << 6) | (from[i + 3] & 0x3F);
				i += 4;
			}
			else
			{
				++i;
				continue;
			}

			if constexpr (sizeof(wchar_t) >= 4)
			{
				to += static_cast<wchar_t>(codepoint);
			}
			else
			{
				if (codepoint <= 0xFFFF)
				{
					to += static_cast<wchar_t>(codepoint);
				}
				else
				{
					codepoint -= 0x10000;
					to += static_cast<wchar_t>((codepoint >> 10) + 0xD800);
					to += static_cast<wchar_t>((codepoint & 0x3FF) + 0xDC00);
				}
			}
		}
	}

	void StringConvert(const std::wstring& from, std::string& to)
	{
		to.clear();
		for (size_t i = 0; i < from.size(); ++i)
		{
			uint32_t codepoint = 0;
			wchar_t wc = from[i];

			if constexpr (sizeof(wchar_t) >= 4)
			{
				codepoint = static_cast<uint32_t>(wc);
			}
			else
			{
				if (wc >= 0xD800 && wc <= 0xDBFF)
				{
					if (i + 1 < from.size())
					{
						wchar_t wc_low = from[i + 1];
						if (wc_low >= 0xDC00 && wc_low <= 0xDFFF)
						{
							codepoint = ((static_cast<uint32_t>(wc - 0xD800) << 10) | (static_cast<uint32_t>(wc_low - 0xDC00))) + 0x10000;
							++i;
						}
					}
				}
				else
				{
					codepoint = static_cast<uint32_t>(wc);
				}
			}

			if (codepoint <= 0x7F)
			{
				to += static_cast<char>(codepoint);
			}
			else if (codepoint <= 0x7FF)
			{
				to += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
				to += static_cast<char>(0x80 | (codepoint & 0x3F));
			}
			else if (codepoint <= 0xFFFF)
			{
				to += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
				to += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				to += static_cast<char>(0x80 | (codepoint & 0x3F));
			}
			else if (codepoint <= 0x10FFFF)
			{
				to += static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
				to += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
				to += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				to += static_cast<char>(0x80 | (codepoint & 0x3F));
			}
		}
	}

	int StringConvert(const char* from, wchar_t* to, int dest_size_in_characters)
	{
		if (!from || !to || dest_size_in_characters <= 0)
			return 0;

		const unsigned char* src = reinterpret_cast<const unsigned char*>(from);
		int written = 0;

		while (*src && written < dest_size_in_characters - 1)
		{
			uint32_t codepoint = 0;
			unsigned char c = *src;

			if (c < 0x80)
			{
				codepoint = c;
				++src;
			}
			else if ((c & 0xE0) == 0xC0)
			{
				if (!src[1])
					break;
				codepoint = ((c & 0x1F) << 6) | (src[1] & 0x3F);
				src += 2;
			}
			else if ((c & 0xF0) == 0xE0)
			{
				if (!src[1] || !src[2])
					break;
				codepoint = ((c & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);
				src += 3;
			}
			else if ((c & 0xF8) == 0xF0)
			{
				if (!src[1] || !src[2] || !src[3])
					break;
				codepoint = ((c & 0x07) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F);
				src += 4;
			}
			else
			{
				++src;
				continue;
			}

			if constexpr (sizeof(wchar_t) >= 4)
			{
				to[written++] = static_cast<wchar_t>(codepoint);
			}
			else
			{
				if (codepoint <= 0xFFFF)
				{
					to[written++] = static_cast<wchar_t>(codepoint);
				}
				else
				{
					if (written + 1 >= dest_size_in_characters - 1)
						break;
					codepoint -= 0x10000;
					to[written++] = static_cast<wchar_t>((codepoint >> 10) + 0xD800);
					to[written++] = static_cast<wchar_t>((codepoint & 0x3FF) + 0xDC00);
				}
			}
		}

		to[written] = 0;
		return written;
	}

	int StringConvert(const wchar_t* from, char* to, int dest_size_in_characters)
	{
		if (!from || !to || dest_size_in_characters <= 0)
			return 0;

		int written = 0;
		for (int i = 0; from[i] != 0 && written < dest_size_in_characters - 1; ++i)
		{
			uint32_t codepoint = 0;
			wchar_t wc = from[i];

			if constexpr (sizeof(wchar_t) >= 4)
			{
				codepoint = static_cast<uint32_t>(wc);
			}
			else
			{
				if (wc >= 0xD800 && wc <= 0xDBFF)
				{
					wchar_t wc_low = from[i + 1];
					if (wc_low >= 0xDC00 && wc_low <= 0xDFFF)
					{
						codepoint = ((static_cast<uint32_t>(wc - 0xD800) << 10) | (static_cast<uint32_t>(wc_low - 0xDC00))) + 0x10000;
						++i;
					}
					else
					{
						codepoint = wc;
					}
				}
				else
				{
					codepoint = static_cast<uint32_t>(wc);
				}
			}

			if (codepoint <= 0x7F)
			{
				if (written + 1 >= dest_size_in_characters)
					break;
				to[written++] = static_cast<char>(codepoint);
			}
			else if (codepoint <= 0x7FF)
			{
				if (written + 2 >= dest_size_in_characters)
					break;
				to[written++] = static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
				to[written++] = static_cast<char>(0x80 | (codepoint & 0x3F));
			}
			else if (codepoint <= 0xFFFF)
			{
				if (written + 3 >= dest_size_in_characters)
					break;
				to[written++] = static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
				to[written++] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				to[written++] = static_cast<char>(0x80 | (codepoint & 0x3F));
			}
			else if (codepoint <= 0x10FFFF)
			{
				if (written + 4 >= dest_size_in_characters)
					break;
				to[written++] = static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
				to[written++] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
				to[written++] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				to[written++] = static_cast<char>(0x80 | (codepoint & 0x3F));
			}
		}

		if (written < dest_size_in_characters)
			to[written] = '\0';

		return written;
	}

	std::string StringRemoveTrailingWhitespaces(const std::string& str)
	{
		size_t found;
		found = str.find_last_not_of(" /t");
		if (found == std::string::npos)
			return str;
		return str.substr(0, found + 2);
	}

	void DebugOut(const std::string& str, DebugLevel level)
	{
#ifdef _WIN32
		std::wstring wstr = ToNativeString(str);
		OutputDebugString(wstr.c_str());
#else
		switch (level)
		{
		default:
		case DebugLevel::Normal:
			std::cout << str;
			std::flush(std::cout);
			break;
		case DebugLevel::Warning:
			std::clog << str;
			std::flush(std::clog);
			break;
		case DebugLevel::Error:
			std::cerr << str;
			std::flush(std::cerr);
			break;
	}
#endif // _WIN32
	}

	void Sleep(float milliseconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds((int)milliseconds));
	}

	void Spin(float milliseconds)
	{
		const std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		const double seconds = double(milliseconds) / 1000.0;
		while (std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - t1).count() < seconds);
	}

	void QuickSleep(float milliseconds)
	{
		const std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
		const double seconds = double(milliseconds) / 1000.0;
		const int sleep_millisec_accuracy = 1;
		const double sleep_sec_accuracy = double(sleep_millisec_accuracy) / 1000.0;
		while (std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - t1).count() < seconds)
		{
			if (seconds - (std::chrono::high_resolution_clock::now() - t1).count() > sleep_sec_accuracy)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(sleep_millisec_accuracy));
			}
		}
	}

	void OpenUrl(const std::string& url)
	{

#ifdef PLATFORM_WINDOWS_DESKTOP
		std::string op = "start \"\" \"" + url + "\"";
		int status = system(op.c_str());
		wi::backlog::post("wi::helper::OpenUrl(" + url + ") returned status: " + std::to_string(status));
		return;
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef PLATFORM_LINUX
		std::string op = "xdg-open " + url;
		int status = system(op.c_str());
		wi::backlog::post("wi::helper::OpenUrl(" + url + ") returned status: " + std::to_string(status));
		return;
#endif // PLATFORM_WINDOWS_DESKTOP

		wi::backlog::post("wi::helper::OpenUrl(" + url + "): not implemented for this operating system!", wi::backlog::LogLevel::Warning);
	}

	MemoryUsage GetMemoryUsage()
	{
		MemoryUsage mem;
#if defined(_WIN32)
		// https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
		MEMORYSTATUSEX memInfo = {};
		memInfo.dwLength = sizeof(MEMORYSTATUSEX);
		BOOL ret = GlobalMemoryStatusEx(&memInfo);
		assert(ret);
		mem.total_physical = memInfo.ullTotalPhys;
		mem.total_virtual = memInfo.ullTotalVirtual;

		PROCESS_MEMORY_COUNTERS_EX pmc = {};
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		mem.process_physical = pmc.WorkingSetSize;
		mem.process_virtual = pmc.PrivateUsage;
#elif defined(PLATFORM_LINUX)
		struct sysinfo info;
		constexpr int PAGE_SIZE = 4096;
		if (sysinfo(&info) == 0)
		{
			unsigned long phys = info.totalram - info.totalswap;
			mem.total_physical = phys * info.mem_unit;
			mem.total_virtual = info.totalswap * info.mem_unit;
		}
		unsigned long l;
		std::ifstream statm("/proc/self/statm");
		// Format of statm:
		// size resident shared trs lrs drs dt
		// see linux Documentation/filesystems/proc.rst

		// we want "resident", the second number, so just read the first one
		// and discard it
		statm >> l;
		statm >> l;
		mem.process_physical = l * PAGE_SIZE;
		// there doesn't seem to be an easy way to determine
		// swapped out memory
#elif defined(PLATFORM_PS5)
		wi::graphics::GraphicsDevice::MemoryUsage gpumem = wi::graphics::GetDevice()->GetMemoryUsage();
		mem.process_physical = mem.total_physical = gpumem.budget;
		mem.process_virtual = mem.total_virtual = gpumem.usage;
#endif // defined(_WIN32)
		return mem;
	}

	std::string GetMemorySizeText(size_t sizeInBytes)
	{
		std::stringstream ss;
		ss << std::fixed << std::setprecision(1);
		if (sizeInBytes >= 1024ull * 1024ull * 1024ull)
		{
			ss << (double)sizeInBytes / 1024.0 / 1024.0 / 1024.0 << " GB";
		}
		else if (sizeInBytes >= 1024ull * 1024ull)
		{
			ss << (double)sizeInBytes / 1024.0 / 1024.0 << " MB";
		}
		else if (sizeInBytes >= 1024ull)
		{
			ss << (double)sizeInBytes / 1024.0 << " KB";
		}
		else
		{
			ss << sizeInBytes << " bytes";
		}
		return ss.str();
	}

	std::string GetTimerDurationText(float timerSeconds)
	{
		std::stringstream ss;
		ss << std::fixed << std::setprecision(1);
		if (timerSeconds < 1)
		{
			ss << timerSeconds * 1000 << " ms";
		}
		else if (timerSeconds < 60)
		{
			ss << timerSeconds << " sec";
		}
		else if (timerSeconds < 60 * 60)
		{
			ss << timerSeconds / 60 << " min";
		}
		else
		{
			ss << timerSeconds / 60 / 60 << " hours";
		}
		return ss.str();
	}

	std::string GetPlatformErrorString(wi::platform::error_type code)
	{
		std::string str;

#ifdef PLATFORM_WINDOWS_DESKTOP
		_com_error err(code);
		LPCTSTR errMsg = err.ErrorMessage();
		wchar_t wtext[1024] = {};
		_snwprintf_s(wtext, arraysize(wtext), arraysize(wtext), L"0x%08x (%s)", code, errMsg);
		char text[1024] = {};
		wi::helper::StringConvert(wtext, text, arraysize(text));
		str = text;
#endif // _WIN32

#ifdef PLATFORM_XBOX
		char text[1024] = {};
		snprintf(text, arraysize(text), "HRESULT error: 0x%08x", code);
		str = text;
#endif // PLATFORM_XBOX

		return str;
	}

	bool Compress(const uint8_t* src_data, size_t src_size, wi::vector<uint8_t>& dst_data, int level)
	{
		dst_data.resize(ZSTD_compressBound(src_size));
		size_t res = ZSTD_compress(dst_data.data(), dst_data.size(), src_data, src_size, level);
		if (ZSTD_isError(res))
			return false;
		dst_data.resize(res);
		return true;
	}

	bool Decompress(const uint8_t* src_data, size_t src_size, wi::vector<uint8_t>& dst_data)
	{
		size_t res = ZSTD_getFrameContentSize(src_data, src_size);
		if (ZSTD_isError(res))
			return false;
		dst_data.resize(res);
		res = ZSTD_decompress(dst_data.data(), dst_data.size(), src_data, src_size);
		return ZSTD_isError(res) == 0;
	}

	size_t HashByteData(const uint8_t* data, size_t size)
	{
		size_t hash = 0;
		for (size_t i = 0; i < size; ++i)
		{
			hash_combine(hash, data[i]);
		}
		return hash;
	}

	std::wstring GetClipboardText()
	{
		std::wstring wstr;

#ifdef PLATFORM_WINDOWS_DESKTOP
		if (!::OpenClipboard(NULL))
			return wstr;
		HANDLE wbuf_handle = ::GetClipboardData(CF_UNICODETEXT);
		if (wbuf_handle == NULL)
		{
			::CloseClipboard();
			return wstr;
		}
		if (const WCHAR* wbuf_global = (const WCHAR*)::GlobalLock(wbuf_handle))
		{
			wstr = wbuf_global;
		}
		::GlobalUnlock(wbuf_handle);
		::CloseClipboard();
#endif // PLATFORM_WINDOWS_DESKTOP

		return wstr;
	}

	void SetClipboardText(const std::wstring& wstr)
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		if (!::OpenClipboard(NULL))
			return;
		const int wbuf_length = (int)wstr.length() + 1;
		HGLOBAL wbuf_handle = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wbuf_length * sizeof(WCHAR));
		if (wbuf_handle == NULL)
		{
			::CloseClipboard();
			return;
		}
		WCHAR* wbuf_global = (WCHAR*)::GlobalLock(wbuf_handle);
		std::memcpy(wbuf_global, wstr.c_str(), wbuf_length * sizeof(wchar_t));
		::GlobalUnlock(wbuf_handle);
		::EmptyClipboard();
		if (::SetClipboardData(CF_UNICODETEXT, wbuf_handle) == NULL)
			::GlobalFree(wbuf_handle);
		::CloseClipboard();
#endif // PLATFORM_WINDOWS_DESKTOP
	}
}
