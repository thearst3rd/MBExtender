#pragma once
#include <vector>
#include "MBPakFileEntry.h"
#include "MemoryStream.h"
#include "../../external/cryptopp/rsa.h"
#include "KeyStore.h"

class MBPakFile
{
public:
	MBPakFile(std::string path, KeyStore* keys);
	~MBPakFile();

	std::string name;
	std::string path;
	std::vector<MBPakFileEntry> entries;
	char* key;
	int keyLength;
	KeyStore* keys;
	bool failed;

	void ReadHeader(std::ifstream& stream);
	bool VerifySignature(char* databuffer, size_t datalen, CryptoPP::RSA::PublicKey publickey, char* sign, size_t signlen);
	char* Decrypt(MBPakFileEntry* entry, std::string keyStr, int64_t* size);
	char* ReadFile(MBPakFileEntry* entry, std::string keyStr, int64_t* size);
};