#include "CryptoBox.h"
#include "lwt/logging.h"

#include "KeyPair.h"
#include "Message.h"
#include "Nonce.h"

#include <algorithm>

#include <sodium.h>


using namespace tox;


template<typename ForwardIterator>
static bool
is_all_zero (ForwardIterator begin, ForwardIterator end)
{
  return std::find_if (begin, end, [](byte zero) { return zero != 0; }) == end;
}

CryptoBox::CryptoBox (KeyPair const &pair)
  : CryptoBox (pair.public_key, pair.secret_key)
{
}

CryptoBox::CryptoBox (PublicKey const &public_key, SecretKey const &secret_key)
{
  LOG (INFO) << "Deriving shared key from public key " << public_key << " and secret key " << secret_key;
  crypto_box_beforenm (shared_key_.data (), public_key.data (), secret_key.data ());
  LOG (INFO) << "Shared key: " << shared_key_;
}


CipherText
CryptoBox::encrypt (PlainText const &plain, Nonce const &n) const
{
  LOG (INFO) << "Encrypting " << plain.size () << " bytes with nonce " << n;
#if 0
  output_hex (LOG (INFO) << "Plain text: ", plain.data (), plain.size ());
#endif

  byte_vector padded_plain (plain.size () + crypto_box_ZEROBYTES);
  std::copy (plain.begin (), plain.end (), padded_plain.begin () + crypto_box_ZEROBYTES);

  // The caller must ensure, before calling the C NaCl crypto_box function,
  // that the first crypto_box_ZEROBYTES bytes of the message m are all 0.
  LOG_ASSERT (is_all_zero (padded_plain.cbegin (),
                           padded_plain.cbegin () + crypto_box_ZEROBYTES));

  // mlen counts all of the bytes, including the bytes required to be 0.
  size_t const mlen = plain.size () + crypto_box_ZEROBYTES;

  // The crypto_box function encrypts and authenticates a message.
  CipherText padded_crypto (plain.size () + crypto_box_BOXZEROBYTES + crypto_box_MACBYTES);
  int result = crypto_box_afternm (padded_crypto.data (), padded_plain.data (), mlen,
                                   n.data (), shared_key_.data ());
  // It then returns 0.
  LOG_ASSERT (result == 0);

  // The crypto_box function ensures that the first crypto_box_BOXZEROBYTES
  // bytes of the ciphertext c are all 0.
  LOG_ASSERT (is_all_zero (padded_crypto.cbegin (),
                           padded_crypto.cbegin () + crypto_box_BOXZEROBYTES));

  LOG (INFO) << "Cipher text is " << padded_crypto.size () - crypto_box_BOXZEROBYTES << " bytes";

  return CipherText (padded_crypto.cbegin () + crypto_box_BOXZEROBYTES,
                     padded_crypto.cend ());
}


Partial<PlainText>
CryptoBox::decrypt (BitStream<CipherText> const &crypto, Nonce const &n) const
{
  LOG (INFO) << "Decrypting " << crypto.size () << " bytes with nonce " << n;

  byte_vector padded_crypto (crypto.size () + crypto_box_BOXZEROBYTES);
  std::copy (crypto.cbegin (), crypto.cend (), padded_crypto.begin () + crypto_box_BOXZEROBYTES);

  // The caller must ensure, before calling the crypto_box_open function,
  // that the first crypto_box_BOXZEROBYTES bytes of the ciphertext c are
  // all 0.
  LOG_ASSERT (is_all_zero (padded_crypto.cbegin (),
                           padded_crypto.cbegin () + crypto_box_BOXZEROBYTES));

  // mlen counts all of the bytes, including the bytes required to be 0.
  size_t const mlen = crypto.size () + crypto_box_BOXZEROBYTES;

  PlainText padded_plain (crypto.size () + crypto_box_ZEROBYTES);
  int result = crypto_box_open_afternm (padded_plain.data (), padded_crypto.data (), mlen,
                                        n.data (), shared_key_.data ());
  if (result != 0)
    return failure (Status::HMACError);

  // The crypto_box_open function ensures (in case of success) that the first
  // crypto_box_ZEROBYTES bytes of the plaintext m are all 0.
  LOG_ASSERT (is_all_zero (padded_plain.cbegin (),
                           padded_plain.cbegin () + crypto_box_ZEROBYTES));

#if 0
  output_hex (LOG (INFO) << "Plain text: ",
              padded_plain.data () + crypto_box_ZEROBYTES,
              padded_plain.size () - crypto_box_ZEROBYTES - crypto_box_MACBYTES);
#endif

  LOG (INFO) << "Plain text is " << padded_plain.size () - crypto_box_ZEROBYTES - crypto_box_MACBYTES << " bytes";

  return success (PlainText (padded_plain.cbegin () + crypto_box_ZEROBYTES,
                             padded_plain.cend () - crypto_box_MACBYTES));
}


Partial<PlainText>
CryptoBox::decrypt (CipherText const &crypto, Nonce const &n) const
{
  return decrypt (BitStream<CipherText> (crypto), n);
}
