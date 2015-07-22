#include "wiHelper.h"

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

}
