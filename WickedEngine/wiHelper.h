#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"

#include <string>
#include <vector>
#include <functional>

namespace wiHelper
{
	template <class T>
	constexpr void hash_combine(std::size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	constexpr size_t string_hash(const char* input) 
	{
		// https://stackoverflow.com/questions/2111667/compile-time-string-hashing
		size_t hash = sizeof(size_t) == 8 ? 0xcbf29ce484222325 : 0x811c9dc5;
		const size_t prime = sizeof(size_t) == 8 ? 0x00000100000001b3 : 0x01000193;

		while (*input) 
		{
			hash ^= static_cast<size_t>(*input);
			hash *= prime;
			++input;
		}

		return hash;
	}

	std::string toUpper(const std::string& s);

	void messageBox(const std::string& msg, const std::string& caption = "Warning!");

	void screenshot(const std::string& name = "");

	bool saveTextureToMemory(const wiGraphics::Texture& texture, std::vector<uint8_t>& data);

	bool saveTextureToFile(const wiGraphics::Texture& texture, const std::string& fileName);

	bool saveTextureToFile(const std::vector<uint8_t>& textureData, const wiGraphics::TextureDesc& desc, const std::string& fileName);

	std::string getCurrentDateTimeAsString();

	std::string GetApplicationDirectory();

	std::string GetOriginalWorkingDirectory();

	std::string GetWorkingDirectory();

	void SetWorkingDirectory(const std::string& path);

	void SplitPath(const std::string& fullPath, std::string& dir, std::string& fileName);

	std::string GetFileNameFromPath(const std::string& fullPath);

	std::string GetDirectoryFromPath(const std::string& fullPath);

	std::string GetExtensionFromFileName(const std::string& filename);

	void RemoveExtensionFromFileName(std::string& filename);

	std::string ExpandPath(const std::string& path);

	bool FileRead(const std::string& fileName, std::vector<uint8_t>& data);

	bool FileWrite(const std::string& fileName, const uint8_t* data, size_t size);

	bool FileExists(const std::string& fileName);

	struct FileDialogParams
	{
		enum TYPE
		{
			OPEN,
			SAVE,
		} type = OPEN;
		std::string description;
		std::vector<std::string> extensions;
	};
	void FileDialog(const FileDialogParams& params, std::function<void(std::string fileName)> onSuccess);

	void StringConvert(const std::string& from, std::wstring& to);

	void StringConvert(const std::wstring& from, std::string& to);

	// Parameter - to - must be pre-allocated!
	// returns result string length
	int StringConvert(const char* from, wchar_t* to);

	// Parameter - to - must be pre-allocated!
	// returns result string length
	int StringConvert(const wchar_t* from, char* to);

	// Puts the current thread to sleeping state for a given time (OS can overtake)
	void Sleep(float milliseconds);

	// Spins for the given time and does nothing (OS can not overtake)
	void Spin(float milliseconds);
};
