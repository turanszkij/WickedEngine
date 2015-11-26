#include "wiHelper.h"
#include "wiRenderer.h"
#include "wiBackLog.h"
#include "Utility/ScreenGrab.h"

namespace wiHelper
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

	void messageBox(const string& msg, const string& caption, HWND hWnd){
#ifndef WINSTORE_SUPPORT
		MessageBoxA(hWnd, msg.c_str(), caption.c_str(), 0);
#else
		//Windows::UI::Popups::MessageDialog("ASD").ShowAsync();
#endif
	}

	void screenshot(const string& name)
	{
#ifndef WINSTORE_SUPPORT
		CreateDirectoryA("screenshots", 0);
		stringstream ss("");
		if (name.length() <= 0)
			ss << "screenshots/sc_" << getCurrentDateTimeAsString() << ".png";
		else
			ss << name;
		wstringstream wss(L"");
		wss << ss.str().c_str();
		ID3D11Resource* res = nullptr;
		wiRenderer::renderTargetView->GetResource(&res);
		HRESULT h = SaveWICTextureToFile(wiRenderer::immediateContext, res, GUID_ContainerFormatPng, wss.str().c_str());
		if (FAILED(h))
			wiBackLog::post("Screenshot failed");
		else
		{
			ss << " Saved successfully!";
			wiBackLog::post(ss.str().c_str());
		}
		res->Release();
#endif
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
}
