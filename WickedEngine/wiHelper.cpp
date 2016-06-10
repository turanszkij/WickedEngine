#include "wiHelper.h"
#include "wiRenderer.h"
#include "wiBackLog.h"
#include "wiWindowRegistration.h"

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

	string GetWorkingDirectory()
	{
		char result[MAX_PATH];
		return std::string(result, GetModuleFileNameA(NULL, result, MAX_PATH));
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
}
