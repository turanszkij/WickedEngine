#pragma once
#include "CommonInclude.h"
#include <stdint.h>

#include <string>
#include <vector>

class wiArchive
{
private:
	uint64_t version = 0;
	bool readMode = false;
	size_t pos = 0;
	std::vector<uint8_t> DATA;

	std::string fileName; // save to this file on closing if not empty
	std::string directory;

	void CreateEmpty();

public:
	// Create empty arhive for writing
	wiArchive();
	wiArchive(const wiArchive&) = default;
	wiArchive(wiArchive&&) = default;
	// Create archive and link to file
	wiArchive(const std::string& fileName, bool readMode = true);
	~wiArchive() { Close(); }

	wiArchive& operator=(const wiArchive&) = default;
	wiArchive& operator=(wiArchive&&) = default;

	const uint8_t* GetData() const { return DATA.data(); }
	size_t GetSize() const { return pos; }
	uint64_t GetVersion() const { return version; }
	bool IsReadMode() const { return readMode; }
	void SetReadModeAndResetPos(bool isReadMode);
	bool IsOpen();
	void Close();
	bool SaveFile(const std::string& fileName);
	const std::string& GetSourceDirectory() const;
	const std::string& GetSourceFileName() const;

	// It could be templated but we have to be extremely careful of different datasizes on different platforms
	// because serialized data should be interchangeable!
	// So providing exact copy operations for exact types enforces platform agnosticism

	// Write operations
	inline wiArchive& operator<<(bool data)
	{
		_write((uint32_t)(data ? 1 : 0));
		return *this;
	}
	inline wiArchive& operator<<(char data)
	{
		_write((int8_t)data);
		return *this;
	}
	inline wiArchive& operator<<(unsigned char data)
	{
		_write((uint8_t)data);
		return *this;
	}
	inline wiArchive& operator<<(int data)
	{
		_write((int64_t)data);
		return *this;
	}
	inline wiArchive& operator<<(unsigned int data)
	{
		_write((uint64_t)data);
		return *this;
	}
	inline wiArchive& operator<<(long data)
	{
		_write((int64_t)data);
		return *this;
	}
	inline wiArchive& operator<<(unsigned long data)
	{
		_write((uint64_t)data);
		return *this;
	}
	inline wiArchive& operator<<(long long data)
	{
		_write((int64_t)data);
		return *this;
	}
	inline wiArchive& operator<<(unsigned long long data)
	{
		_write((uint64_t)data);
		return *this;
	}
	inline wiArchive& operator<<(float data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(double data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const XMFLOAT2& data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const XMFLOAT3& data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const XMFLOAT4& data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const XMFLOAT3X3& data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const XMFLOAT4X3& data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const XMFLOAT4X4& data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const XMUINT2& data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const XMUINT3& data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const XMUINT4& data)
	{
		_write(data);
		return *this;
	}
	inline wiArchive& operator<<(const std::string& data)
	{
		uint64_t len = (uint64_t)(data.length() + 1); // +1 for the null-terminator
		_write(len);
		_write(*data.c_str(), len);
		return *this;
	}
	template<typename T>
	inline wiArchive& operator<<(const std::vector<T>& data)
	{
		// Here we will use the << operator so that non-specified types will have compile error!
		(*this) << data.size();
		for (const T& x : data)
		{
			(*this) << x;
		}
		return *this;
	}

	// Read operations
	inline wiArchive& operator >> (bool& data)
	{
		uint32_t temp;
		_read(temp);
		data = (temp == 1);
		return *this;
	}
	inline wiArchive& operator >> (char& data)
	{
		int8_t temp;
		_read(temp);
		data = (char)temp;
		return *this;
	}
	inline wiArchive& operator >> (unsigned char& data)
	{
		uint8_t temp;
		_read(temp);
		data = (unsigned char)temp;
		return *this;
	}
	inline wiArchive& operator >> (int& data)
	{
		int64_t temp;
		_read(temp);
		data = (int)temp;
		return *this;
	}
	inline wiArchive& operator >> (unsigned int& data)
	{
		uint64_t temp;
		_read(temp);
		data = (unsigned int)temp;
		return *this;
	}
	inline wiArchive& operator >> (long& data)
	{
		int64_t temp;
		_read(temp);
		data = (long)temp;
		return *this;
	}
	inline wiArchive& operator >> (unsigned long& data)
	{
		uint64_t temp;
		_read(temp);
		data = (unsigned long)temp;
		return *this;
	}
	inline wiArchive& operator >> (long long& data)
	{
		int64_t temp;
		_read(temp);
		data = (long long)temp;
		return *this;
	}
	inline wiArchive& operator >> (unsigned long long& data)
	{
		uint64_t temp;
		_read(temp);
		data = (unsigned long long)temp;
		return *this;
	}
	inline wiArchive& operator >> (float& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (double& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (XMFLOAT2& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (XMFLOAT3& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (XMFLOAT4& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (XMFLOAT3X3& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (XMFLOAT4X3& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (XMFLOAT4X4& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (XMUINT2& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (XMUINT3& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (XMUINT4& data)
	{
		_read(data);
		return *this;
	}
	inline wiArchive& operator >> (std::string& data)
	{
		uint64_t len;
		_read(len);
		char* str = new char[(size_t)len];
		memset(str, '\0', (size_t)(sizeof(char)*len));
		_read(*str, len);
		data = std::string(str);
		delete[] str;
		return *this;
	}
	template<typename T>
	inline wiArchive& operator >> (std::vector<T>& data)
	{
		// Here we will use the >> operator so that non-specified types will have compile error!
		size_t count;
		(*this) >> count;
		data.resize(count);
		for (size_t i = 0; i < count; ++i)
		{
			(*this) >> data[i];
		}
		return *this;
	}



private:

	// This should not be exposed to avoid misaligning data by mistake
	// Any specific type serialization should be implemented by hand
	// But these can be used as helper functions inside this class

	// Write data using memory operations
	template<typename T>
	inline void _write(const T& data, uint64_t count = 1)
	{
		size_t _size = (size_t)(sizeof(data)*count);
		size_t _right = pos + _size;
		if (_right > DATA.size())
		{
			DATA.resize(_right * 2);
		}
		memcpy(reinterpret_cast<void*>((uint64_t)DATA.data() + (uint64_t)pos), &data, _size);
		pos = _right;
	}

	// Read data using memory operations
	template<typename T>
	inline void _read(T& data, uint64_t count = 1)
	{
		memcpy(&data, reinterpret_cast<void*>((uint64_t)DATA.data() + (uint64_t)pos), (size_t)(sizeof(data)*count));
		pos += (size_t)(sizeof(data)*count);
	}
};

