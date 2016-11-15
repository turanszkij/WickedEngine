#include "wiHelper.h"
#include "wiRenderer.h"
#include "wiBackLog.h"
#include "wiWindowRegistration.h"

#include <locale>
#include <direct.h>
#include <chrono>

namespace wiHelper
{

	string toUpper(const string& s)
	{
		std::string result;
		std::locale loc;
		for (unsigned int i = 0; i < s.length(); ++i)
		{
			result += std::toupper(s.at(i), loc);
		}
		return result;
	}

	bool readByteData(const string& fileName, BYTE** data, size_t& dataSize){
		ifstream file(fileName, ios::binary | ios::ate);
		if (file.is_open()){

			dataSize = (size_t)file.tellg();
			file.seekg(0, file.beg);
			*data = new BYTE[dataSize];
			file.read((char*)*data, dataSize);
			file.close();

			return true;
		}
		stringstream ss("");
		ss << "File not found: " << fileName;
		messageBox(ss.str());
		return false;
	}

	void messageBox(const string& msg, const string& caption){
#ifndef WINSTORE_SUPPORT
		MessageBoxA(wiWindowRegistration::GetInstance()->GetRegisteredWindow(), msg.c_str(), caption.c_str(), 0);
#else
		wstring wmsg(msg.begin(), msg.end());
		wstring wcaption(caption.begin(), caption.end());
		Windows::UI::Popups::MessageDialog(ref new Platform::String(wmsg.c_str()), ref new Platform::String(wcaption.c_str())).ShowAsync();
#endif
	}

	void screenshot(const string& name)
	{
		CreateDirectoryA("screenshots", 0);
		stringstream ss("");
		if (name.length() <= 0)
			ss << "screenshots/sc_" << getCurrentDateTimeAsString() << ".png";
		else
			ss << name;
		if (SUCCEEDED(wiRenderer::GetDevice()->SaveTexturePNG(ss.str(), &wiRenderer::GetDevice()->GetBackBuffer())))
		{
			ss << " Saved successfully!";
			wiBackLog::post(ss.str().c_str());
		}
		else
		{
			wiBackLog::post("Screenshot failed");
		}
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
			LPSTR fileName = new CHAR[MAX_TEXT];
			GetModuleFileNameA(NULL, fileName, MAX_TEXT);
			__appDir = GetDirectoryFromPath(fileName);
			delete[] fileName;
			__initComplete = true;
		}
		return __appDir;
	}

	string GetOriginalWorkingDirectory()
	{
		static const string __originalWorkingDir = GetWorkingDirectory();
		return __originalWorkingDir;
	}

	string GetWorkingDirectory()
	{
		return string(_getcwd(NULL, 0)) + "/";
	}

	bool SetWorkingDirectory(const string& path)
	{
		return _chdir(path.c_str()) == 0;
	}

	void GetFilesInDirectory(vector<string>& out, const string& directory)
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

	void SplitPath(const string& fullPath, string& dir, string& fileName)
	{
		size_t found;
		found = fullPath.find_last_of("/\\");
		dir = fullPath.substr(0, found + 1);
		fileName = fullPath.substr(found + 1);
	}

	string GetFileNameFromPath(const string& fullPath)
	{
		if (fullPath.empty())
		{
			return fullPath;
		}

		string ret, empty;
		SplitPath(fullPath, empty, ret);
		return ret;
	}

	string GetDirectoryFromPath(const string& fullPath)
	{
		if (fullPath.empty())
		{
			return fullPath;
		}

		string ret, empty;
		SplitPath(fullPath, ret, empty);
		return ret;
	}
	
	void Sleep(float milliseconds)
	{
		//::Sleep((DWORD)milliseconds);
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
