#include "wiHelper.h"
#include "wiPlatform.h"
#include "wiRenderer.h"
#include "wiBackLog.h"

#include "Utility/stb_image_write.h"

#include <thread>
#include <locale>
#include <direct.h>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <codecvt> // string conversion

#ifdef _WIN32
#ifdef PLATFORM_UWP
#include <collection.h>
#include <ppltasks.h>
#else
#include <Commdlg.h> // openfile
#include <WinBase.h>
#endif // PLATFORM_UWP
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
		auto& state = wiPlatform::GetWindowState();
		state.messagemutex.lock();
		state.messages.emplace_back();
		StringConvert(msg, state.messages.back().message);
		StringConvert(caption, state.messages.back().caption);
		state.messagemutex.unlock();
	}

	void screenshot(const std::string& name)
	{
		CreateDirectoryA("screenshots", 0);
		stringstream ss("");
		if (name.length() <= 0)
			ss << GetOriginalWorkingDirectory() << "screenshots/sc_" << getCurrentDateTimeAsString() << ".jpg";
		else
			ss << name;

		bool result = saveTextureToFile(wiRenderer::GetDevice()->GetBackBuffer(), ss.str());
		assert(result);
	}

	bool saveTextureToMemory(const wiGraphics::Texture& texture, std::vector<uint8_t>& data)
	{
		using namespace wiGraphics;

		GraphicsDevice* device = wiRenderer::GetDevice();

		device->WaitForGPU();

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
		HRESULT hr = device->CreateTexture(&staging_desc, nullptr, &stagingTex);
		assert(SUCCEEDED(hr));

		bool download_success = device->DownloadResource(&texture, &stagingTex, data.data());
		assert(download_success);

		return download_success;
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
		if (!extension.compare("JPG"))
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
		localtime_s(&time_info, &t);
		stringstream ss("");
		ss << std::put_time(&time_info, "%d-%m-%Y %H-%M-%S");
		return ss.str();
	}

	string GetApplicationDirectory()
	{
		static string appDir;
		static bool initComplete = false;
		if (!initComplete)
		{
			CHAR fileName[1024] = {};
			GetModuleFileNameA(NULL, fileName, arraysize(fileName));
			appDir = GetDirectoryFromPath(fileName);
			initComplete = true;
		}
		return appDir;
	}

	string GetOriginalWorkingDirectory()
	{
		static const string __originalWorkingDir = GetWorkingDirectory();
		return __originalWorkingDir;
	}

	string GetWorkingDirectory()
	{
		char* cwd = _getcwd(NULL, 0);
		if (cwd != nullptr)
		{
			return string(cwd) + "/";
		}
		return "/";
	}

	bool SetWorkingDirectory(const std::string& path)
	{
		return _chdir(path.c_str()) == 0;
	}

	void GetFilesInDirectory(std::vector<string>& out, const std::string& directory)
	{
#ifndef PLATFORM_UWP
		// WINDOWS
		wstring wdirectory;
		StringConvert(directory, wdirectory);
		HANDLE dir;
		WIN32_FIND_DATA file_data;

		if ((dir = FindFirstFile((wdirectory + L"/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
			return; /* No files found */

		do {
			const wstring file_name = file_data.cFileName;
			const wstring full_file_name = wdirectory + L"/" + file_name;
			const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			//if (file_name[0] == '.')
			//	continue;

			//if (is_directory)
			//	continue;

			string fname;
			StringConvert(full_file_name, fname);
			out.push_back(fname);
		} while (FindNextFile(dir, &file_data));

		FindClose(dir);
#endif

		// UNIX
		//DIR *dir;
		//class dirent *ent;
		//class stat st;

		//dir = opendir(directory);
		//while ((ent = readdir(dir)) != NULL) {
		//	const string file_name = ent->d_name;
		//	const string full_file_name = directory + "/" + file_name;

		//	if (file_name[0] == '.')
		//		continue;

		//	if (stat(full_file_name.c_str(), &st) == -1)
		//		continue;

		//	const bool is_directory = (st.st_mode & S_IFDIR) != 0;

		//	if (is_directory)
		//		continue;

		//	out.push_back(full_file_name);
		//}
		//closedir(dir);
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

	void RemoveExtensionFromFileName(std::string& filename)
	{
		string extension = GetExtensionFromFileName(filename);

		if (!extension.empty())
		{
			filename = filename.substr(0, filename.length() - extension.length() - 1);
		}
	}

	bool FileRead(const std::string& fileName, std::vector<uint8_t>& data)
	{
#ifndef PLATFORM_UWP
		ifstream file(fileName, ios::binary | ios::ate);
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
		using namespace concurrency;
		using namespace Platform;
		using namespace Windows::Storage;
		using namespace Windows::Storage::Streams;
		wstring wstr;
		string filepath = fileName;
		if (filepath.find(":\\") == string::npos)
		{
			filepath = GetWorkingDirectory() + filepath;
		}
		StringConvert(filepath, wstr);
		std::replace(wstr.begin(), wstr.end(), '/', '\\');
		bool success = false;
		std::thread([&] {
			bool end0 = false;
			create_task(StorageFile::GetFileFromPathAsync(ref new String(wstr.c_str()))).then([&](task<StorageFile^> task) {
				
				StorageFile^ file;
				try
				{
					file = task.get();
				}
				catch (Platform::AccessDeniedException^ e)
				{
					wiBackLog::post(("Opening file failed: " + fileName + "\nReason: Permission Denied!").c_str());
					end0 = true;
					return;
				}
				catch (...)
				{
					end0 = true;
					return;
				}

				if (file)
				{
					bool end1 = false;
					create_task(FileIO::ReadBufferAsync(file)).then([&](IBuffer^ buffer) {
						auto reader = DataReader::FromBuffer(buffer);
						auto size = buffer->Length;
						data.resize((size_t)size);
						for (auto& x : data)
						{
							x = reader->ReadByte();
						}
						success = true;
						end1 = true;
						});
					while (!end1) { Sleep(1); }
				}
				end0 = true;
				});
			while (!end0) { Sleep(1); }

		}).join();

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
		using namespace concurrency;
		using namespace Platform;
		using namespace Windows::Storage;
		using namespace Windows::Storage::Streams;
		using namespace Windows::Security::Cryptography;
		wstring wstr;
		string filepath = fileName;
		if (filepath.find(":\\") == string::npos)
		{
			filepath = GetWorkingDirectory() + filepath;
		}
		StringConvert(filepath, wstr);
		std::replace(wstr.begin(), wstr.end(), '/', '\\');
		bool success = false;
		std::thread([&] {
			bool end0 = false;
			create_task(StorageFile::GetFileFromPathAsync(ref new String(wstr.c_str()))).then([&](StorageFile^ file) {
				if (file)
				{
					bool end1 = false;
					create_task(FileIO::WriteBytesAsync(file, ref new Platform::Array<unsigned char>((unsigned char*)data, (unsigned int)size))).then([&]() {
						success = true;
						end1 = true;
						});
					while (!end1) { Sleep(1); }
				}
				end0 = true;
				});
			while (!end0) { Sleep(1); }
			}).join();

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
		using namespace concurrency;
		using namespace Platform;
		using namespace Windows::Storage;
		using namespace Windows::Storage::Streams;
		string filepath = fileName;
		if (filepath.find(":\\") == string::npos)
		{
			filepath = GetWorkingDirectory() + filepath;
		}
		string directory, name;
		SplitPath(filepath, directory, name);
		wstring wdir, wname;
		StringConvert(directory, wdir);
		StringConvert(name, wname);
		std::replace(wdir.begin(), wdir.end(), '/', '\\');
		bool success = false;
		std::thread([&] {
			bool end0 = false;
			create_task(StorageFolder::GetFolderFromPathAsync(ref new String(wdir.c_str()))).then([&](task<StorageFolder^> task) {

				StorageFolder^ folder;
				try
				{
					folder = task.get();
				}
				catch (Platform::AccessDeniedException^ e)
				{
					wiBackLog::post(("Opening file failed: " + fileName + "\nReason: Permission Denied!").c_str());
					end0 = true;
					return;
				}

				if (folder)
				{
					bool end1 = false;
					create_task(folder->TryGetItemAsync(ref new String(wname.c_str()))).then([&](IStorageItem^ item) {
						if (item)
						{
							success = true;
						}
						end1 = true;
					});
					while (!end1) { Sleep(1); }
				}
				end0 = true;
			});
			while (!end0) { Sleep(1); }
		}).join();

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
				ok = GetOpenFileNameA(&ofn) == TRUE;
				break;
			case FileDialogParams::SAVE:
				ofn.Flags = OFN_OVERWRITEPROMPT;
				ok = GetSaveFileNameA(&ofn) == TRUE;
				break;
			}

			if (ok)
			{
				onSuccess(ofn.lpstrFile);
			}

			}).detach();

#else

		using namespace concurrency;
		using namespace Platform;
		using namespace Windows::Storage;
		using namespace Windows::Storage::Pickers;
		using namespace Windows::Storage::AccessCache;

		switch (params.type)
		{
		case FileDialogParams::OPEN:
		{
			FileOpenPicker^ picker = ref new FileOpenPicker();
			picker->ViewMode = PickerViewMode::List;
			picker->SuggestedStartLocation = PickerLocationId::Objects3D;

			for (auto& x : params.extensions)
			{
				wstring wstr;
				StringConvert(x, wstr);
				wstr = L"." + wstr;
				picker->FileTypeFilter->Append(ref new String(wstr.c_str()));
			}

			create_task(picker->PickSingleFileAsync()).then([=](StorageFile^ file) {
				if (file)
				{
					auto futureaccess = StorageApplicationPermissions::FutureAccessList;
					futureaccess->Clear();
					futureaccess->Add(file);
					wstring wstr = file->Path->Data();
					string str;
					StringConvert(wstr, str);

					// Need to verify that parent folder is accessible:
					create_task(file->GetParentAsync()).then([=](StorageFolder^ folder) {
						if (folder)
						{
							onSuccess(str);
						}
						else
						{
							// Folder not accessible:
							using namespace Windows::UI::Popups;
							auto msg = ref new MessageDialog(
								ref new Platform::String(L"No permission to folder!\nPlease pick the current folder to receive permission!\nOtherwise, some files might fail to load."),
								ref new Platform::String(L"Warning!")
							);
							create_task(msg->ShowAsync()).then([=](IUICommand^ cmd) {

								FolderPicker^ folderpicker = ref new FolderPicker();
								folderpicker->ViewMode = PickerViewMode::List;
								folderpicker->SuggestedStartLocation = PickerLocationId::ComputerFolder;

								for (auto& x : params.extensions)
								{
									wstring wstr;
									StringConvert(x, wstr);
									wstr = L"." + wstr;
									folderpicker->FileTypeFilter->Append(ref new String(wstr.c_str()));
								}

								create_task(folderpicker->PickSingleFolderAsync()).then([=](StorageFolder^ folder) {
									if (folder)
									{
										futureaccess->Add(folder);
										onSuccess(str);
									}
								});
							});
						}
					});
				}
			});
		}
		break;

		case FileDialogParams::SAVE:
		{
			FileSavePicker^ picker = ref new FileSavePicker();
			picker->SuggestedStartLocation = PickerLocationId::ComputerFolder;

			for (auto& x : params.extensions)
			{
				wstring wstr;
				StringConvert(x, wstr);
				wstr = L"." + wstr;
				auto plainTextExtensions = ref new Platform::Collections::Vector<String^>();
				plainTextExtensions->Append(ref new String(wstr.c_str()));
				StringConvert(params.description, wstr);
				picker->FileTypeChoices->Insert(ref new String(wstr.c_str()), plainTextExtensions);
			}

			create_task(picker->PickSaveFileAsync()).then([=](StorageFile^ file) {

				if (file)
				{
					auto futureaccess = StorageApplicationPermissions::FutureAccessList;
					futureaccess->Clear();
					futureaccess->Add(file);
					wstring wstr = file->Path->Data();
					string str;
					StringConvert(wstr, str);
					onSuccess(str);
				}
			});
		}
		break;
		}

#endif // PLATFORM_UWP
#endif // _WIN32
	}

	void StringConvert(const std::string& from, std::wstring& to)
	{
		int num = MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, NULL, 0);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, &to[0], num);
		}
	}

	void StringConvert(const std::wstring& from, std::string& to)
	{
		int num = WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, NULL, 0, NULL, NULL);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, &to[0], num, NULL, NULL);
		}
	}

	int StringConvert(const char* from, wchar_t* to)
	{
		int num = MultiByteToWideChar(CP_UTF8, 0, from, -1, NULL, 0);
		if (num > 0)
		{
			MultiByteToWideChar(CP_UTF8, 0, from, -1, &to[0], num);
		}
		return num;
	}

	int StringConvert(const wchar_t* from, char* to)
	{
		int num = WideCharToMultiByte(CP_UTF8, 0, from, -1, NULL, 0, NULL, NULL);
		if (num > 0)
		{
			WideCharToMultiByte(CP_UTF8, 0, from, -1, &to[0], num, NULL, NULL);
		}
		return num;
	}
	
	void Sleep(float milliseconds)
	{
		::Sleep((DWORD)milliseconds);
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
