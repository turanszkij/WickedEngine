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

	// Write operations
	wiArchive& operator<<(bool data)
	{
		_write((uint32_t)(data ? 1 : 0));
		return *this;
	}
	wiArchive& operator<<(int data)
	{
		_write((int64_t)data);
		return *this;
	}
	wiArchive& operator<<(unsigned int data)
	{
		_write((uint64_t)data);
		return *this;
	}
	wiArchive& operator<<(long data)
	{
		_write((int64_t)data);
		return *this;
	}
	wiArchive& operator<<(unsigned long data)
	{
		_write((uint64_t)data);
		return *this;
	}
	wiArchive& operator<<(long long data)
	{
		_write((int64_t)data);
		return *this;
	}
	wiArchive& operator<<(unsigned long long data)
	{
		_write((uint64_t)data);
		return *this;
	}
	wiArchive& operator<<(float data)
	{
		_write(data);
		return *this;
	}
	wiArchive& operator<<(double data)
	{
		_write(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT2& data)
	{
		_write(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT3& data)
	{
		_write(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT4& data)
	{
		_write(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT3X3& data)
	{
		_write(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT4X3& data)
	{
		_write(data);
		return *this;
	}
	wiArchive& operator<<(const XMFLOAT4X4& data)
	{
		_write(data);
		return *this;
	}
	wiArchive& operator<<(const string& data)
	{
		uint64_t len = (uint64_t)(data.length() + 1); // +1 for the null-terminator
		_write(len);
		file.seekp(pos);
		file.write(data.c_str(), sizeof(char)*len);
		pos += sizeof(char)*len;
		return *this;
	}

	// Read operations
	wiArchive& operator >> (bool& data)
	{
		uint32_t temp;
		_read(temp);
		data = (temp == 1);
		return *this;
	}
	wiArchive& operator >> (int& data)
	{
		int64_t temp;
		_read(temp);
		data = (int)temp;
		return *this;
	}
	wiArchive& operator >> (unsigned int& data)
	{
		uint64_t temp;
		_read(temp);
		data = (unsigned int)temp;
		return *this;
	}
	wiArchive& operator >> (long& data)
	{
		int64_t temp;
		_read(temp);
		data = (long)temp;
		return *this;
	}
	wiArchive& operator >> (unsigned long& data)
	{
		uint64_t temp;
		_read(temp);
		data = (unsigned long)temp;
		return *this;
	}
	wiArchive& operator >> (long long& data)
	{
		int64_t temp;
		_read(temp);
		data = (long long)temp;
		return *this;
	}
	wiArchive& operator >> (unsigned long long& data)
	{
		uint64_t temp;
		_read(temp);
		data = (unsigned long long)temp;
		return *this;
	}
	wiArchive& operator >> (float& data)
	{
		_read(data);
		return *this;
	}
	wiArchive& operator >> (double& data)
	{
		_read(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT2& data)
	{
		_read(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT3& data)
	{
		_read(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT4& data)
	{
		_read(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT3X3& data)
	{
		_read(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT4X3& data)
	{
		_read(data);
		return *this;
	}
	wiArchive& operator >> (XMFLOAT4X4& data)
	{
		_read(data);
		return *this;
	}
	wiArchive& operator >> (string& data)
	{
		uint64_t len;
		_read(len);
		char* str = new char[(size_t)len];
		memset(str, '\0', (size_t)(sizeof(char)*len));
		memcpy(str, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), (size_t)(sizeof(char)*len));
		pos += (size_t)(sizeof(char)*len);
		data = string(str);
		delete[] str;
		return *this;
	}



private:

	// This should not be exposed to avoid misaligning data by mistake
	// Any specific type serialization should be implemented by hand
	// But these can be used as helper functions inside this class

	// Write data using stream operations
	template<typename T>
	void _write(const T& data)
	{
		file.seekp(pos);
		file.write(reinterpret_cast<const char*>(&data), sizeof(data));
		pos += sizeof(data);
	}
	// Read data using memory operations
	template<typename T>
	void _read(T& data)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), sizeof(data));
		pos += sizeof(data);
	}
};

