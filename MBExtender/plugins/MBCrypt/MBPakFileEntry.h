#pragma once
#include <string>

class MBPakFileEntry
{
public:
	std::string filepath;
	int64_t uncompressedSize;
	int compressedSize;
	int64_t fileOffset;
	int encrypted;
};