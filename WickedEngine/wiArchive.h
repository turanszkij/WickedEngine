#pragma once
#include "CommonInclude.h"

class wiArchive
{
private:
	unsigned long version;
	fstream file;
	bool readMode;
	streamsize pos;
	char* DATA;
public:
	wiArchive(const string& fileName, bool readMode = true);
	~wiArchive();

	unsigned long GetVersion() { return version; }
	bool IsReadMode() { return readMode; }
	bool IsOpen();
	void Close();

	// Write data using file operations
	template<typename T>
	wiArchive& operator<<(const T& data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator<<(const string& data)
	{
		file.seekp(pos);
		size_t len = data.length() + 1; // +1 for the null-terminator
		file.write(reinterpret_cast<const char*>(&len), sizeof(len));
		pos += sizeof(len);
		file.seekp(pos);
		file.write(data.c_str(), sizeof(char)*len);
		pos += sizeof(char)*len;
		return *this;
	}

	// Read data using memory operations
	template<typename T>
	wiArchive& operator >> (T& data)
	{
		memcpy(&data, reinterpret_cast<void*>((int)DATA + (int)pos), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator >> (string& data)
	{
		size_t len;
		(*this) >> len;
		char* str = new char[len];
		memset(str, '\0', sizeof(char)*len);
		memcpy(str, reinterpret_cast<void*>((int)DATA + (int)pos), sizeof(char)*len);
		pos += sizeof(char)*len;
		data = string(str);
		delete[] str;
		return *this;
	}


	//template<typename T>
	//wiArchive& operator >> (T& data)
	//{
	//	file.seekg(pos);
	//	file.read(reinterpret_cast<char*>(&data), sizeof(data));
	//	pos += sizeof(data);
	//	return *this;
	//}
	//wiArchive& operator >> (string& data)
	//{
	//	file.seekg(pos);
	//	size_t len;
	//	file.read(reinterpret_cast<char*>(&len), sizeof(len));
	//	pos += sizeof(len);
	//	char* str = new char[len];
	//	memset(str, '\0', sizeof(char)*len);
	//	file.seekg(pos);
	//	file.read(str, sizeof(char)*len);
	//	pos += sizeof(char)*len;
	//	data = string(str);
	//	delete[] str;
	//	return *this;
	//}
};

