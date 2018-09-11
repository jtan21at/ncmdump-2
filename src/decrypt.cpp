#include "decrypt.h"

#include <cassert>
#include <cstring>
#include <stdexcept>

#include <openssl/bio.h>
#include <openssl/evp.h>

const std::uint8_t aes_core_key[17] = {0x68, 0x7A, 0x48, 0x52, 0x41, 0x6D, 0x73, 0x6F, 0x35, 0x6B, 0x49, 0x6E, 0x62,
                                       0x61, 0x78, 0x57, 0};
const std::uint8_t aes_modify_key[17] = {0x23, 0x31, 0x34, 0x6C, 0x6A, 0x6B, 0x5F, 0x21, 0x5C, 0x5D, 0x26, 0x30, 0x55,
                                         0x3C, 0x27, 0x28, 0};

int base64_decode(const std::uint8_t* in_str, int in_len, std::uint8_t* out_str)
{
    assert(in_str);
    assert(out_str);
    BIO* b64, * bio;
    int size = 0;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    bio = BIO_new_mem_buf(in_str, in_len);
    bio = BIO_push(b64, bio);

    size = BIO_read(bio, out_str, in_len);
    out_str[size] = '\0';

    BIO_free_all(bio);
    return size;
}

int aes128_ecb_decrypt(const std::uint8_t* key, std::uint8_t* in, int in_len, std::uint8_t* out)
{
    int out_len;
    int temp;

    EVP_CIPHER_CTX* x = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_set_padding(x, 1);

    if(!EVP_DecryptInit_ex(x, EVP_aes_128_ecb(), nullptr, key, nullptr))
        throw std::runtime_error("EVP_DecryptInit_ex");

    if(!EVP_DecryptUpdate(x, out, &out_len, in, in_len))
        throw std::runtime_error("EVP_DecryptUpdate");

    if(!EVP_DecryptFinal_ex(x, out + out_len, &temp))
        throw std::runtime_error("EVP_DecryptFinal_ex");

    out_len += temp;

    EVP_CIPHER_CTX_free(x);

    return out_len;
}

std::unique_ptr<std::uint8_t[]> build_key_box(const std::uint8_t* key, int key_len)
{
    std::unique_ptr<std::uint8_t[]> box(new std::uint8_t[256]{0});
    auto pbox = box.get();

    for(int i = 0; i < 256; ++i)
        pbox[i] = static_cast<std::uint8_t>(i);

    std::uint8_t swap = 0;
    std::uint8_t c = 0;
    std::uint8_t last_byte = 0;
    std::uint8_t key_offset = 0;

    for(int i = 0; i < 256; ++i)
    {
        swap = pbox[i];
        c = ((swap + last_byte + key[key_offset++]) & static_cast<std::uint8_t>(0xff));
        if(key_offset >= key_len)
            key_offset = 0;
        pbox[i] = pbox[c];
        pbox[c] = swap;
        last_byte = c;
    }

    return std::move(box);
}

void ncmdump::prepare()
{
    std::uint32_t ulen = 0;

    d_ifs.read(reinterpret_cast<char*>(&ulen), sizeof(ulen));
    if(ulen != 0x4e455443)
    {
        d_failbit = failure_t::invalid_ncm_format;
        return;
    }

    d_ifs.read(reinterpret_cast<char*>(&ulen), sizeof(ulen));
    if(ulen != 0x4d414446)
    {
        d_failbit = failure_t::invalid_ncm_format;
        return;
    }

    d_ifs.ignore(2);

    std::uint32_t key_len = 0;
    d_ifs.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));

    std::unique_ptr<std::uint8_t[]> key_data(new std::uint8_t[key_len]{0});
    d_ifs.read(reinterpret_cast<char*>(key_data.get()), key_len);
    {
        auto pdata = key_data.get();
        for(std::uint32_t i = 0; i < key_len; ++i)
            pdata[i] ^= 0x64;
    }

    int de_key_len = 0;
    std::unique_ptr<std::uint8_t[]> de_key_data(new std::uint8_t[key_len]{0});

    de_key_len = aes128_ecb_decrypt(aes_core_key, key_data.get(), key_len, de_key_data.get());

    d_ifs.read(reinterpret_cast<char*>(&ulen), sizeof(ulen));

    std::unique_ptr<std::uint8_t[]> modify_data(new std::uint8_t[ulen]{0});
    d_ifs.read(reinterpret_cast<char*>(modify_data.get()), ulen);
    {
        auto pmd = modify_data.get();
        for(std::uint32_t i = 0; i < ulen; ++i)
            pmd[i] ^= 0x63;
    }

    // offset header 22
    int data_len;
    std::unique_ptr<std::uint8_t[]> data(new std::uint8_t[ulen]{0});
    std::unique_ptr<std::uint8_t[]> dedata(new std::uint8_t[ulen]{0});

    data_len = base64_decode(modify_data.get() + 22, ulen - 22, data.get());
    data_len = aes128_ecb_decrypt(aes_modify_key, data.get(), data_len, dedata.get());
    std::memmove(dedata.get(), dedata.get() + 6, data_len - 6);
    dedata.get()[data_len - 6] = 0;
    
    std::unique_ptr<Json::CharReader> json_parser{Json::CharReaderBuilder{}.newCharReader()};
    {
        std::string err;
        auto json_beg = reinterpret_cast<char*>(dedata.get());
        auto json_end = reinterpret_cast<char*>(dedata.get()) + data_len - 6;
        if(!json_parser->parse(json_beg, json_end, &d_meta, &err))
            d_failbit = failure_t::invalid_metadata;
    }

    // read crc32 check
    d_ifs.read(reinterpret_cast<char*>(&ulen), sizeof(ulen));
    d_ifs.ignore(5);

    std::uint32_t img_len{0};
    d_ifs.read(reinterpret_cast<char*>(&img_len), sizeof(img_len));
    d_extra.img_len = img_len;
    d_extra.img_data.reset(new char[img_len]);
    d_ifs.read(d_extra.img_data.get(), img_len);

    dp_box = std::move(build_key_box(de_key_data.get() + 17, de_key_len - 17));
}

void ncmdump::dump(std::ostream& os)&
{
    assert(os.good());

    auto pbox = dp_box.get();
    std::streamsize n = 0x8000;
    std::unique_ptr<std::uint8_t[]> buffer(new std::uint8_t[n]{0});
    auto pbuf = reinterpret_cast<char*>(buffer.get());

    while(n > 1)
    {
        d_ifs.read(pbuf, n);
        n = d_ifs.gcount(); // prepare for eof
        for(auto i = 0; i < n; ++i)
        {
            int j = (i + 1) & 0xff;
            pbuf[i] ^= pbox[(pbox[j] + pbox[(pbox[j] + j) & 0xff]) & 0xff];
        }

        os.write(pbuf, n);
    }

    d_ifs.close();  
    os.flush();
}
