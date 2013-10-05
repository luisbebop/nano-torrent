#ifndef _sha1_h__
#define _sha1_h__

/*!
 * @file sha-1.h
 * @author Paul E. Jones (paulej@packetizer.com)
 * @author Andre Caron (andre.l.caron@gmail.com)
 * @copyright Copyright (c) 1998, 2009
 *  Paul E. Jones (paulej@packetizer.com)
 *  All Rights Reserved
 *
 * This library implements the Secure Hashing Standard as defined in FIPS PUB
 * 180-1 published April 17, 1995.  The Secure Hashing Standard, which uses the
 * Secure Hashing Algorithm (SHA), produces a 160-bit message digest for a given
 * data stream.  In theory, it is highly improbable that two messages will
 * produce the same message digest.  Therefore, this algorithm can serve as a
 * means of providing a "fingerprint" for a message.
 *
 * @par Sample usage
 * @code
 *   int stream;
 *   sha1_digest digest;
 *   unsigned char data[1024];
 *   ssize_t size; 
 *
 *   stream = open(...);
 *   if (stream < 0) {
 *     // error opening file.
 *   }
 *   sha1_clear(&digest);
 *   while ((size=read(stream,data,sizeof(data)) > 0) {
 *     sha1_feed(&digest, data, size);
 *   }
 *   if (size < 0) {
 *     // error reading file.
 *   }
 *   if (!sha1_result(&digest, data)) {
 *     // error computing digest...
 *   }
 *   // first 5 bytes of 'data' contains SHA-1 hash.
 * @endcode
 *
 * @par Portability issues
 *  SHA-1 is defined in terms of 32-bit "words".  This code was
 *  written with the expectation that the processor has at least
 *  a 32-bit machine word size.  If the machine word size is larger,
 *  the code should still function properly.  One caveat to that
 *  is that the input functions taking characters and character
 *  arrays assume that only 8 bits of information are stored in each
 *  character.
 *
 * @par Caveats
 *  SHA-1 is designed to work with messages less than 2^64 bits
 *  long. Although SHA-1 allows a message digest to be generated for
 *  messages of any number of bits less than 2^64, this
 *  implementation only works with messages with a size that is a
 *  multiple of the size of an 8-bit character.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief SHA-1 digest accumulator.
 */
typedef struct sha1_digest
{
    unsigned Message_Digest[5]; /* Message Digest (output)          */

    unsigned Length_Low;        /* Message length in bits           */
    unsigned Length_High;       /* Message length in bits           */

    unsigned char Message_Block[64]; /* 512-bit message blocks      */
    int Message_Block_Index;    /* Index into message block array   */

    int Computed;               /* Is the digest computed?          */
    int Corrupted;              /* Is the message digest corruped?  */

} sha1_digest;

/*!
 * @brief Initialize the digest accumulator.
 * @param diges tSHA-1 digest accumulator.
 */
void sha1_clear ( sha1_digest * digest );

/*!
 * @brief Finish computing the SHA-1 and return the 160-bit digest.
 * @param digest SHA-1 digest accumulator.
 * @param data An array of 20 bytes that will receive the digest.
 * @return 1 on success, 0 on failure.
 *
 * @note If the function returns 0, contents of @a data are unspecified.
 */
int sha1_result ( sha1_digest * digest, unsigned char * hash );

/*!
 * @brief Accept an array of octets as the next portion of the message.
 * @param digest SHA-1 digest accumulator.
 * @param data Array of characters containing the message contents.
 * @param size Number of bytes in @a data, size of message.
 */
void sha1_update
    ( sha1_digest * digest , const unsigned char * data, size_t size );

/*!
 * @brief High-level SHA-1 digest computation.
 * @param data Array of characters containing the message contents.
 * @param size Number of bytes in @a data, size of message.
 * @param hash An array of 20 bytes that will receive the digest.
 *
 * This is a simplified SHA-1 computation function for the case where you have
 * already have the entire message in memory.  A single call will produce the
 * SHA-1 160-bit digest of the entire message.
 */
void sha1_compute
    ( const unsigned char * data, size_t size, unsigned char * hash );

void sha1_hexstring
    ( const unsigned char * hash, char * display );

#ifdef __cplusplus
}
#endif

#endif /* _sha1_h__ */