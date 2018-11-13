#include "wiHelper.h"
#include "wiRenderer.h"
#include "wiBackLog.h"
#include "wiWindowRegistration.h"

#include "Utility/stb_image_write.h"

#include <locale>
#include <direct.h>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>

using namespace std;

namespace wiHelper
{
	DataBlob::DataBlob(const string& fileName) : DATA(nullptr), dataSize(0), dataPos(0)
	{
		ifstream file(fileName, ios::binary | ios::ate);

		if (file.is_open())
		{
			dataSize = static_cast<size_t>(file.tellg());
			file.seekg(0, file.beg);
			DATA = new char[dataSize];
			file.read(DATA, dataSize);
			file.close();

			dataPos = reinterpret_cast<size_t>(DATA);
		}
	}
	DataBlob::~DataBlob()
	{
		SAFE_DELETE_ARRAY(DATA);
	}

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

	bool readByteData(const std::string& fileName, std::vector<uint8_t>& data)
	{
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
		stringstream ss("");
		ss << "File not found: " << fileName;
		messageBox(ss.str());
		return false;
	}

	void messageBox(const std::string& msg, const std::string& caption){
#ifndef WINSTORE_SUPPORT
		MessageBoxA(wiWindowRegistration::GetRegisteredWindow(), msg.c_str(), caption.c_str(), 0);
#else
		wstring wmsg(msg.begin(), msg.end());
		wstring wcaption(caption.begin(), caption.end());
		Windows::UI::Popups::MessageDialog(ref new Platform::String(wmsg.c_str()), ref new Platform::String(wcaption.c_str())).ShowAsync();
#endif
	}

	void screenshot(const std::string& name)
	{
		CreateDirectoryA("screenshots", 0);
		stringstream ss("");
		if (name.length() <= 0)
			ss << GetOriginalWorkingDirectory() << "screenshots/sc_" << getCurrentDateTimeAsString() << ".jpg";
		else
			ss << name;

		using namespace wiGraphicsTypes;

		GraphicsDevice* device = wiRenderer::GetDevice();
		
		device->WaitForGPU();

		static Texture2D tex = device->GetBackBuffer();
		TextureDesc desc = tex.GetDesc();
		UINT data_count = desc.Width * desc.Height;
		UINT data_stride = device->GetFormatStride(desc.Format);
		UINT data_size = data_count * data_stride;

		static unsigned char* data = nullptr;
		static Texture2D* stagingTex = nullptr;
		if (stagingTex == nullptr)
		{
			TextureDesc staging_desc = desc;
			staging_desc.Usage = USAGE_STAGING;
			staging_desc.CPUAccessFlags = CPU_ACCESS_READ;
			staging_desc.BindFlags = 0;
			staging_desc.MiscFlags = 0;
			HRESULT hr = device->CreateTexture2D(&staging_desc, nullptr, &stagingTex);
			assert(SUCCEEDED(hr));

			data = new unsigned char[data_size];
		}

		bool download_success = device->DownloadResource(&tex, stagingTex, data, GRAPHICSTHREAD_IMMEDIATE);
		assert(download_success);

		if (desc.Format == FORMAT_R10G10B10A2_UNORM)
		{
			// So this should be converted first to rgba8 before saving to common format...

			uint32_t* data32 = (uint32_t*)data;

			for (UINT i = 0; i < data_count; ++i)
			{
				uint32_t pixel = data32[i];
				float r = ((pixel >> 0 ) & 1023) / 1023.0f;
				float g = ((pixel >> 10) & 1023) / 1023.0f;
				float b = ((pixel >> 20) & 1023) / 1023.0f;
				float a = ((pixel >> 30) & 3   ) / 3.0f;
				
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

		// TODO: png would be better, but it has some problems now...

		//int write_result = stbi_write_png(ss.str().c_str(), (int)desc.Width, (int)desc.Height, 4, data, (int)data_stride);
		int write_result = stbi_write_jpg(ss.str().c_str(), (int)desc.Width, (int)desc.Height, 4, data, 100);
		//int write_result = stbi_write_bmp(ss.str().c_str(), (int)desc.Width, (int)desc.Height, 4, data);
		assert(write_result);

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
		static string __appDir;
		static bool __initComplete = false;
		if (!__initComplete)
		{
			LPSTR fileName = new CHAR[1024];
			GetModuleFileNameA(NULL, fileName, 1024);
			__appDir = GetDirectoryFromPath(fileName);
			delete[] fileName;
			__initComplete = true;
		}
		return __appDir;
	}

	static const string __originalWorkingDir = GetWorkingDirectory();
	string GetOriginalWorkingDirectory()
	{
		return __originalWorkingDir;
	}

	string GetWorkingDirectory()
	{
		return string(_getcwd(NULL, 0)) + "/";
	}

	bool SetWorkingDirectory(const std::string& path)
	{
		return _chdir(path.c_str()) == 0;
	}

	void GetFilesInDirectory(std::vector<string>& out, const std::string& directory)
	{
#ifndef WINSTORE_SUPPORT
		// WINDOWS
		wstring wdirectory = wstring(directory.begin(), directory.end());
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

			out.push_back(string(full_file_name.begin(), full_file_name.end()));
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
