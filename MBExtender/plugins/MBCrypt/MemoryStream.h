#pragma once
#include <cstdint>
#include <cstdbool>
#include <string>
/*
*	Original implementation of MemoryStream in C++
*/
class MemoryStream
{
	// 32 MB Reallocate size cause we gonna be allocating huge shit
	const int REALLOCATE_SIZE = 33554432;

	uint8_t* buffer;
	size_t bufferSize;
	size_t properSize;
	size_t position;

	bool checkEos(bool error = true);
	void reallocate();
public:
	MemoryStream();
	~MemoryStream();
	void createFromBuffer(uint8_t* buffer, size_t count);
	void useBuffer(uint8_t* buffer, size_t count);
	bool readBool();
	int64_t readInt64();
	int32_t readInt32();
	int16_t readInt16();
	int8_t readInt8();
	uint64_t readUInt64();
	uint32_t readUInt32();
	uint16_t readUInt16();
	uint8_t readUInt8();
	float readFloat();
	double readDouble();
	char readChar();
	unsigned char readUChar();
	std::string readString();

	template<typename T>
	void write(T);

	template<typename T>
	T read();

	template<>
	void write(float type)
	{
		writeFloat(type);
	}

	template<>
	void write(double type)
	{
		writeDouble(type);
	}

	template<>
	void write(uint32_t type)
	{
		writeUInt32(type);
	}

	template<>
	void write(uint16_t type)
	{
		writeUInt16(type);
	}

	template<>
	void write(uint8_t type)
	{
		writeUInt8(type);
	}

	template<>
	void write(int32_t type)
	{
		writeInt32(type);
	}

	template<>
	void write(int16_t type)
	{
		writeInt16(type);
	}

	template<>
	void write(int8_t type)
	{
		writeInt8(type);
	}

	template<>
	void write(char type)
	{
		writeChar(type);
	}

	template<>
	void write(bool type)
	{
		writeBool(type);
	}

	template<>
	void write(uint64_t type)
	{
		writeUInt64(type);
	}

	template<>
	void write(int64_t type)
	{
		writeInt64(type);
	}

	template<>
	void write(std::string type)
	{
		writeString(type);
	}

	template<>
	float read()
	{
		return readFloat();
	}

	template<>
	double read()
	{
		return readDouble();
	}

	template<>
	uint64_t read()
	{
		return readUInt64();
	}

	template<>
	uint32_t read()
	{
		return readUInt32();
	}

	template<>
	uint16_t read()
	{
		return readUInt16();
	}

	template<>
	uint8_t read()
	{
		return readUInt8();
	}

	template<>
	int64_t read()
	{
		return readInt64();
	}

	template<>
	int32_t read()
	{
		return readInt32();
	}

	template<>
	int16_t read()
	{
		return readInt16();
	}

	template<>
	int8_t read()
	{
		return readInt8();
	}

	template<>
	char read()
	{
		return readChar();
	}

	template<>
	std::string read()
	{
		return readString();
	}

	template<>
	bool read()
	{
		return readBool();
	}

	void writeBool(bool);
	void writeInt64(int64_t);
	void writeInt32(int32_t);
	void writeInt16(int16_t);
	void writeInt8(int8_t);
	void writeUInt64(uint64_t);
	void writeUInt32(uint32_t);
	void writeUInt16(uint16_t);
	void writeUInt8(uint8_t);
	void writeFloat(float);
	void writeDouble(double);
	void writeChar(char);
	void writeUChar(unsigned char);
	void writeString(std::string);
	void seek(size_t position);
	size_t tell();
	size_t length();
	uint8_t* getBuffer();
};

