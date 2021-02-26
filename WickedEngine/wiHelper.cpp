#include "wiHelper.h"
#include "wiPlatform.h"
#include "wiRenderer.h"
#include "wiBackLog.h"
#include "wiEvent.h"

#include "Utility/stb_image_write.h"

#include <thread>
#include <locale>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <codecvt> // string conversion
#include <filesystem>

#ifdef _WIN32
#include <direct.h>
#ifdef PLATFORM_UWP
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.AccessCache.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#else
#include <Commdlg.h> // openfile
#include <WinBase.h>
#endif // PLATFORM_UWP
#else
#include "Utility/portable-file-dialogs.h"
#endif // _WIN32

using namespace std;

namespace wiHelper
{

	string toUpper(const std::string& s)
	{
		std::string result;
		std::locale loc;
		for (unsigned int i = 0; i < s.length(); ++i)
		{
			result += std::toupper(s.at(i), loc);
		}
		return result;
	}

	void messageBox(const std::string& msg, const std::string& caption)
	{
#ifdef _WIN32
#ifndef PLATFORM_UWP
		MessageBoxA(GetActiveWindow(), msg.c_str(), caption.c_str(), 0);
#else
		wstring wmessage, wcaption;
		StringConvert(msg, wmessage);
		StringConvert(caption, wcaption);
		// UWP can only show message box on main thread:
		wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			winrt::Windows::UI::Popups::MessageDialog(wmessage, wcaption).ShowAsync();
		});
#endif // PLATFORM_UWP
#elif SDL2
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption.c_str(), msg.c_str(), NULL);
#endif // _WIN32
	}

	void screenshot(const std::string& name)
	{
		std::string directory;
		if (name.empty())
		{
			directory = "screenshots";
		}
		else
		{
			directory = GetDirectoryFromPath(name);
		}

		if (!std::filesystem::create_directory(directory.c_str()))
		{
			wiBackLog::post(("Directory couoldn't be created: " + directory).c_str());
		}

		std::string filename = name;
		if (filename.empty())
		{
			filename = directory + "/sc_" + getCurrentDateTimeAsString() + ".jpg";
		}

		bool result = saveTextureToFile(wiRenderer::GetDevice()->GetBackBuffer(), filename);
		assert(result);

		if (result)
		{
			std::string msg = "Screenshot saved: " + filename;
			wiBackLog::post(msg.c_str());
		}
	}

	bool saveTextureToMemory(const wiGraphics::Texture& texture, std::vector<uint8_t>& data)
	{
		using namespace wiGraphics;

		GraphicsDevice* device = wiRenderer::GetDevice();

		TextureDesc desc = texture.GetDesc();
		uint32_t data_count = desc.Width * desc.Height;
		uint32_t data_stride = device->GetFormatStride(desc.Format);
		uint32_t data_size = data_count * data_stride;

		data.resize(data_size);

		Texture stagingTex;
		TextureDesc staging_desc = desc;
		staging_desc.Usage = USAGE_STAGING;
		staging_desc.CPUAccessFlags = CPU_ACCESS_READ;
		staging_desc.BindFlags = 0;
		staging_desc.MiscFlags = 0;
		bool success = device->CreateTexture(&staging_desc, nullptr, &stagingTex);
		assert(success);

		CommandList cmd = device->BeginCommandList();
		device->CopyResource(&stagingTex, &texture, cmd);
		device->SubmitCommandLists();
		device->WaitForGPU();

		Mapping mapping;
		mapping._flags = Mapping::FLAG_READ;
		mapping.size = data_size;
		device->Map(&stagingTex, &mapping);
		if (mapping.data != nullptr)
		{
			if (mapping.rowpitch / data_stride != desc.Width)
			{
				// Copy padded texture row by row:
				const uint32_t cpysize = desc.Width * data_stride;
				for (uint32_t i = 0; i < desc.Height; ++i)
				{
					void* src = (void*)((size_t)mapping.data + size_t(i * mapping.rowpitch));
					void* dst = (void*)((size_t)data.data() + size_t(i * cpysize));
					memcpy(dst, src, cpysize);
				}
			}
			else
			{
				// Copy whole
				std::memcpy(data.data(), mapping.data, data.size());
			}
			device->Unmap(&stagingTex);
		}
		else
		{
			assert(0);
		}

		return mapping.data != nullptr;
	}

	bool saveTextureToFile(const wiGraphics::Texture& texture, const string& fileName)
	{
		using namespace wiGraphics;
		TextureDesc desc = texture.GetDesc();
		std::vector<uint8_t> data;
		if (saveTextureToMemory(texture, data))
		{
			return saveTextureToFile(data, desc, fileName);
		}
		return false;
	}

	bool saveTextureToFile(const std::vector<uint8_t>& textureData, const wiGraphics::TextureDesc& desc, const std::string& fileName)
	{
		using namespace wiGraphics;

		uint32_t data_count = desc.Width * desc.Height;

		if (desc.Format == FORMAT_R10G10B10A2_UNORM)
		{
			// So this should be converted first to rgba8 before saving to common format...

			uint32_t* data32 = (uint32_t*)textureData.data();

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
		else
		{
			assert(desc.Format == FORMAT_R8G8B8A8_UNORM); // If you need to save other backbuffer format, convert the data here yourself...
		}

		int write_result = 0;

		std::vector<uint8_t> filedata;
		stbi_write_func* func = [](void* context, void* data, int size) {
			std::vector<uint8_t>& filedata = *(std::vector<uint8_t>*)context;
			for (int i = 0; i < size; ++i)
			{
				filedata.push_back(*((uint8_t*)data + i));
			}
		};

		string extension = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(fileName));
		if (!extension.compare("JPG") || !extension.compare("JPEG"))
		{
			write_result = stbi_write_jpg_to_func(func, &filedata, (int)desc.Width, (int)desc.Height, 4, textureData.data(), 100);
		}
		else if (!extension.compare("PNG"))
		{
			write_result = stbi_write_png_to_func(func, &filedata, (int)desc.Width, (int)desc.Height, 4, textureData.data(), 0);
		}
		else if (!extension.compare("TGA"))
		{
			write_result = stbi_write_tga_to_func(func, &filedata, (int)desc.Width, (int)desc.Height, 4, textureData.data());
		}
		else if (!extension.compare("BMP"))
		{
			write_result = stbi_write_bmp_to_func(func, &filedata, (int)desc.Width, (int)desc.Height, 4, textureData.data());
		}
		else
		{
			assert(0 && "Unsupported extension");
		}

		if (write_result != 0)
		{
			return FileWrite(fileName, filedata.data(), filedata.size());
		}

		return false;
	}

	string getCurrentDateTimeAsString()
	{
		time_t t = std::time(nullptr);
		struct tm time_info;
#ifdef _WIN32
		localtime_s(&time_info, &t);
#else
		localtime(&t);
#endif
		stringstream ss("");
		ss << std::put_time(&time_info, "%d-%m-%Y %H-%M-%S");
		return ss.str();
	}

	void SplitPath(const std::string& fullPath, string& dir, string& fileName)
	{
		size_t found;
		found = fullPath.find_last_of("/\\");
		dir = fullPath.substr(0, found + 1);
		fileName = fullPath.substr(found + 1);
	}

	string GetFileNameFromPath(const std::string& fullPath)
	{
		if (fullPath.empty())
		{
			return fullPath;
		}

		string ret, empty;
		SplitPath(fullPath, empty, ret);
		return ret;
	}

	string GetDirectoryFromPath(const std::string& fullPath)
	{
		if (fullPath.empty())
		{
			return fullPath;
		}

		string ret, empty;
		SplitPath(fullPath, ret, empty);
		return ret;
	}

	string GetExtensionFromFileName(const string& filename)
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

	void MakePathRelative(const std::string& rootdir, std::string& path)
	{
		if (rootdir.empty() || path.empty())
		{
			return;
		}

		size_t found = path.rfind(rootdir);
		if (found != std::string::npos)
		{
			path = path.substr(found + rootdir.length());
		}
	}

	bool FileRead(const std::string& fileName, std::vector<uint8_t>& data)
	{
#ifndef PLATFORM_UWP
#ifdef SDL_FILESYSTEM_UNIX
		std::string filepath = fileName;
		std::replace(filepath.begin(), filepath.end(), '\\', '/');
		ifstream file(filepath, ios::binary | ios::ate);
#else
		ifstream file(fileName, ios::binary | ios::ate);
#endif // SDL_FILESYSTEM_UNIX
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
		wstring wstr;
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
					wiBackLog::post(("Opening file failed: " + fileName + " | Reason: Permission Denied!").c_str());
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

		wiBackLog::post(("File not found: " + fileName).c_str());
		return false;
	}

	bool FileWrite(const std::string& fileName, const uint8_t* data, size_t size)
	{
		if (size <= 0)
		{
			return false;
		}

#ifndef PLATFORM_UWP
		ofstream file(fileName, ios::binary | ios::trunc);
		if (file.is_open())
		{
			file.write((const char*)data, (streamsize)size);
			file.close();
			return true;
		}
#else

		using namespace winrt::Windows::Storage;
		using namespace winrt::Windows::Storage::Streams;
		using namespace winrt::Windows::Foundation;
		wstring wstr;
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
					wiBackLog::post(("Opening file failed: " + fileName + " | Reason: Permission Denied!").c_str());
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
		ifstream file(fileName);
		bool exists = file.is_open();
		file.close();
		return exists;
#else
		using namespace winrt::Windows::Storage;
		using namespace winrt::Windows::Foundation;
		string directory = GetDirectoryFromPath(fileName);
		string name = GetFileNameFromPath(fileName);
		wstring wdir, wname;
		StringConvert(directory, wdir);
		StringConvert(name, wname);
		bool success = false;

		auto async_helper = [&]() -> IAsyncAction {
			try
			{
				auto folder = co_await StorageFolder::GetFolderFromPathAsync(wdir);
				auto item = co_await folder.TryGetItemAsync(wname);
				if (item)
				{
					success = true;
				}
			}
			catch (winrt::hresult_error const& ex)
			{
				switch (ex.code())
				{
				case E_ACCESSDENIED:
					wiBackLog::post(("Opening file failed: " + fileName + " | Reason: Permission Denied!").c_str());
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
#endif
	}

	void FileDialog(const FileDialogParams& params, std::function<void(std::string fileName)> onSuccess)
	{
#ifdef _WIN32
#ifndef PLATFORM_UWP

		std::thread([=] {

			char szFile[256];

			OPENFILENAMEA ofn;
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
			std::vector<char> filter;
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
				ok = GetOpenFileNameA(&ofn) == TRUE;
				break;
			case FileDialogParams::SAVE:
				ofn.Flags = OFN_OVERWRITEPROMPT;
				ofn.Flags |= OFN_NOCHANGEDIR;
				ok = GetSaveFileNameA(&ofn) == TRUE;
				break;
			}

			if (ok)
			{
				onSuccess(ofn.lpstrFile);
			}

			}).detach();

#else
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
					wstring wstr;
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
					wstring wstr = file.Path().data();
					string str;
					StringConvert(wstr, str);

					onSuccess(str);
				}
			}
			break;
			case FileDialogParams::SAVE:
			{
				FileSavePicker picker;
				picker.SuggestedStartLocation(PickerLocationId::Objects3D);

				wstring wdesc;
				StringConvert(params.description, wdesc);
				winrt::Windows::Foundation::Collections::IVector<winrt::hstring> extensions{ winrt::single_threaded_vector<winrt::hstring>() };
				for (auto& x : params.extensions)
				{
					wstring wstr;
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
					wstring wstr = file.Path().data();
					string str;
					StringConvert(wstr, str);
					onSuccess(str);
				}
			}
			break;
			}
		};
		filedialoghelper(params, onSuccess);

#endif // PLATFORM_UWP

#else
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

		std::vector<std::string> extensions = {params.description};
		extensions.reserve(params.extensions.size()+1);
		extensions.insert(extensions.end(), params.extensions.cbegin(), params.extensions.cend());

		switch (params.type) {
			case FileDialogParams::OPEN: {
				std::vector<std::string> selection = pfd::open_file(
						params.description,
						std::filesystem::current_path().string(),
						extensions
						// allow multi selection here
				).result();
				// result() will wait for user action before returning.
				// This operation will block and return the user choice

				if (!selection.empty()) {
					onSuccess(selection[0]);
				}
				//TODO what happens if the user cancelled the action? Is there not a onFailure callback?
			}
			case FileDialogParams::SAVE: {
				std::string destination = pfd::save_file(params.description,
						std::filesystem::current_path().string(),
						extensions
						// remove overwrite warning here
				).result();
				onSuccess(destination);
			}
		}
#endif // _WIN32
	}

	bool Bin2H(const uint8_t* data, size_t size, const std::string& dst_filename, const char* dataName)
	{
		std::stringstream ss;
		ss << "const uint8_t " << dataName << "[] = {";
		for (size_t i = 0; i < size; ++i)
		{
			ss << (uint32_t)data[i] << ", ";
		}
		ss << "};" << endl;
		return FileWrite(dst_filename, (uint8_t*)ss.str().c_str(), ss.str().length());
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
		to = std::wstring(from.begin(), from.end()); // TODO
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
		to = std::string(from.begin(), from.end()); // TODO
#endif // _WIN32
	}

	int StringConvert(const char* from, wchar_t* to)
	{
#ifdef _WIN32
		int num = MultiByteToWideChar(CP_UTF8, 0, from, -1, NULL, 0);
		if (num > 0)
		{
			MultiByteToWideChar(CP_UTF8, 0, from, -1, &to[0], num);
		}
#else
		int num = 0; // TODO
		const char * message = "int StringConvert(const char* from, wchar_t* to) not implemented";
		std::cerr << message << std::endl;
		throw std::runtime_error(message);
#endif // _WIN32
		return num;
	}

	int StringConvert(const wchar_t* from, char* to)
	{
#ifdef _WIN32
		int num = WideCharToMultiByte(CP_UTF8, 0, from, -1, NULL, 0, NULL, NULL);
		if (num > 0)
		{
			WideCharToMultiByte(CP_UTF8, 0, from, -1, &to[0], num, NULL, NULL);
		}
#else
		int num = 0; // TODO
		const char * message = "int StringConvert(const wchar_t* from, char* to) not implemented";
		std::cerr << message << std::endl;
		throw std::runtime_error(message);
#endif // _WIN32
		return num;
	}
	
	void Sleep(float milliseconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds((int)milliseconds));
	}

	void Spin(float milliseconds)
	{
		milliseconds /= 1000.0f;
		chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
		double ms = 0;
		while (ms < milliseconds)
		{
			chrono::high_resolution_clock::time_point t2 = chrono::high_resolution_clock::now();
			chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
			ms = time_span.count();
		}
	}
}
