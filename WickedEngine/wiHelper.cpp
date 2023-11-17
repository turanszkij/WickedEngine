#include "wiHelper.h"
#include "wiPlatform.h"
#include "wiBacklog.h"
#include "wiEventHandler.h"
#include "wiMath.h"

#include "Utility/lodepng.h"
#include "Utility/dds_write.h"
#include "Utility/stb_image_write.h"
#include "Utility/basis_universal/encoder/basisu_comp.h"
#include "Utility/basis_universal/encoder/basisu_gpu_texture.h"

#include <thread>
#include <locale>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <codecvt> // string conversion
#include <filesystem>
#include <vector>
#include <iostream>
#include <cstdlib>

#if defined(_WIN32)
#include <direct.h>
#include <Psapi.h> // GetProcessMemoryInfo
#ifdef PLATFORM_UWP
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.AccessCache.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.h>
#else
#include <Commdlg.h> // openfile
#include <WinBase.h>
#endif // PLATFORM_UWP
#elif defined(PLATFORM_PS5)
#else
#include "Utility/portable-file-dialogs.h"
#endif // _WIN32

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
		MessageBoxA(GetActiveWindow(), msg.c_str(), caption.c_str(), 0);
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef PLATFORM_UWP
		std::wstring wmessage, wcaption;
		StringConvert(msg, wmessage);
		StringConvert(caption, wcaption);
		// UWP can only show message box on main thread:
		wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			winrt::Windows::UI::Popups::MessageDialog(wmessage, wcaption).ShowAsync();
			});
#endif // PLATFORM_UWP

#ifdef SDL2
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption.c_str(), msg.c_str(), NULL);
#endif // SDL2
	}

	std::string screenshot(const wi::graphics::SwapChain& swapchain, const std::string& name)
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

		bool result = saveTextureToFile(wi::graphics::GetDevice()->GetBackBuffer(&swapchain), filename);
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
			texturedata.resize(stagingTex.mapped_size);

			const uint32_t data_stride = GetFormatStride(desc.format);
			const uint32_t block_size = GetFormatBlockSize(desc.format);
			const uint32_t num_blocks_x = desc.width / block_size;
			const uint32_t num_blocks_y = desc.height / block_size;
			size_t cpy_offset = 0;
			size_t subresourceIndex = 0;
			for (uint32_t layer = 0; layer < desc.array_size; ++layer)
			{
				uint32_t mip_width = num_blocks_x;
				uint32_t mip_height = num_blocks_y;
				uint32_t mip_depth = desc.depth;
				for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
				{
					assert(subresourceIndex < stagingTex.mapped_subresource_count);
					const SubresourceData& subresourcedata = stagingTex.mapped_subresources[subresourceIndex++];
					const size_t dst_rowpitch = mip_width * data_stride;
					for (uint32_t z = 0; z < mip_depth; ++z)
					{
						uint8_t* dst_slice = texturedata.data() + cpy_offset;
						uint8_t* src_slice = (uint8_t*)subresourcedata.data_ptr + subresourcedata.slice_pitch * z;
						for (uint32_t i = 0; i < mip_height; ++i)
						{
							std::memcpy(
								dst_slice + i * dst_rowpitch,
								src_slice + i * subresourcedata.row_pitch,
								dst_rowpitch
							);
						}
						cpy_offset += mip_height * dst_rowpitch;
					}

					mip_width = std::max(1u, mip_width / 2);
					mip_height = std::max(1u, mip_height / 2);
					mip_depth = std::max(1u, mip_depth / 2);
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
			filedata.resize(sizeof(dds_write::Header) + texturedata.size());
			dds_write::DXGI_FORMAT dds_format = dds_write::DXGI_FORMAT_UNKNOWN;
			switch (desc.format)
			{
			case wi::graphics::Format::R32G32B32A32_FLOAT:
				dds_format = dds_write::DXGI_FORMAT_R32G32B32A32_FLOAT;
				break;
			case wi::graphics::Format::R32G32B32A32_UINT:
				dds_format = dds_write::DXGI_FORMAT_R32G32B32A32_UINT;
				break;
			case wi::graphics::Format::R32G32B32A32_SINT:
				dds_format = dds_write::DXGI_FORMAT_R32G32B32A32_SINT;
				break;
			case wi::graphics::Format::R32G32B32_FLOAT:
				dds_format = dds_write::DXGI_FORMAT_R32G32B32_FLOAT;
				break;
			case wi::graphics::Format::R32G32B32_UINT:
				dds_format = dds_write::DXGI_FORMAT_R32G32B32_UINT;
				break;
			case wi::graphics::Format::R32G32B32_SINT:
				dds_format = dds_write::DXGI_FORMAT_R32G32B32_SINT;
				break;
			case wi::graphics::Format::R16G16B16A16_FLOAT:
				dds_format = dds_write::DXGI_FORMAT_R16G16B16A16_FLOAT;
				break;
			case wi::graphics::Format::R16G16B16A16_UNORM:
				dds_format = dds_write::DXGI_FORMAT_R16G16B16A16_UNORM;
				break;
			case wi::graphics::Format::R16G16B16A16_UINT:
				dds_format = dds_write::DXGI_FORMAT_R16G16B16A16_UINT;
				break;
			case wi::graphics::Format::R16G16B16A16_SNORM:
				dds_format = dds_write::DXGI_FORMAT_R16G16B16A16_SNORM;
				break;
			case wi::graphics::Format::R16G16B16A16_SINT:
				dds_format = dds_write::DXGI_FORMAT_R16G16B16A16_SINT;
				break;
			case wi::graphics::Format::R32G32_FLOAT:
				dds_format = dds_write::DXGI_FORMAT_R32G32_FLOAT;
				break;
			case wi::graphics::Format::R32G32_UINT:
				dds_format = dds_write::DXGI_FORMAT_R32G32_UINT;
				break;
			case wi::graphics::Format::R32G32_SINT:
				dds_format = dds_write::DXGI_FORMAT_R32G32_SINT;
				break;
			case wi::graphics::Format::R10G10B10A2_UNORM:
				dds_format = dds_write::DXGI_FORMAT_R10G10B10A2_UNORM;
				break;
			case wi::graphics::Format::R10G10B10A2_UINT:
				dds_format = dds_write::DXGI_FORMAT_R10G10B10A2_UINT;
				break;
			case wi::graphics::Format::R11G11B10_FLOAT:
				dds_format = dds_write::DXGI_FORMAT_R11G11B10_FLOAT;
				break;
			case wi::graphics::Format::R8G8B8A8_UNORM:
				dds_format = dds_write::DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			case wi::graphics::Format::R8G8B8A8_UNORM_SRGB:
				dds_format = dds_write::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				break;
			case wi::graphics::Format::R8G8B8A8_UINT:
				dds_format = dds_write::DXGI_FORMAT_R8G8B8A8_UINT;
				break;
			case wi::graphics::Format::R8G8B8A8_SNORM:
				dds_format = dds_write::DXGI_FORMAT_R8G8B8A8_SNORM;
				break;
			case wi::graphics::Format::R8G8B8A8_SINT:
				dds_format = dds_write::DXGI_FORMAT_R8G8B8A8_SINT;
				break;
			case wi::graphics::Format::B8G8R8A8_UNORM:
				dds_format = dds_write::DXGI_FORMAT_B8G8R8A8_UNORM;
				break;
			case wi::graphics::Format::B8G8R8A8_UNORM_SRGB:
				dds_format = dds_write::DXGI_FORMAT_R16G16_SINT;
				break;
			case wi::graphics::Format::R16G16_FLOAT:
				dds_format = dds_write::DXGI_FORMAT_R16G16_FLOAT;
				break;
			case wi::graphics::Format::R16G16_UNORM:
				dds_format = dds_write::DXGI_FORMAT_R16G16_UNORM;
				break;
			case wi::graphics::Format::R16G16_UINT:
				dds_format = dds_write::DXGI_FORMAT_R16G16_UINT;
				break;
			case wi::graphics::Format::R16G16_SNORM:
				dds_format = dds_write::DXGI_FORMAT_R16G16_SNORM;
				break;
			case wi::graphics::Format::R16G16_SINT:
				dds_format = dds_write::DXGI_FORMAT_R16G16_SINT;
				break;
			case wi::graphics::Format::D32_FLOAT:
			case wi::graphics::Format::R32_FLOAT:
				dds_format = dds_write::DXGI_FORMAT_R32_FLOAT;
				break;
			case wi::graphics::Format::R32_UINT:
				dds_format = dds_write::DXGI_FORMAT_R32_UINT;
				break;
			case wi::graphics::Format::R32_SINT:
				dds_format = dds_write::DXGI_FORMAT_R32_SINT;
				break;
			case wi::graphics::Format::R9G9B9E5_SHAREDEXP:
				dds_format = dds_write::DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
				break;
			case wi::graphics::Format::R8G8_UNORM:
				dds_format = dds_write::DXGI_FORMAT_R8G8_UNORM;
				break;
			case wi::graphics::Format::R8G8_UINT:
				dds_format = dds_write::DXGI_FORMAT_R8G8_UINT;
				break;
			case wi::graphics::Format::R8G8_SNORM:
				dds_format = dds_write::DXGI_FORMAT_R8G8_SNORM;
				break;
			case wi::graphics::Format::R8G8_SINT:
				dds_format = dds_write::DXGI_FORMAT_R8G8_SINT;
				break;
			case wi::graphics::Format::R16_FLOAT:
				dds_format = dds_write::DXGI_FORMAT_R16_FLOAT;
				break;
			case wi::graphics::Format::D16_UNORM:
			case wi::graphics::Format::R16_UNORM:
				dds_format = dds_write::DXGI_FORMAT_R16_UNORM;
				break;
			case wi::graphics::Format::R16_UINT:
				dds_format = dds_write::DXGI_FORMAT_R16_UINT;
				break;
			case wi::graphics::Format::R16_SNORM:
				dds_format = dds_write::DXGI_FORMAT_R16_SNORM;
				break;
			case wi::graphics::Format::R16_SINT:
				dds_format = dds_write::DXGI_FORMAT_R16_SINT;
				break;
			case wi::graphics::Format::R8_UNORM:
				dds_format = dds_write::DXGI_FORMAT_R8_UNORM;
				break;
			case wi::graphics::Format::R8_UINT:
				dds_format = dds_write::DXGI_FORMAT_R8_UINT;
				break;
			case wi::graphics::Format::R8_SNORM:
				dds_format = dds_write::DXGI_FORMAT_R8_SNORM;
				break;
			case wi::graphics::Format::R8_SINT:
				dds_format = dds_write::DXGI_FORMAT_R8_SINT;
				break;
			case wi::graphics::Format::BC1_UNORM:
				dds_format = dds_write::DXGI_FORMAT_BC1_UNORM;
				break;
			case wi::graphics::Format::BC1_UNORM_SRGB:
				dds_format = dds_write::DXGI_FORMAT_BC1_UNORM_SRGB;
				break;
			case wi::graphics::Format::BC2_UNORM:
				dds_format = dds_write::DXGI_FORMAT_BC2_UNORM;
				break;
			case wi::graphics::Format::BC2_UNORM_SRGB:
				dds_format = dds_write::DXGI_FORMAT_BC2_UNORM_SRGB;
				break;
			case wi::graphics::Format::BC3_UNORM:
				dds_format = dds_write::DXGI_FORMAT_BC3_UNORM;
				break;
			case wi::graphics::Format::BC3_UNORM_SRGB:
				dds_format = dds_write::DXGI_FORMAT_BC3_UNORM_SRGB;
				break;
			case wi::graphics::Format::BC4_UNORM:
				dds_format = dds_write::DXGI_FORMAT_BC4_UNORM;
				break;
			case wi::graphics::Format::BC4_SNORM:
				dds_format = dds_write::DXGI_FORMAT_BC4_SNORM;
				break;
			case wi::graphics::Format::BC5_UNORM:
				dds_format = dds_write::DXGI_FORMAT_BC5_UNORM;
				break;
			case wi::graphics::Format::BC5_SNORM:
				dds_format = dds_write::DXGI_FORMAT_BC5_SNORM;
				break;
			case wi::graphics::Format::BC6H_UF16:
				dds_format = dds_write::DXGI_FORMAT_BC6H_UF16;
				break;
			case wi::graphics::Format::BC6H_SF16:
				dds_format = dds_write::DXGI_FORMAT_BC6H_SF16;
				break;
			case wi::graphics::Format::BC7_UNORM:
				dds_format = dds_write::DXGI_FORMAT_BC7_UNORM;
				break;
			case wi::graphics::Format::BC7_UNORM_SRGB:
				dds_format = dds_write::DXGI_FORMAT_BC7_UNORM_SRGB;
				break;
			default:
				assert(0);
				return false;
			}
			dds_write::write_header(
				filedata.data(),
				dds_format,
				desc.width,
				desc.type == TextureDesc::Type::TEXTURE_1D ? 0 : desc.height,
				desc.mip_levels,
				desc.array_size,
				has_flag(desc.misc_flags, ResourceMiscFlag::TEXTURECUBE),
				desc.type == TextureDesc::Type::TEXTURE_3D ? desc.depth : 0
			);
			std::memcpy(filedata.data() + sizeof(dds_write::Header), texturedata.data(), texturedata.size());
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

		bool basis = !extension.compare("BASIS");
		bool ktx2 = !extension.compare("KTX2");
		basisu::image basis_image;
		basisu::vector<basisu::image> basis_mipmaps;

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
		else if (IsFormatBlockCompressed(desc.format))
		{
			basisu::texture_format fmt;
			switch (desc.format)
			{
			default:
				assert(0);
				return false;
			case Format::BC1_UNORM:
			case Format::BC1_UNORM_SRGB:
				fmt = basisu::texture_format::cBC1;
				break;
			case Format::BC3_UNORM:
			case Format::BC3_UNORM_SRGB:
				fmt = basisu::texture_format::cBC3;
				break;
			case Format::BC4_UNORM:
				fmt = basisu::texture_format::cBC4;
				break;
			case Format::BC5_UNORM:
				fmt = basisu::texture_format::cBC5;
				break;
			case Format::BC7_UNORM:
			case Format::BC7_UNORM_SRGB:
				fmt = basisu::texture_format::cBC7;
				break;
			}
			basisu::gpu_image basis_gpu_image;
			basis_gpu_image.init(fmt, desc.width, desc.height);
			std::memcpy(basis_gpu_image.get_ptr(), texturedata.data(), std::min(texturedata.size(), (size_t)basis_gpu_image.get_size_in_bytes()));
			basis_gpu_image.unpack(basis_image);
		}
		else
		{
			assert(desc.format == Format::R8G8B8A8_UNORM || desc.format == Format::R8G8B8A8_UNORM_SRGB); // If you need to save other texture format, implement data conversion for it
		}

		if (basis || ktx2)
		{
			if (basis_image.get_total_pixels() == 0)
			{
				basis_image.init(texturedata.data(), desc.width, desc.height, 4);
				if (desc.mip_levels > 1)
				{
					basis_mipmaps.reserve(desc.mip_levels - 1);
					for (uint32_t mip = 1; mip < desc.mip_levels; ++mip)
					{
						basisu::image basis_mip;
						const MipDesc& mipdesc = mips[mip];
						basis_mip.init(mipdesc.address, mipdesc.width, mipdesc.height, 4);
						basis_mipmaps.push_back(basis_mip);
					}
				}
			}
			static bool encoder_initialized = false;
			if (!encoder_initialized)
			{
				encoder_initialized = true;
				basisu::basisu_encoder_init(false, false);
			}
			basisu::basis_compressor_params params;
			params.m_source_images.push_back(basis_image);
			if (desc.mip_levels > 1)
			{
				params.m_source_mipmap_images.push_back(basis_mipmaps);
			}
			if (ktx2)
			{
				params.m_create_ktx2_file = true;
			}
			else
			{
				params.m_create_ktx2_file = false;
			}
#if 1
			params.m_compression_level = basisu::BASISU_DEFAULT_COMPRESSION_LEVEL;
#else
			params.m_compression_level = basisu::BASISU_MAX_COMPRESSION_LEVEL;
#endif
			// Disable CPU mipmap generation:
			//	instead we provide mipmap data that was downloaded from the GPU with m_source_mipmap_images.
			//	This is better, because engine specific mipgen options will be retained, such as coverage preserving mipmaps
			params.m_mip_gen = false;
			params.m_quality_level = basisu::BASISU_QUALITY_MAX;
			params.m_multithreading = true;
			int num_threads = std::max(1u, std::thread::hardware_concurrency());
			basisu::job_pool jpool(num_threads);
			params.m_pJob_pool = &jpool;
			basisu::basis_compressor compressor;
			if (compressor.init(params))
			{
				auto result = compressor.process();
				if (result == basisu::basis_compressor::cECSuccess)
				{
					if (basis)
					{
						const auto& basis_file = compressor.get_output_basis_file();
						filedata.resize(basis_file.size());
						std::memcpy(filedata.data(), basis_file.data(), basis_file.size());
						return true;
					}
					else if (ktx2)
					{
						const auto& ktx2_file = compressor.get_output_ktx2_file();
						filedata.resize(ktx2_file.size());
						std::memcpy(filedata.data(), ktx2_file.data(), ktx2_file.size());
						return true;
					}
				}
				else
				{
					wi::backlog::post("basisu::basis_compressor::process() failure!", wi::backlog::LogLevel::Error);
					assert(0);
				}
			}
			return false;
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

	std::string GetPathRelative(const std::string& rootdir, std::string& path)
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

#ifdef PLATFORM_UWP

		size_t found = path.rfind(rootdir);
		if (found != std::string::npos)
		{
			path = path.substr(found + rootdir.length());
		}

#else

		std::filesystem::path filepath = ToNativeString(path);
		if (filepath.is_absolute())
		{
			std::filesystem::path rootpath = ToNativeString(rootdir);
			std::filesystem::path relative = std::filesystem::relative(filepath, rootpath);
			if (!relative.empty())
			{
				path = relative.generic_u8string();
			}
		}

#endif // PLATFORM_UWP

	}

	void MakePathAbsolute(std::string& path)
	{
		std::filesystem::path absolute = std::filesystem::absolute(ToNativeString(path));
		if (!absolute.empty())
		{
			path = absolute.generic_u8string();
		}
	}

	void DirectoryCreate(const std::string& path)
	{
		std::filesystem::create_directories(ToNativeString(path));
	}

	template<template<typename T, typename A> typename vector_interface>
	bool FileRead_Impl(const std::string& fileName, vector_interface<uint8_t, std::allocator<uint8_t>>& data)
	{
#ifndef PLATFORM_UWP
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
			file.seekg(0, file.beg);
			data.resize(dataSize);
			file.read((char*)data.data(), dataSize);
			file.close();
			return true;
		}
#else
		using namespace winrt::Windows::Storage;
		using namespace winrt::Windows::Storage::Streams;
		using namespace winrt::Windows::Foundation;
		std::wstring wstr;
		std::filesystem::path filepath = fileName;
		filepath = std::filesystem::absolute(filepath);
		StringConvert(filepath.string(), wstr);
		bool success = false;

		auto async_helper = [&]() -> IAsyncAction {
			try
			{
				auto file = co_await StorageFile::GetFileFromPathAsync(wstr);
				auto buffer = co_await FileIO::ReadBufferAsync(file);
				auto reader = DataReader::FromBuffer(buffer);
				auto size = buffer.Length();
				data.resize((size_t)size);
				for (auto& x : data)
				{
					x = reader.ReadByte();
				}
				success = true;
			}
			catch (winrt::hresult_error const& ex)
			{
				switch (ex.code())
				{
				case E_ACCESSDENIED:
					wi::backlog::post("Opening file failed: " + fileName + " | Reason: Permission Denied!");
					break;
				default:
					break;
				}
			}

		};

		if (winrt::impl::is_sta_thread())
		{
			std::thread([&] { async_helper().get(); }).join(); // can't block coroutine from ui thread
		}
		else
		{
			async_helper().get();
		}

		if (success)
		{
			return true;
		}

#endif // PLATFORM_UWP

		wi::backlog::post("File not found: " + fileName, wi::backlog::LogLevel::Warning);
		return false;
	}
	bool FileRead(const std::string& fileName, wi::vector<uint8_t>& data)
	{
		return FileRead_Impl(fileName, data);
	}
#if WI_VECTOR_TYPE
	bool FileRead(const std::string& fileName, std::vector<uint8_t>& data)
	{
		return FileRead_Impl(fileName, data);
	}
#endif // WI_VECTOR_TYPE

	bool FileWrite(const std::string& fileName, const uint8_t* data, size_t size)
	{
		if (size <= 0)
		{
			return false;
		}

#ifndef PLATFORM_UWP
		std::ofstream file(ToNativeString(fileName), std::ios::binary | std::ios::trunc);
		if (file.is_open())
		{
			file.write((const char*)data, (std::streamsize)size);
			file.close();
			return true;
		}
#else

		using namespace winrt::Windows::Storage;
		using namespace winrt::Windows::Storage::Streams;
		using namespace winrt::Windows::Foundation;
		std::wstring wstr;
		std::filesystem::path filepath = fileName;
		filepath = std::filesystem::absolute(filepath);
		StringConvert(filepath.string(), wstr);

		CREATEFILE2_EXTENDED_PARAMETERS params = {};
		params.dwSize = (DWORD)size;
		params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		HANDLE filehandle = CreateFile2FromAppW(wstr.c_str(), GENERIC_READ | GENERIC_WRITE, 0, CREATE_ALWAYS, &params);
		assert(filehandle);
		CloseHandle(filehandle);

		bool success = false;
		auto async_helper = [&]() -> IAsyncAction {
			try
			{
				auto file = co_await StorageFile::GetFileFromPathAsync(wstr);
				winrt::array_view<const uint8_t> dataarray(data, (winrt::array_view<const uint8_t>::size_type)size);
				co_await FileIO::WriteBytesAsync(file, dataarray);
				success = true;
			}
			catch (winrt::hresult_error const& ex)
			{
				switch (ex.code())
				{
				case E_ACCESSDENIED:
					wi::backlog::post("Opening file failed: " + fileName + " | Reason: Permission Denied!");
					break;
				default:
					break;
				}
			}

		};

		if (winrt::impl::is_sta_thread())
		{
			std::thread([&] { async_helper().get(); }).join(); // can't block coroutine from ui thread
		}
		else
		{
			async_helper().get();
		}

		if (success)
		{
			return true;
		}
#endif // PLATFORM_UWP

		return false;
	}

	bool FileExists(const std::string& fileName)
	{
#ifndef PLATFORM_UWP
		bool exists = std::filesystem::exists(ToNativeString(fileName));
		return exists;
#else
		using namespace winrt::Windows::Storage;
		using namespace winrt::Windows::Storage::Streams;
		using namespace winrt::Windows::Foundation;
		std::wstring wstr;
		std::filesystem::path filepath = fileName;
		filepath = std::filesystem::absolute(filepath);
		StringConvert(filepath.string(), wstr);
		bool success = false;

		auto async_helper = [&]() -> IAsyncAction {
			try
			{
				auto file = co_await StorageFile::GetFileFromPathAsync(wstr);
				success = true;
			}
			catch (winrt::hresult_error const& ex)
			{
				switch (ex.code())
				{
				case E_ACCESSDENIED:
					wi::backlog::post("Opening file failed: " + fileName + " | Reason: Permission Denied!");
					break;
				default:
					break;
				}
			}

		};

		if (winrt::impl::is_sta_thread())
		{
			std::thread([&] { async_helper().get(); }).join(); // can't block coroutine from ui thread
		}
		else
		{
			async_helper().get();
		}

		return success;
#endif // PLATFORM_UWP
	}

	uint64_t FileTimestamp(const std::string& fileName)
	{
		auto tim = std::filesystem::last_write_time(ToNativeString(fileName));
		return std::chrono::duration_cast<std::chrono::duration<uint64_t>>(tim.time_since_epoch()).count();
	}

	std::string GetTempDirectoryPath()
	{
#if defined(PLATFORM_XBOX) || defined(PLATFORM_PS5)
		return "";
#else
		auto path = std::filesystem::temp_directory_path();
		return path.generic_u8string();
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
		return path.generic_u8string();
#endif // PLATFORM_PS5
	}

	void FileDialog(const FileDialogParams& params, std::function<void(std::string fileName)> onSuccess)
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		std::thread([=] {

			wchar_t szFile[256];

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
				ofn.Flags |= OFN_NOCHANGEDIR;
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
				std::string result_filename;
				StringConvert(ofn.lpstrFile, result_filename);
				onSuccess(result_filename);
			}

			}).detach();
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef PLATFORM_UWP
		auto filedialoghelper = [](FileDialogParams params, std::function<void(std::string fileName)> onSuccess) -> winrt::fire_and_forget {

			using namespace winrt::Windows::Storage;
			using namespace winrt::Windows::Storage::Pickers;
			using namespace winrt::Windows::Storage::AccessCache;

			switch (params.type)
			{
			default:
			case FileDialogParams::OPEN:
			{
				FileOpenPicker picker;
				picker.ViewMode(PickerViewMode::List);
				picker.SuggestedStartLocation(PickerLocationId::Objects3D);

				for (auto& x : params.extensions)
				{
					std::wstring wstr;
					StringConvert(x, wstr);
					wstr = L"." + wstr;
					picker.FileTypeFilter().Append(wstr);
				}

				auto file = co_await picker.PickSingleFileAsync();

				if (file)
				{
					auto futureaccess = StorageApplicationPermissions::FutureAccessList();
					futureaccess.Clear();
					futureaccess.Add(file);
					std::wstring wstr = file.Path().data();
					std::string str;
					StringConvert(wstr, str);

					onSuccess(str);
				}
			}
			break;
			case FileDialogParams::SAVE:
			{
				FileSavePicker picker;
				picker.SuggestedStartLocation(PickerLocationId::Objects3D);

				std::wstring wdesc;
				StringConvert(params.description, wdesc);
				winrt::Windows::Foundation::Collections::IVector<winrt::hstring> extensions{ winrt::single_threaded_vector<winrt::hstring>() };
				for (auto& x : params.extensions)
				{
					std::wstring wstr;
					StringConvert(x, wstr);
					wstr = L"." + wstr;
					extensions.Append(wstr);
				}
				picker.FileTypeChoices().Insert(wdesc, extensions);

				auto file = co_await picker.PickSaveFileAsync();
				if (file)
				{
					auto futureaccess = StorageApplicationPermissions::FutureAccessList();
					futureaccess.Clear();
					futureaccess.Add(file);
					std::wstring wstr = file.Path().data();
					std::string str;
					StringConvert(wstr, str);
					onSuccess(str);
				}
			}
			break;
			}
		};
		filedialoghelper(params, onSuccess);

#endif // PLATFORM_UWP

#ifdef PLATFORM_LINUX
		if (!pfd::settings::available()) {
			const char *message = "No dialog backend available";
#ifdef SDL2
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
									 "File dialog error!",
									 message,
									 nullptr);
#endif
			std::cerr << message << std::endl;
		}

		std::vector<std::string> extensions = {params.description, ""};
		int extcount = 0;
		for (auto& x : params.extensions)
		{
			extensions[1] += "*." + toLower(x);
			extensions[1] += " ";
			extensions[1] += "*." + toUpper(x);
			if (extcount < params.extensions.size() - 1) {
				extensions[1] += " ";
			}
			extcount++;
		}

		switch (params.type) {
			case FileDialogParams::OPEN: {
				std::vector<std::string> selection = pfd::open_file(
					"Open file",
					std::filesystem::current_path().string(),
					extensions
				).result();
				if (!selection.empty())
				{
					onSuccess(selection[0]);
				}
				break;
			}
			case FileDialogParams::SAVE: {
				std::string destination = pfd::save_file(
					"Save file",
					std::filesystem::current_path().string(),
					extensions
				).result();
				if (!destination.empty())
				{
					onSuccess(destination);
				}
				break;
			}
		}
#endif // PLATFORM_LINUX
	}

	void GetFileNamesInDirectory(const std::string& directory, std::function<void(std::string fileName)> onSuccess, const std::string& filter_extension)
	{
		std::filesystem::path directory_path = ToNativeString(directory);
		if (!std::filesystem::exists(directory_path))
			return;

		for (const auto& entry : std::filesystem::directory_iterator(directory_path))
		{
			std::string filename = entry.path().filename().generic_u8string();
			if (filter_extension.empty() || wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename)).compare(filter_extension) == 0)
			{
				onSuccess(directory + filename);
			}
		}
	}

	bool Bin2H(const uint8_t* data, size_t size, const std::string& dst_filename, const char* dataName)
	{
		std::string ss;
		ss += "const uint8_t ";
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

	void StringConvert(const std::string& from, std::wstring& to)
	{
#ifdef _WIN32
		int num = MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, NULL, 0);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, &to[0], num);
		}
#else
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
		to = cv.from_bytes(from);
#endif // _WIN32
	}

	void StringConvert(const std::wstring& from, std::string& to)
	{
#ifdef _WIN32
		int num = WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, NULL, 0, NULL, NULL);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, &to[0], num, NULL, NULL);
		}
#else
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
		to = cv.to_bytes(from);
#endif // _WIN32
	}

	int StringConvert(const char* from, wchar_t* to, int dest_size_in_characters)
	{
#ifdef _WIN32
		int num = MultiByteToWideChar(CP_UTF8, 0, from, -1, NULL, 0);
		if (num > 0)
		{
			if (dest_size_in_characters >= 0)
			{
				num = std::min(num, dest_size_in_characters);
			}
			MultiByteToWideChar(CP_UTF8, 0, from, -1, &to[0], num);
		}
		return num;
#else
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
		auto result = cv.from_bytes(from).c_str();
		int num = (int)cv.converted();
		if (dest_size_in_characters >= 0)
		{
			num = std::min(num, dest_size_in_characters);
		}
		std::memcpy(to, result, num * sizeof(wchar_t));
		return num;
#endif // _WIN32
	}

	int StringConvert(const wchar_t* from, char* to, int dest_size_in_characters)
	{
#ifdef _WIN32
		int num = WideCharToMultiByte(CP_UTF8, 0, from, -1, NULL, 0, NULL, NULL);
		if (num > 0)
		{
			if (dest_size_in_characters >= 0)
			{
				num = std::min(num, dest_size_in_characters);
			}
			WideCharToMultiByte(CP_UTF8, 0, from, -1, &to[0], num, NULL, NULL);
		}
		return num;
#else
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
		auto result = cv.to_bytes(from).c_str();
		int num = (size_t)cv.converted();
		if (dest_size_in_characters >= 0)
		{
			num = std::min(num, dest_size_in_characters);
		}
		std::memcpy(to, result, num * sizeof(char));
		return num;
#endif // _WIN32
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
			break;
		case DebugLevel::Warning:
			std::clog << str;
			break;
		case DebugLevel::Error:
			std::cerr << str;
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
#ifdef PLATFORM_UWP
		winrt::Windows::System::Launcher::LaunchUriAsync(winrt::Windows::Foundation::Uri(winrt::to_hstring(url)));
		return;
#endif // PLATFORM_UWP

#ifdef PLATFORM_WINDOWS_DESKTOP
		std::string op = "start " + url;
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
		// TODO Linux
#endif // _WIN32
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
}
