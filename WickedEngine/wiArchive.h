#pragma once
#include "CommonInclude.h"
#include <stdint.h>

#include <string>

class wiArchive
{
private:
	uint64_t version;
	bool readMode;
	size_t pos;
	char* DATA;
	size_t dataSize;

	string fileName; // save to this file on closing if not empty
public:
	wiArchive(const string& fileName, bool readMode = true);
	~wiArchive();

	uint64_t GetVersion() { return version; }
	bool IsReadMode() { return readMode; }
	bool IsOpen();
	void Close();
	bool SaveFile(const string& fileName);
	string GetSourceDirectory();
	string GetSourceFileName();

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
		_write(*data.c_str(), len);
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
		_read(*str, len);
		data = string(str);
		delete[] str;
		return *this;
	}



private:

	// This should not be exposed to avoid misaligning data by mistake
	// Any specific type serialization should be implemented by hand
	// But these can be used as helper functions inside this class

	// Write data using memory operations
	template<typename T>
	void _write(const T& data, uint64_t count = 1)
	{
		size_t _size = (size_t)(sizeof(data)*count);
		size_t _right = pos + _size;
		if (_right > dataSize)
		{
			char* NEWDATA = new char[_right * 2];
			memcpy(NEWDATA, DATA, dataSize);
			dataSize = _right * 2;
			SAFE_DELETE_ARRAY(DATA);
			DATA = NEWDATA;
		}
		memcpy(reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), &data, _size);
		pos = _right;
	}

	// Read data using memory operations
	template<typename T>
	void _read(T& data, uint64_t count = 1)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA + (uint64_t)pos), (size_t)(sizeof(data)*count));
		pos += (size_t)(sizeof(data)*count);
	}
};

