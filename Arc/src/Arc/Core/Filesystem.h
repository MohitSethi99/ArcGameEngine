#pragma once

#include "Buffer.h"

namespace ArcEngine
{
	class Filesystem
	{
	public:
		[[nodiscard]] static Buffer ReadFileBinary(const std::filesystem::path& filepath);
		static void ReadFileText(const std::filesystem::path& filepath, std::string& outString);
		static void WriteFileText(const std::filesystem::path& filepath, const std::string& buffer);
	};
}
