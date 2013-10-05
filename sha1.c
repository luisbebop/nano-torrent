/*!
 * @file sha-1.c
 * @author Paul E. Jones (paulej@packetizer.com)
 * @author Andre Caron (andre.l.caron@gmail.com)
 * @copyright Copyright (c) 1998, 2009
 *  Paul E. Jones (paulej@packetizer.com)
 *  All Rights Reserved
 */

#include "sha1.h"
#include <string.h>

#define _sha1_circular_shift(bits,word) \
    ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))

/*!
 * @internal
 * @brief Process the next 512 bits of the message.
 * @param digest SHA-1 digest accumulator.
 *
 * @note Many of the variable names in the SHADigest, especially the single
 *  character names, were used because those were the names used in the
 *  publication.
 */
static void _sha1_process_message_block ( sha1_digest * digest )
{
    const unsigned K[] =            /* Constants defined in SHA-1   */      
    {
        0x5A827999,
        0x6ED9EBA1,
        0x8F1BBCDC,
        0xCA62C1D6
    };
    int         t;                  /* Loop counter                 */
    unsigned    temp;               /* Temporary word value         */
    unsigned    W[80];              /* Word sequence                */
    unsigned    A, B, C, D, E;      /* Word buffers                 */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t] = ((unsigned) digest->Message_Block[t * 4]) << 24;
        W[t] |= ((unsigned) digest->Message_Block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) digest->Message_Block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) digest->Message_Block[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = _sha1_circular_shift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = digest->Message_Digest[0];
    B = digest->Message_Digest[1];
    C = digest->Message_Digest[2];
    D = digest->Message_Digest[3];
    E = digest->Message_Digest[4];

    for(t = 0; t < 20; t++)
    {
        temp =  _sha1_circular_shift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = _sha1_circular_shift(30,B);
        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = _sha1_circular_shift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = _sha1_circular_shift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = _sha1_circular_shift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = _sha1_circular_shift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = _sha1_circular_shift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = _sha1_circular_shift(30,B);
        B = A;
        A = temp;
    }

    digest->Message_Digest[0] =
                        (digest->Message_Digest[0] + A) & 0xFFFFFFFF;
    digest->Message_Digest[1] =
                        (digest->Message_Digest[1] + B) & 0xFFFFFFFF;
    digest->Message_Digest[2] =
                        (digest->Message_Digest[2] + C) & 0xFFFFFFFF;
    digest->Message_Digest[3] =
                        (digest->Message_Digest[3] + D) & 0xFFFFFFFF;
    digest->Message_Digest[4] =
                        (digest->Message_Digest[4] + E) & 0xFFFFFFFF;

    digest->Message_Block_Index = 0;
}

/*!
 * @internal
 * @brief Ensure the message is padded to an integer multiple of 512-bits.
 * @param digest SHA-1 digest accumulator.
 *
 * According to the standard, the message must be padded to an even 512 bits.
 * The first padding bit must be a '1'.  The last 64 bits represent the size of
 * the original message.  All bits in between should be 0.  This function will
 * pad the message according to those rules by filling @c Message_Block array
 * accordingly.  It will also call @c _sha1_process_message_block()
 * appropriately.  When it returns, it can be assumed that the message digest
 * has been computed.
 */
static void _sha1_pad_message ( sha1_digest * digest )
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and size.  If so, we will pad the
     *  block, process it, and then continue padding into a second
     *  block.
     */
    if (digest->Message_Block_Index > 55)
    {
        digest->Message_Block[digest->Message_Block_Index++] = 0x80;
        while(digest->Message_Block_Index < 64)
        {
            digest->Message_Block[digest->Message_Block_Index++] = 0;
        }

        _sha1_process_message_block(digest);

        while(digest->Message_Block_Index < 56)
        {
            digest->Message_Block[digest->Message_Block_Index++] = 0;
        }
    }
    else
    {
        digest->Message_Block[digest->Message_Block_Index++] = 0x80;
        while(digest->Message_Block_Index < 56)
        {
            digest->Message_Block[digest->Message_Block_Index++] = 0;
        }
    }

    /*
     *  Store the message size as the last 8 octets
     */
    digest->Message_Block[56] = (digest->Length_High >> 24) & 0xFF;
    digest->Message_Block[57] = (digest->Length_High >> 16) & 0xFF;
    digest->Message_Block[58] = (digest->Length_High >> 8) & 0xFF;
    digest->Message_Block[59] = (digest->Length_High) & 0xFF;
    digest->Message_Block[60] = (digest->Length_Low >> 24) & 0xFF;
    digest->Message_Block[61] = (digest->Length_Low >> 16) & 0xFF;
    digest->Message_Block[62] = (digest->Length_Low >> 8) & 0xFF;
    digest->Message_Block[63] = (digest->Length_Low) & 0xFF;

    _sha1_process_message_block(digest);
}

/*!
 * @internal
 * @brief Compute the 160-bit message digest.
 * @param digest SHA-1 digest accumulator.
 * @return 1 on success, 0 on failure.
 */
static int sha1_finish ( sha1_digest * digest )
{
    if (digest->Corrupted)
    {
        return 0;
    }

    if (!digest->Computed)
    {
        _sha1_pad_message(digest);
        digest->Computed = 1;
    }

    return 1;
}

void sha1_clear ( sha1_digest * digest )
{
    digest->Length_Low             = 0;
    digest->Length_High            = 0;
    digest->Message_Block_Index    = 0;

    digest->Message_Digest[0]      = 0x67452301;
    digest->Message_Digest[1]      = 0xEFCDAB89;
    digest->Message_Digest[2]      = 0x98BADCFE;
    digest->Message_Digest[3]      = 0x10325476;
    digest->Message_Digest[4]      = 0xC3D2E1F0;

    digest->Computed   = 0;
    digest->Corrupted  = 0;
}

int sha1_result ( sha1_digest * digest, unsigned char * hash )
{
    if (digest->Corrupted)
    {
        return 0;
    }
    if (!digest->Computed)
    {
        _sha1_pad_message(digest);
        digest->Computed = 1;
    }
    hash[ 0] = (digest->Message_Digest[0] & 0xff000000) >> 24;
    hash[ 1] = (digest->Message_Digest[0] & 0x00ff0000) >> 16;
    hash[ 2] = (digest->Message_Digest[0] & 0x0000ff00) >>  8;
    hash[ 3] = (digest->Message_Digest[0] & 0x000000ff) >>  0;
    hash[ 4] = (digest->Message_Digest[1] & 0xff000000) >> 24;
    hash[ 5] = (digest->Message_Digest[1] & 0x00ff0000) >> 16;
    hash[ 6] = (digest->Message_Digest[1] & 0x0000ff00) >>  8;
    hash[ 7] = (digest->Message_Digest[1] & 0x000000ff) >>  0;
    hash[ 8] = (digest->Message_Digest[2] & 0xff000000) >> 24;
    hash[ 9] = (digest->Message_Digest[2] & 0x00ff0000) >> 16;
    hash[10] = (digest->Message_Digest[2] & 0x0000ff00) >>  8;
    hash[11] = (digest->Message_Digest[2] & 0x000000ff) >>  0;
    hash[12] = (digest->Message_Digest[3] & 0xff000000) >> 24;
    hash[13] = (digest->Message_Digest[3] & 0x00ff0000) >> 16;
    hash[14] = (digest->Message_Digest[3] & 0x0000ff00) >>  8;
    hash[15] = (digest->Message_Digest[3] & 0x000000ff) >>  0;
    hash[16] = (digest->Message_Digest[4] & 0xff000000) >> 24;
    hash[17] = (digest->Message_Digest[4] & 0x00ff0000) >> 16;
    hash[18] = (digest->Message_Digest[4] & 0x0000ff00) >>  8;
    hash[19] = (digest->Message_Digest[4] & 0x000000ff) >>  0;
    return 1;
}

void sha1_update
    ( sha1_digest * digest, const unsigned char *data, size_t size )
{
    if (!size)
    {
        return;
    }

    if (digest->Computed || digest->Corrupted)
    {
        digest->Corrupted = 1;
        return;
    }

    while(size-- && !digest->Corrupted)
    {
        digest->Message_Block[digest->Message_Block_Index++] =
                                                (*data & 0xFF);

        digest->Length_Low += 8;
        /* Force it to 32 bits */
        digest->Length_Low &= 0xFFFFFFFF;
        if (digest->Length_Low == 0)
        {
            digest->Length_High++;
            /* Force it to 32 bits */
            digest->Length_High &= 0xFFFFFFFF;
            if (digest->Length_High == 0)
            {
                /* Message is too long */
                digest->Corrupted = 1;
            }
        }

        if (digest->Message_Block_Index == 64)
        {
            _sha1_process_message_block(digest);
        }

        data++;
    }
}

void sha1_compute
    ( const unsigned char * data, size_t size, unsigned char * hash )
{
    sha1_digest digest;
    sha1_clear(&digest);
    sha1_update(&digest, data, size);
    sha1_result(&digest, hash);
}

static char HEX[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

void sha1_hexstring
    ( const unsigned char * hash, char * display )
{
    size_t i, j;
    for (i=0, j=0; i < 20; i++)
    {
        display[j++] = HEX[(hash[i]&0xf0)>>4];
        display[j++] = HEX[(hash[i]&0x0f)>>0];
    }
}
