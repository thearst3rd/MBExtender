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

MBPakFile::MBPakFile(std::string path, KeyStore* keys)
{
	this->path = path;
	this->keys = keys;
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
	char* str = new char[stringlen + 1];
	stream.read(str, stringlen);
	str[stringlen] = '\0';
	std::string ret = std::string(str);
	delete[] str;
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

	char* buffer = new char[sz];
	stream.read(buffer, sz);

	if (!this->VerifySignature(buffer, sz, this->keys->rsaPublicKey, this->key, this->keyLength))
	{
		throw std::exception("Data integrity failed!");
	}
}

bool MBPakFile::VerifySignature(char* databuffer, size_t datalen, CryptoPP::RSA::PublicKey publickey, char* sign, size_t signlen)
{
	CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Verifier verifier(publickey);
	bool result = verifier.VerifyMessage((CryptoPP::byte*)databuffer, datalen, (CryptoPP::byte*)sign, signlen);
	return result;
}

char* MBPakFile::Decrypt(std::string filepath, std::string keyStr, int64_t* size)
{
	MBPakFileEntry* entry = NULL;

	//CryptoPP::SHA256 sha;
	//CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
	//sha.CalculateDigest(digest, (CryptoPP::byte*)keyStr.c_str(), keyStr.length());

	//CryptoPP::HexEncoder encoder;
	//std::string output;
	//encoder.Attach(new CryptoPP::StringSink(output));
	//encoder.Put(digest, sizeof(digest));
	//encoder.MessageEnd();

	CryptoPP::byte* keyBytes = (CryptoPP::byte*)keyStr.data();

	for (int i = 0; i < entries.size(); i++)
	{
		if (entries[i].filepath == filepath)
		{
			entry = &entries[i];
			break;
		}
	}
	if (entry != NULL)
	{
		char* encryptedContents = new char[entry->compressedSize];

		std::ifstream f = std::ifstream(this->path, std::ifstream::binary);
		f.seekg(entry->fileOffset);
		f.read(encryptedContents, entry->compressedSize);
		f.close();

		if (!entry->encrypted)
			return encryptedContents;

		MemoryStream encryptedData;
		encryptedData.createFromBuffer((uint8_t*)encryptedContents, entry->compressedSize);

		int ivLen = encryptedData.readInt32();
		char* ivRaw = new char[ivLen];
		for (int i = 0; i < ivLen; i++)
		{
			ivRaw[i] = encryptedData.readChar();
		}

		CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption d;
		d.SetKeyWithIV(reinterpret_cast<const CryptoPP::byte*>(&keyBytes[0]), 32, reinterpret_cast<const CryptoPP::byte*>(&ivRaw[0]), ivLen);

		CryptoPP::StreamTransformationFilter decryptor(d);

		int maxLen = encryptedData.length() - encryptedData.tell();

		for (int i = 0; i < maxLen; i++)
		{
			CryptoPP::byte b = encryptedData.readChar();
			decryptor.Put(b);
		}

		decryptor.MessageEnd();

		int64_t decryptSize = decryptor.MaxRetrievable();
		char* decryptedData = new char[decryptSize];

		decryptor.Get((CryptoPP::byte*)decryptedData, decryptSize);

		*size = decryptSize;

		delete[] ivRaw;

		return decryptedData;
	}
	return NULL;
}

char* MBPakFile::ReadFile(std::string filepath, std::string keyStr, int64_t* size)
{
	MBPakFileEntry* entry = NULL;

	for (int i = 0; i < entries.size(); i++)
	{
		if (entries[i].filepath == filepath)
		{
			entry = &entries[i];
			break;
		}
	}

	if (entry != NULL)
	{
		// First decrypt the thing
		int64_t zipSize;
		char* buffer = this->Decrypt(filepath, keyStr, &zipSize);
		if (buffer != NULL)
		{
			uLongf uSize = entry->uncompressedSize;
			char* uncompressBuffer = new char[uSize];

			uncompress((Bytef*)uncompressBuffer, &uSize, (Bytef*)buffer, zipSize);

			*size = uSize;
			return uncompressBuffer;
		}
		else
		{
			return NULL;
		}
	}
}