#include "MemoryStream.h"
#include "MBPakFile.h"
#include "MBPakFileEntry.h"
#include "../../external/cryptopp/aes.h"
#include "../../external/cryptopp/cryptlib.h"
#include "../../external/cryptopp/rijndael.h"
#include "../../external/cryptopp/modes.h"
#include "../../external/cryptopp/files.h"
#include "../../external/cryptopp/osrng.h"
#include "../../external/cryptopp/hex.h"
#include "../../external/cryptopp/secblock.h"
#include "../../external/cryptopp/sha.h"
#include "../../external/zlib/zlib.h"
#include <fstream>
#include <exception>
#include <MBExtender/Allocator.h>

MBPakFile::MBPakFile(std::string path, KeyStore* keys)
{
	this->path = path;
	this->keys = keys;
	this->failed = false;
	std::ifstream f;
	f.open(path, std::ifstream::binary);
	this->ReadHeader(f);
	f.close();
}

MBPakFile::~MBPakFile()
{
	if (this->key != NULL)
		delete[] this->key;
}

std::string readString(std::ifstream& stream)
{
	char stringlen;
	stream.read((char*)&stringlen, sizeof(char));
	char* str = (char*)MBX_Malloc(stringlen + 1);
	stream.read(str, stringlen);
	str[stringlen] = '\0';
	std::string ret = std::string(str);
	MBX_Free(str);
	return ret;
}

void MBPakFile::ReadHeader(std::ifstream& stream)
{
	stream.read((char*)&this->keyLength, sizeof(int));
	this->key = new char[keyLength];
	stream.read(this->key, this->keyLength);

	int entryCount;
	stream.read((char*)&entryCount, sizeof(int));
	this->entries.clear();
	for (int i = 0; i < entryCount; i++)
	{
		MBPakFileEntry entry;
		entry.filepath = readString(stream);
		stream.read((char*)&entry.encrypted, sizeof(uint8_t));
		stream.read((char*)&entry.fileOffset, sizeof(int64_t));
		stream.read((char*)&entry.uncompressedSize, sizeof(int64_t));
		stream.read((char*)&entry.compressedSize, sizeof(int));
		this->entryMap.insert(std::make_pair(entry.filepath, this->entries.size()));
		this->entries.push_back(entry);
	}

	int offset = stream.tellg();
	std::streampos thispos = stream.tellg();

	for (int i = 0; i < entryCount; i++)
	{
		this->entries[i].fileOffset += offset;
	}

	stream.seekg(0, std::ios::end);
	int sz = stream.tellg() - thispos;
	stream.seekg(offset);

	char* buffer = (char*)MBX_Malloc(sz);
	stream.read(buffer, sz);

	if (!this->VerifySignature(buffer, sz, this->keys->rsaPublicKey, this->key, this->keyLength))
	{
		MBX_Free(buffer);
		this->failed = true;
		return;
		// throw std::exception("Data integrity failed!");
	}
	MBX_Free(buffer);
}

bool MBPakFile::VerifySignature(char* databuffer, size_t datalen, CryptoPP::RSA::PublicKey publickey, char* sign, size_t signlen)
{
	CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Verifier verifier(publickey);
	bool result = verifier.VerifyMessage((CryptoPP::byte*)databuffer, datalen, (CryptoPP::byte*)sign, signlen);
	return result;
}

char* MBPakFile::Decrypt(MBPakFileEntry* entry, std::string keyStr, int64_t* size)
{
	//CryptoPP::SHA256 sha;
	//CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
	//sha.CalculateDigest(digest, (CryptoPP::byte*)keyStr.c_str(), keyStr.length());

	//CryptoPP::HexEncoder encoder;
	//std::string output;
	//encoder.Attach(new CryptoPP::StringSink(output));
	//encoder.Put(digest, sizeof(digest));
	//encoder.MessageEnd();

	if (entry != NULL)
	{
		char* encryptedContents = (char*)MBX_Malloc(entry->compressedSize);

		std::ifstream f = std::ifstream(this->path, std::ifstream::binary);
		f.seekg(entry->fileOffset);
		f.read(encryptedContents, entry->compressedSize);
		f.close();

		uint32_t num = 0;
		for (int i = 0; i < sizeof(uint32_t); i++)
		{
			uint8_t byte = encryptedContents[i];
			num += (static_cast<uint32_t>(byte) << (8 * i));
		}
		int ivLen = num;

		CryptoPP::byte* ivRaw = (CryptoPP::byte*)&encryptedContents[4];

		CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption d;
		d.SetKeyWithIV(reinterpret_cast<const CryptoPP::byte*>(&keyStr[0]), 32, ivRaw, ivLen);

		CryptoPP::StreamTransformationFilter decryptor(d);

		int maxLen = entry->compressedSize - 4 - ivLen;

		decryptor.Put((CryptoPP::byte*)&encryptedContents[ivLen + 4], maxLen);

		decryptor.MessageEnd();

		int64_t decryptSize = decryptor.MaxRetrievable();
		char* decryptedData = (char*)MBX_Malloc(decryptSize);

		decryptor.Get((CryptoPP::byte*)decryptedData, decryptSize);

		*size = decryptSize;

		MBX_Free(encryptedContents);

		return decryptedData;
	}
	return NULL;
}

char* MBPakFile::ReadFile(MBPakFileEntry* entry, std::string keyStr, int64_t* size)
{
	if (entry != NULL)
	{
		// First decrypt the thing
		int64_t zipSize;
		char* buffer = this->Decrypt(entry, keyStr, &zipSize);
		if (buffer != NULL)
		{
			uLongf uSize = entry->uncompressedSize;
			char* uncompressBuffer = (char*)MBX_Malloc(uSize);

			uncompress((Bytef*)uncompressBuffer, &uSize, (Bytef*)buffer, zipSize);

			*size = uSize;
			MBX_Free(buffer);
			return uncompressBuffer;
		}
		else
		{
			return NULL;
		}
	}
}