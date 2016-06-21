#pragma once
#include "CommonInclude.h"
#include <stdint.h>

class wiArchive
{
private:
	uint64_t version;
	fstream file;
	bool readMode;
	streamsize pos;
	char* DATA;
public:
	wiArchive(const string& fileName, bool readMode = true);
	~wiArchive();

	uint64_t GetVersion() { return version; }
	bool IsReadMode() { return readMode; }
	bool IsOpen();
	void Close();

	// It could be templated but we have to be extremely careful of different datasizes on different platforms
	// because serialized data should be interchangeable!
	// So providing exact copy operations for exact types enforces platform agnosticism

	// Write data using file operations
	wiArchive& operator<<(bool data)
	{
		uint32_t temp = (uint32_t)(data ? 1 : 0);
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&temp), sizeof(temp));
		pos += sizeof(temp);
		return *this;
	}
	wiArchive& operator<<(int data)
	{
		int64_t temp = (int64_t)data;
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&temp), sizeof(temp));
		pos += sizeof(temp);
		return *this;
	}
	wiArchive& operator<<(unsigned int data)
	{
		uint64_t temp = (uint64_t)data;
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&temp), sizeof(temp));
		pos += sizeof(temp);
		return *this;
	}
	wiArchive& operator<<(long data)
	{
		int64_t temp = (int64_t)data;
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&temp), sizeof(temp));
		pos += sizeof(temp);
		return *this;
	}
	wiArchive& operator<<(unsigned long data)
	{
		uint64_t temp = (uint64_t)data;
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&temp), sizeof(temp));
		pos += sizeof(temp);
		return *this;
	}
	wiArchive& operator<<(long long data)
	{
		int64_t temp = (int64_t)data;
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&temp), sizeof(temp));
		pos += sizeof(temp);
		return *this;
	}
	wiArchive& operator<<(unsigned long long data)
	{
		uint64_t temp = (uint64_t)data;
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&temp), sizeof(temp));
		pos += sizeof(temp);
		return *this;
	}
	wiArchive& operator<<(float data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator<<(double data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT2& data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT3& data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT4& data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT3X3& data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT4X3& data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT4X4& data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator<<(const string& data)
	{
		file.seekp(pos);
		uint64_t len = (uint64_t)(data.length() + 1); // +1 for the null-terminator
		file.write(reinterpret_cast<const char*>(&len), sizeof(len));
		pos += sizeof(len);
		file.seekp(pos);
		file.write(data.c_str(), sizeof(char)*len);
		pos += sizeof(char)*len;
		return *this;
	}

	// Read data using memory operations
	wiArchive& operator >> (bool& data)
	{
		uint32_t temp;
		memcpy(&temp, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(temp));
		pos += sizeof(temp);
		data = (temp == 1);
		return *this;
	}
	wiArchive& operator >> (int& data)
	{
		int64_t temp;
		memcpy(&temp, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(temp));
		pos += sizeof(temp);
		data = (int)temp;
		return *this;
	}
	wiArchive& operator >> (unsigned int& data)
	{
		uint64_t temp;
		memcpy(&temp, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(temp));
		pos += sizeof(temp);
		data = (unsigned int)temp;
		return *this;
	}
	wiArchive& operator >> (long& data)
	{
		int64_t temp;
		memcpy(&temp, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(temp));
		pos += sizeof(temp);
		data = (long)temp;
		return *this;
	}
	wiArchive& operator >> (unsigned long& data)
	{
		uint64_t temp;
		memcpy(&temp, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(temp));
		pos += sizeof(temp);
		data = (unsigned long)temp;
		return *this;
	}
	wiArchive& operator >> (long long& data)
	{
		int64_t temp;
		memcpy(&temp, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(temp));
		pos += sizeof(temp);
		data = (long long)temp;
		return *this;
	}
	wiArchive& operator >> (unsigned long long& data)
	{
		uint64_t temp;
		memcpy(&temp, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(temp));
		pos += sizeof(temp);
		data = (unsigned long long)temp;
		return *this;
	}
	wiArchive& operator >> (float& data)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator >> (double& data)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT2& data)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT3& data)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT4& data)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT3X3& data)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT4X3& data)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT4X4& data)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(data));
		pos += sizeof(data);
		return *this;
	}
	wiArchive& operator >> (string& data)
	{
		uint64_t len;
		(*this) >> len;
		char* str = new char[(size_t)len];
		memset(str, '\0', (size_t)(sizeof(char)*len));
		memcpy(str, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), (size_t)(sizeof(char)*len));
		pos += (size_t)(sizeof(char)*len);
		data = string(str);
		delete[] str;
		return *this;
	}



//private:
//
//	template<typename T>
//	wiArchive& operator<<(const T& data)
//	{
//		file.seekp(pos);
//		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
//		pos += sizeof(data);
//		return *this;
//	}
//	template<typename T>
//	wiArchive& operator >> (T& data)
//	{
//		memcpy(&data, reinterpret_cast<void*>((int)DATA + (int)pos), sizeof(data));
//		pos += sizeof(data);
//		return *this;
//	}
};

