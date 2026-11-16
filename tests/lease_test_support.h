#pragma once
#include "../src/main/jni/Security/Lease.h"
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <cassert>
#include <vector>

struct LeaseSigner {
    EVP_PKEY* key=nullptr;
    std::string publicKey,keyId;
    LeaseSigner() {
        auto* generator=EVP_PKEY_CTX_new_id(EVP_PKEY_RSA,nullptr);
        assert(generator&&EVP_PKEY_keygen_init(generator)==1);
        assert(EVP_PKEY_CTX_set_rsa_keygen_bits(generator,3072)==1&&EVP_PKEY_keygen(generator,&key)==1);
        EVP_PKEY_CTX_free(generator);
        BIO* bio=BIO_new(BIO_s_mem());assert(bio&&PEM_write_bio_PUBKEY(bio,key)==1);
        char* data=nullptr;const auto count=BIO_get_mem_data(bio,&data);publicKey.assign(data,count);BIO_free(bio);
        unsigned char* der=nullptr;const int length=i2d_PUBKEY(key,&der);assert(length>0);
        unsigned char digest[SHA256_DIGEST_LENGTH];SHA256(der,length,digest);OPENSSL_free(der);
        constexpr char hex[]="0123456789abcdef";
        for(auto byte:digest) {keyId+=hex[byte>>4];keyId+=hex[byte&15];}
    }
    ~LeaseSigner(){EVP_PKEY_free(key);}
    std::string Sign(const std::string& payload) const {
        auto* context=EVP_MD_CTX_new();assert(context);
        assert(EVP_DigestSignInit(context,nullptr,EVP_sha256(),nullptr,key)==1);
        assert(EVP_DigestSignUpdate(context,payload.data(),payload.size())==1);
        std::size_t size=0;assert(EVP_DigestSignFinal(context,nullptr,&size)==1);
        std::vector<unsigned char> signature(size);assert(EVP_DigestSignFinal(context,signature.data(),&size)==1);
        EVP_MD_CTX_free(context);
        std::vector<unsigned char> encoded((size+2)/3*4+1);
        const int written=EVP_EncodeBlock(encoded.data(),signature.data(),size);assert(written>0);
        return {reinterpret_cast<char*>(encoded.data()),std::size_t(written)};
    }
    std::string Payload(const license::Request& request,std::uint32_t caps=3,std::uint64_t duration=180) const {
        return "v2|CODMGR|1|"+request.key+'|'+request.serial+'|'+request.nonce+'|'+
            std::to_string(request.wall)+'|'+std::to_string(request.wall+duration)+"|2100000000|"+
            std::to_string(caps)+'|'+keyId;
    }
};
