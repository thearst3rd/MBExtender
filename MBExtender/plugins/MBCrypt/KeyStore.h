#include "../../external/cryptopp/rsa.h"
#pragma once
class KeyStore
{
public:
	CryptoPP::RSA::PublicKey rsaPublicKey;
	std::string aesKey;

	void Load();
};