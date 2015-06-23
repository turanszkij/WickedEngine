#include "WickedHelper.h"

std::string WickedHelper::toUpper(const std::string& s)
{
	std::string result;
		std::locale loc;
		for (unsigned int i = 0; i < s.length(); ++i)
		{
			result += std::toupper(s.at(i), loc);
		}
		return result;
}

bool WickedHelper::readByteData(const string& fileName, BYTE** data, size_t& dataSize){
	ifstream file(fileName, ios::binary | ios::ate);
	if (file.is_open()){

		dataSize = file.tellg();
		file.seekg(0, file.beg);
		*data = new BYTE[dataSize];
		file.read((char*)*data, dataSize);
		file.close();

		return true;
	}
	stringstream ss("");
	ss << "File not found: " << fileName;
	warningMessage(ss.str());
	return false;
}

void WickedHelper::warningMessage(const string& msg){
	MessageBoxA(nullptr, msg.c_str(), "Warning!", 0);
}