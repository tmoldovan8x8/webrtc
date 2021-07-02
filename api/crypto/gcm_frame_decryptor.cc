#include "api/crypto/gcm_frame_decryptor.h"

#include <stdio.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <vector>
#include "rtc_base/logging.h"

namespace webrtc {

static const unsigned char gcm_key[] = {
    195, 130, 222, 164, 47, 57, 241, 245, 151, 138, 25, 165, 95, 71, 146, 
                 67, 189, 29, 194, 5, 9, 22, 33, 224, 139, 35, 60, 122, 146, 97, 169, 206
};

/*static const unsigned char gcm_iv[] = {
    0x99, 0xaa, 0x3e, 0x68, 0xed, 0x81, 0x73, 0xa0, 0xee, 0xd0, 0x66, 0x84
};*/

/*static const unsigned char gcm_pt[] = {
    0xf5, 0x6e, 0x87, 0x05, 0x5b, 0xc3, 0x2d, 0x0e, 0xeb, 0x31, 0xb2, 0xea,
    0xcc, 0x2b, 0xf2, 0xa5
};*/

/*static const unsigned char gcm_aad[] = {
    0x4d, 0x23, 0xc3, 0xce, 0xc3, 0x34, 0xb4, 0x9b, 0xdb, 0x37, 0x0c, 0x43,
    0x7f, 0xec, 0x78, 0xde
};*/

/*static const unsigned char gcm_ct[] = {
    0xf7, 0x26, 0x44, 0x13, 0xa8, 0x4c, 0x0e, 0x7c, 0xd5, 0x36, 0x86, 0x7e,
    0xb9, 0xf2, 0x17, 0x36
};*/

static const unsigned char gcm_tag[] = {
    0x67, 0xba, 0x05, 0x10, 0x26, 0x2a, 0xe4, 0x87, 0xd7, 0x37, 0xee, 0x62,
    0x98, 0xf7, 0x7e, 0x0c
};

 GCMFrameDecryptor::GCMFrameDecryptor() {
      RTC_LOG(LS_VERBOSE) << "XXX GCMFrameDecryptor";
 }

int new_decrypt(unsigned char *ciphertext, 
                int ciphertext_len, 
                unsigned char *key,
                unsigned char *aad, 
                int aad_len,
                unsigned char *iv, 
                unsigned char *plaintext, 
                unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;

    int myUniqueId = rand();
    int len;

    int plaintext_len;

    for (size_t i =0 ; i < ciphertext_len; i++) {
      RTC_LOG(LS_VERBOSE) << "XXX decrypting initial------------------------" << myUniqueId<< " " << i << " " << ciphertext[i];
    }

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        RTC_LOG(LS_VERBOSE) << "XXX decrypting error 21------------------------";

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
         RTC_LOG(LS_VERBOSE) << "XXX decrypting error 22------------------------";

    if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
        RTC_LOG(LS_VERBOSE) << "XXX decrypting error 221------------------------";

    /*
     * Provide any AAD data. This can be called zero or more times as
     * required
     */
    if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
         RTC_LOG(LS_VERBOSE) << "XXX decrypting error 222------------------------";
    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
         RTC_LOG(LS_VERBOSE) << "XXX decrypting error 23------------------------";
    plaintext_len = len;

    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)) {
        RTC_LOG(LS_VERBOSE) << "XXX decrypting error 231------------------------";
    }
    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    int rv = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    if(1 != rv) {
        RTC_LOG(LS_VERBOSE) << "XXX1 decrypting error 24------------------------";
        ERR_print_errors_fp(stderr);
    }
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    RTC_LOG(LS_VERBOSE) << "XXX decrht plaintext_len------------------------" << plaintext_len;
    RTC_LOG(LS_VERBOSE) << "XXX decrht rv------------------------" << rv;

    for (size_t i =0 ; i < plaintext_len; i++) {
      RTC_LOG(LS_VERBOSE) << "XXX decryption final------------------------" << myUniqueId<< " " << i << " " << plaintext[i];
    }
    return plaintext_len;
}

GCMFrameDecryptor::Result GCMFrameDecryptor::Decrypt(
    cricket::MediaType media_type,
    const std::vector<uint32_t>& csrcs,
    rtc::ArrayView<const uint8_t> additional_data,
    rtc::ArrayView<const uint8_t> encrypted_frame,
    rtc::ArrayView<uint8_t> frame) {

 uint8_t unencrypted_bytes = 10;
 switch (media_type) {
    case cricket::MEDIA_TYPE_AUDIO:
      unencrypted_bytes = 1;
      break;
    case cricket::MEDIA_TYPE_VIDEO:
      unencrypted_bytes = 3;
      break;
 }

  RTC_LOG(LS_VERBOSE) << "XXX decrypting------------------------";
  RTC_LOG(LS_VERBOSE) << "XXX unencrypted_bytes ------------------------" << unencrypted_bytes;
  // Frame header
   size_t frame_header_size = unencrypted_bytes;
  std::vector<uint8_t> frame_header;
  for (size_t i = 0; i < unencrypted_bytes; i++) {
    frame[i] = encrypted_frame[i];
    frame_header.push_back(encrypted_frame[i]);
  }

  // Frame trailer
  size_t frame_trailer_size = 2;
  std::vector<uint8_t> frame_trailer;
  frame_trailer.reserve(frame_trailer_size);
  frame_trailer.push_back(encrypted_frame[encrypted_frame.size() - 2]);//IV_LENGHT
  frame_trailer.push_back(encrypted_frame[encrypted_frame.size() - 1]);
  
  // IV
  uint8_t iv_lenght = frame_trailer[0];
  uint8_t iv_start = encrypted_frame.size() - frame_trailer_size - iv_lenght - 1;
  std::vector<uint8_t> iv;
  iv.reserve(iv_lenght);
  for (size_t i = iv_start; i < iv_start + iv_lenght; i++) {
      iv.push_back(encrypted_frame[i]);
  }

  // payload
  size_t payload_lenght = encrypted_frame.size() - (unencrypted_bytes + frame_trailer[0] + frame_trailer_size);
  std::vector<uint8_t> payload;
  payload.reserve(payload_lenght);
  for (size_t i = unencrypted_bytes; i < unencrypted_bytes + payload_lenght; i++) {
    payload.push_back(encrypted_frame[i]);
  }

  std::vector<uint8_t> plaintext;

    /* Buffer for the decrypted text */
    unsigned char decryptedtext[200];
    unsigned char tag[200];

    int decryptedtext_len;

    unsigned char gcm_key1[] = {
                 195, 130, 222, 164, 47, 57, 241, 245, 151, 138, 25, 165, 95, 71, 146, 
                 67, 189, 29, 194, 5, 9, 22, 33, 224, 139, 35, 60, 122, 146, 97, 169, 206
    };

    std::vector<uint8_t> iv1 = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    RTC_LOG(LS_VERBOSE) << "XXX newEncrypt------------------------";

    /* Decrypt the ciphertext */
    decryptedtext_len = new_decrypt(
      &payload[0], 
      payload_lenght, 
      gcm_key1, 
      &frame_header[0],
      frame_header_size,
      &iv1[0], 
      decryptedtext, 
      tag);
    /*for(size_t i = 0; i < payload_lenght; i++) {
        RTC_LOG(LS_VERBOSE) << "XXX payload" << i << " " << payload[i];
    }*/

  for (size_t i = 0; i < decryptedtext_len; i++) {
    frame[i + unencrypted_bytes] = decryptedtext[unencrypted_bytes];
  }

  return Result(Status::kOk, frame.size());
}

size_t GCMFrameDecryptor::GetMaxPlaintextByteSize(
    cricket::MediaType media_type,
    size_t encrypted_frame_size) {
 return encrypted_frame_size;
}

}