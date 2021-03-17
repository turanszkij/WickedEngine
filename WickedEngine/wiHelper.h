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

	// Save raw pixel data from the texture to memory
	bool saveTextureToMemory(const wiGraphics::Texture& texture, std::vector<uint8_t>& texturedata);

	// Save texture to memory as a file format
	bool saveTextureToMemoryFile(const wiGraphics::Texture& texture, const std::string& fileExtension, std::vector<uint8_t>& filedata);

	// Save raw texture data to memory as file format
	bool saveTextureToMemoryFile(const std::vector<uint8_t>& textureData, const wiGraphics::TextureDesc& desc, const std::string& fileExtension, std::vector<uint8_t>& filedata);

	// Save texture to file format
	bool saveTextureToFile(const wiGraphics::Texture& texture, const std::string& fileName);

	// Save raw texture data to file format
	bool saveTextureToFile(const std::vector<uint8_t>& texturedata, const wiGraphics::TextureDesc& desc, const std::string& fileName);

	std::string getCurrentDateTimeAsString();

	void SplitPath(const std::string& fullPath, std::string& dir, std::string& fileName);

	std::string GetFileNameFromPath(const std::string& path);

	std::string GetDirectoryFromPath(const std::string& path);

	std::string GetExtensionFromFileName(const std::string& filename);

	std::string ReplaceExtension(const std::string& filename, const std::string& extension);

	void MakePathRelative(const std::string& rootdir, std::string& path);

	void MakePathAbsolute(std::string& path);

	void DirectoryCreate(const std::string& path);

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

	// Converts a file into a C++ header file that contains the file contents as byte array.
	//	dataName : the byte array's name
	bool Bin2H(const uint8_t* data, size_t size, const std::string& dst_filename, const char* dataName);

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
