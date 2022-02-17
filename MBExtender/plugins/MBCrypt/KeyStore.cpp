#include "KeyStore.h"
#include "../../external/cryptopp/base64.h"
#include "../../external/cryptopp/hex.h"

void KeyStore::Load() {
    // std::string keystr =
    //	"";

    // Put your un-base64-ed rsa public key here for verifying data
    CryptoPP::byte keystr[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                               255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                               255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                               255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                               255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
    CryptoPP::ByteQueue queue;
    queue.Put(keystr, 161);

    // CryptoPP::Base64Decoder decoder;

    // decoder.Attach(new CryptoPP::Redirector(queue));
    // decoder.Put((const CryptoPP::byte*)keystr.data(), keystr.length());
    // decoder.MessageEnd();

    this->rsaPublicKey.Load(queue);

    // std::string aesKey = "";
    // CryptoPP::Base64Decoder decoder2;
    // decoder2.Put((CryptoPP::byte*)aesKey.data(), aesKey.size());
    // decoder2.MessageEnd();

    // std::string decoded;

    // int size = decoder2.MaxRetrievable();
    // if (size && size <= SIZE_MAX)
    //{
    //	decoded.resize(size);
    //	decoder2.Get((CryptoPP::byte*)&decoded[0], decoded.size());
    // }

    // Put your un-base64-ed AES key here
    const char aesKey[] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

    std::string decoded = std::string(reinterpret_cast<char*>(aesKey), 32);

    this->aesKey = decoded;
}