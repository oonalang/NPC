#include "../src/main/jni/Includes/PanelLicense.h"
#include <openssl/rsa.h>
#include <cassert>
#include <iostream>
#include <vector>

int main() {
    EVP_PKEY_CTX* generator = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    assert(generator && EVP_PKEY_keygen_init(generator) == 1);
    assert(EVP_PKEY_CTX_set_rsa_keygen_bits(generator, 2048) == 1);
    EVP_PKEY* key = nullptr;
    assert(EVP_PKEY_keygen(generator, &key) == 1);
    EVP_PKEY_CTX_free(generator);
    BIO* bio = BIO_new(BIO_s_mem());
    assert(bio && PEM_write_bio_PUBKEY(bio, key) == 1);
    char* data = nullptr;
    const long size = BIO_get_mem_data(bio, &data);
    const std::string publicKey(data, static_cast<std::size_t>(size));
    BIO_free(bio);

    const std::string payload = "v1|sample-key|12345678-abcd|2000000000|1";
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    assert(context && EVP_DigestSignInit(context, nullptr, EVP_sha256(), nullptr, key) == 1);
    assert(EVP_DigestSignUpdate(context, payload.data(), payload.size()) == 1);
    std::size_t signatureLength = 0;
    assert(EVP_DigestSignFinal(context, nullptr, &signatureLength) == 1);
    std::vector<unsigned char> signature(signatureLength);
    assert(EVP_DigestSignFinal(context, signature.data(), &signatureLength) == 1);
    signature.resize(signatureLength);
    EVP_MD_CTX_free(context);
    EVP_PKEY_free(key);
    std::vector<unsigned char> encoded((signature.size() + 2) / 3 * 4 + 1);
    const int encodedLength = EVP_EncodeBlock(encoded.data(), signature.data(),
                                               static_cast<int>(signature.size()));
    assert(encodedLength > 0);
    std::string token(reinterpret_cast<char*>(encoded.data()), static_cast<std::size_t>(encodedLength));
    using PanelLicense::Verification;
    auto verify = [&](const std::string& p, const std::string& s, const std::string& k,
                      const std::string& device, std::uint64_t exp, std::uint64_t now) {
        return PanelLicense::Verify(p,s,k,device,exp,now,publicKey.c_str());
    };
    assert(verify(payload,token,"sample-key","12345678-abcd",2000000000,1900000000) == Verification::Valid);
    assert(verify(payload,token,"different","12345678-abcd",2000000000,1900000000) == Verification::InvalidPayload);
    assert(verify(payload,token,"sample-key","87654321-abcd",2000000000,1900000000) == Verification::InvalidPayload);
    assert(verify(payload,token,"sample-key","12345678-abcd",2000000001,1900000000) == Verification::InvalidPayload);
    assert(verify(payload,token,"sample-key","12345678-abcd",2000000000,2000000000) == Verification::Expired);
    assert(verify(payload,token,"sample-key","12345678-abcd",2000000000,0) == Verification::InvalidInput);
    assert(verify(payload+"|extra",token,"sample-key","12345678-abcd",2000000000,1900000000) == Verification::InvalidPayload);
    assert(verify("v1|sample-key|12345678-abcd|02000000000|1",token,"sample-key","12345678-abcd",2000000000,1900000000) == Verification::InvalidPayload);
    assert(verify("v1|sample-key|12345678-abcd|2000000000|2",token,"sample-key","12345678-abcd",2000000000,1900000000) == Verification::InvalidPayload);
    std::string altered = token;
    altered[4] = altered[4] == 'A' ? 'B' : 'A';
    assert(verify(payload,altered,"sample-key","12345678-abcd",2000000000,1900000000) == Verification::InvalidSignature);
    assert(verify(payload,token+"=","sample-key","12345678-abcd",2000000000,1900000000) == Verification::InvalidSignature);
    assert(PanelLicense::OutcomeText(false, "OK") == "License rejected");
    assert(PanelLicense::OutcomeText(false, "").compare("License rejected") == 0);
    assert(PanelLicense::OutcomeText(true, "") == "OK");
    std::cout << "Panel license signature and claims checks passed\n";
}
