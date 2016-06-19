#pragma once
#include "CommonInclude.h"

class wiArchive
{
private:
	unsigned long version;
	fstream file;
	bool readMode;
	streamsize pos;
public:
	wiArchive(const string& fileName, bool readMode = true);
	~wiArchive();

	unsigned long GetVersion() { return version; }
	bool IsReadMode() { return readMode; }
	bool IsOpen();

	template<typename T>
	wiArchive& operator<<(const T& data)
	{
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	template<typename T>
	wiArchive& operator >> (T& data)
	{
		file.read(reinterpret_cast<char*>(&data), pos);
		pos += sizeof(data);
		return *this;
	}

	// special object serializers
	wiArchive& operator<<(const string& data)
	{
		size_t len = data.length();
		file.write(reinterpret_cast<const char*>(&len), sizeof(len));
		pos += sizeof(len);
		file.write(data.c_str(), sizeof(char)*len);
		pos += sizeof(char)*len;
		return *this;
	}
	wiArchive& operator>>(string& data)
	{
		size_t len;
		file.read(reinterpret_cast<char*>(&len), sizeof(len));
		pos += sizeof(len);
		char* str = new char[len];
		file.read(str, sizeof(char)*len);
		pos += sizeof(char)*len;
		data = string(str);
		delete[] str;
		return *this;
	}
};

