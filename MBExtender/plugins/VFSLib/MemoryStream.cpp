#include "MemoryStream.h"
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <MBExtender/Allocator.h>

MemoryStream::MemoryStream()
{
	this->buffer = (uint8_t*) MBX_Malloc(256);//  new uint8_t[256];
	this->bufferSize = 256;
	this->properSize = 0;
	this->position = 0;
}

MemoryStream::~MemoryStream()
{
	if (this->buffer != NULL)
		MBX_Free(this->buffer);
		// delete[] this->buffer;
}

void MemoryStream::createFromBuffer(uint8_t* buffer, size_t count)
{
	MBX_Free(this->buffer);
	this->buffer = (uint8_t*)MBX_Malloc(count);
	this->bufferSize = count;
	this->properSize = count;
	this->position = 0;
	memcpy(this->buffer, buffer, count);
}

void MemoryStream::useBuffer(uint8_t* buffer, size_t count)
{
	MBX_Free(this->buffer);
	this->buffer = buffer;
	this->bufferSize = count;
	this->properSize = count;
	this->position = 0;
}

char MemoryStream::readChar()
{
	unsigned char chr = readUChar();
	return *(char*)&chr;
}

int8_t MemoryStream::readInt8()
{
	uint8_t n = readUInt8();
	return *(int8_t*)&n;
}

int16_t MemoryStream::readInt16()
{
	uint16_t n = readUInt16();
	return *(int16_t*)&n;
}

int32_t MemoryStream::readInt32()
{
	uint32_t n = readUInt32();
	return *(int32_t*)&n;
}

int64_t MemoryStream::readInt64()
{
	uint64_t n = readUInt64();
	return *(int64_t*)&n;
}

unsigned char MemoryStream::readUChar()
{
	return readUInt8();
}

bool MemoryStream::readBool()
{
	return readUInt8();
}

uint8_t MemoryStream::readUInt8()
{
	checkEos();
	uint8_t ret = this->buffer[this->position];
	this->position++;
	return ret;
}

uint16_t MemoryStream::readUInt16()
{
	checkEos();
	uint16_t num = 0;
	for (int i = 0; i < sizeof(uint16_t); i++)
	{
		uint8_t byte = readUInt8();
		num += (static_cast<uint16_t>(byte) << (8 * i));
	}
	return num;
}

uint32_t MemoryStream::readUInt32()
{
	checkEos();
	uint32_t num = 0;
	for (int i = 0; i < sizeof(uint32_t); i++)
	{
		uint8_t byte = readUInt8();
		num += (static_cast<uint32_t>(byte) << (8 * i));
	}
	return num;
}

uint64_t MemoryStream::readUInt64()
{
	checkEos();
	uint64_t num = 0;
	for (int i = 0; i < sizeof(uint64_t); i++)
	{
		uint8_t byte = readUInt8();
		num += (static_cast<uint64_t>(byte) << (8 * i));
	}
	return num;
}

float MemoryStream::readFloat()
{
	checkEos();
	uint8_t bytes[] = { readUInt8(), readUInt8(), readUInt8(), readUInt8() };
	float ret;
	memcpy(&ret, bytes, 4);
	return ret;
}

double MemoryStream::readDouble()
{
	checkEos();
	uint64_t bytes = readUInt64();
	double ret = *reinterpret_cast<double*>(&bytes);
	return ret;
}

std::string MemoryStream::readString()
{
	int len = readChar();
	char* str = new char[len + 1];
	for (int i = 0; i < len; i++)
		str[i] = readChar();
	str[len] = '\0';
	return std::string(str);
}

void MemoryStream::reallocate()
{
	while (this->position >= this->bufferSize)
	{
		this->bufferSize *= 1.5;
		this->buffer = (uint8_t*)MBX_Realloc(this->buffer, this->bufferSize);
		//uint8_t* newBuffer = new uint8_t[this->bufferSize]();
		//memcpy(newBuffer, this->buffer, this->bufferSize - this->REALLOCATE_SIZE);
		//delete[] this->buffer;
		//this->buffer = newBuffer;
	}
}

void MemoryStream::writeChar(char chr)
{
	if (this->position == this->properSize) {
		reallocate();
		this->properSize++;
	}
	this->buffer[this->position] = chr;
	this->position++;
}

void MemoryStream::writeInt8(int8_t i8)
{
	writeChar(i8);
}

void MemoryStream::writeBool(bool b)
{
	writeUInt8(b);
}

void MemoryStream::writeUChar(unsigned char chr)
{
	if (this->position == this->properSize) {
		reallocate();
		this->properSize++;
	}
	this->buffer[this->position] = chr;
	this->position++;
}

void MemoryStream::writeInt16(int16_t i16)
{
	reallocate();

	for (int i = 0; i < sizeof(uint16_t); i++)
	{
		uint8_t byte = (i16 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeInt32(int32_t i32)
{
	reallocate();
	for (int i = 0; i < sizeof(uint32_t); i++)
	{
		uint8_t byte = (i32 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeInt64(int64_t i64)
{
	reallocate();
	for (int i = 0; i < sizeof(uint64_t); i++)
	{
		uint8_t byte = (i64 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeUInt8(uint8_t i8)
{
	writeUChar(i8);
}

void MemoryStream::writeUInt16(uint16_t i16)
{
	reallocate();
	for (int i = 0; i < sizeof(uint16_t); i++)
	{
		uint8_t byte = (i16 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeUInt32(uint32_t i32)
{
	reallocate();
	for (int i = 0; i < sizeof(uint32_t); i++)
	{
		uint8_t byte = (i32 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeUInt64(uint64_t i64)
{
	reallocate();
	for (int i = 0; i < sizeof(uint64_t); i++)
	{
		uint8_t byte = (i64 >> (8 * i)) & 0xFF;
		writeUInt8(byte);
	}
}

void MemoryStream::writeFloat(float f)
{
	reallocate();
	union {
		float a;
		uint8_t bytes[4];
	} floatbytes;
	floatbytes.a = f;
	writeUInt8(floatbytes.bytes[0]);
	writeUInt8(floatbytes.bytes[1]);
	writeUInt8(floatbytes.bytes[2]);
	writeUInt8(floatbytes.bytes[3]);
}

void MemoryStream::writeDouble(double d)
{
	reallocate();
	uint64_t reinterpreted = *reinterpret_cast<uint64_t*>(&d);
	writeUInt64(reinterpreted);
}

void MemoryStream::writeString(std::string s)
{
	reallocate();
	int len = s.length();
	writeUInt32(len);
	for (int i = 0; i < s.length(); i++)
	{
		writeChar(s[i]);
	}
}

void MemoryStream::writeBuffer(const char* buf, int32_t size)
{
	int curpos = this->position;
	this->position += size;
	this->reallocate();
	memcpy(&this->buffer[curpos], buf, size);

	if (curpos <= this->properSize && curpos + size >= this->properSize) {
		this->properSize = curpos + size;
	}
}

void MemoryStream::seek(size_t position)
{
	this->position = position;
	checkEos();
}

size_t MemoryStream::tell()
{
	return this->position;
}

size_t MemoryStream::length()
{
	return this->properSize;
}

uint8_t* MemoryStream::getBuffer()
{
	return this->buffer;
}

bool MemoryStream::checkEos(bool error)
{
	if (this->position > this->properSize || this->position < 0)
		if (error)
			throw std::runtime_error("End of stream!");
		else
			return true;
	else
		return false;
}