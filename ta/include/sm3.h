#ifndef CUBE_DATA_TYPE_H
#define CUBE_DATA_TYPE_H

#ifndef TSS_PLATFORM_H
typedef unsigned char BYTE;
typedef unsigned short int UINT16;
typedef unsigned int UINT32;
typedef unsigned long int UINT64;
typedef unsigned short int WORD;
typedef unsigned int DWORD;
#endif

#define BITSTRING (unsigned char *)
#define CUBE_DEBUG

#define DIGEST_SIZE 32
// #define PAGE_SIZE	4096
#ifndef NULL
#define NULL 0
#endif

#define IS_ERR(ptr) (ptr - 4096 < 0)
#ifdef CUBE_DEBUG
#define cube_dbg(format, arg...) \
  printk(KERN_DEBUG, format, ##arg)
#else
#define cube_dbg(format, arg...) \
  do                             \
  {                              \
  } while (0)
#endif
#endif

#ifndef CRYPTO_FUNC_H
#define CRYPTO_FUNC_H
#define DIGEST_SIZE 32

typedef struct
{
  UINT32 total_bytes_High;
  UINT32 total_bytes_Low;
  UINT32 vector[8];
  BYTE buffer[64]; /* 64 byte buffer                            */

  BYTE ipad[64]; // HMAC: inner padding
  BYTE opad[64]; // HMAC: outer padding
} sm3_context;

int SM3_init(sm3_context *index);
int SM3_update(sm3_context *index, BYTE *chunk_data, UINT32 chunk_length);
int SM3_final(sm3_context *index, UINT32 *SM3_hash);
void SM3_hmac(BYTE *key, int keylen, BYTE *input, int ilen, UINT32 *output);

#endif

#ifndef _SM3_
#define _SM3_

// #define DEBUG

typedef struct
{
  UINT32 total_bytes_High;
  UINT32 total_bytes_Low;
  UINT32 vector[8];
  BYTE buffer[64]; // 64 byte buffer

  BYTE ipad[64]; // HMAC: inner padding
  BYTE opad[64]; // HMAC: outer padding

} SM3_context;

#define rol(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))
/*
inline int rol(UINT32 operand, BYTE width){
   asm volatile("rol %%cl, %%eax"
               : "=a" (operand)
               : "a" (operand), "c" (width)
               );
}
*/
#define P0(x) ((x ^ (rol(x, 9)) ^ (rol(x, 17))))
#define P1(x) ((x ^ (rol(x, 15)) ^ (rol(x, 23))))

#define CONCAT_4_BYTES(w32, w8, w8_i)          \
  {                                            \
    (w32) = ((UINT32)(w8)[(w8_i)] << 24) |     \
            ((UINT32)(w8)[(w8_i) + 1] << 16) | \
            ((UINT32)(w8)[(w8_i) + 2] << 8) |  \
            ((UINT32)(w8)[(w8_i) + 3]);        \
  }

#define SPLIT_INTO_4_BYTES(w32, w8, w8_i)   \
  {                                         \
    (w8)[(w8_i)] = (BYTE)((w32) >> 24);     \
    (w8)[(w8_i) + 1] = (BYTE)((w32) >> 16); \
    (w8)[(w8_i) + 2] = (BYTE)((w32) >> 8);  \
    (w8)[(w8_i) + 3] = (BYTE)((w32));       \
  }

static BYTE SM3_padding[64] =
    {
        (BYTE)0x80, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0,
        (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0,
        (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0,
        (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0, (BYTE)0};

#endif

int SM3_init(sm3_context *index)
{
  if (index == NULL)
  {
    return -1;
  }

  index->total_bytes_High = 0;
  index->total_bytes_Low = 0;
  index->vector[0] = 0x7380166f;
  index->vector[1] = 0x4914b2b9;
  index->vector[2] = 0x172442d7;
  index->vector[3] = 0xda8a0600;
  index->vector[4] = 0xa96f30bc;
  index->vector[5] = 0x163138aa;
  index->vector[6] = 0xe38dee4d;
  index->vector[7] = 0xb0fb0e4e;

  return 0;
}

static void SM3_CF(sm3_context *index, BYTE *byte_64_block)
{
  UINT32 j, temp, W[68];

  UINT32 A, B, C, D, E, F, G, H, SS1, SS2, TT1, TT2;
  CONCAT_4_BYTES(W[0], byte_64_block, 0);
  CONCAT_4_BYTES(W[1], byte_64_block, 4);
  CONCAT_4_BYTES(W[2], byte_64_block, 8);
  CONCAT_4_BYTES(W[3], byte_64_block, 12);
  CONCAT_4_BYTES(W[4], byte_64_block, 16);
  CONCAT_4_BYTES(W[5], byte_64_block, 20);
  CONCAT_4_BYTES(W[6], byte_64_block, 24);
  CONCAT_4_BYTES(W[7], byte_64_block, 28);
  CONCAT_4_BYTES(W[8], byte_64_block, 32);
  CONCAT_4_BYTES(W[9], byte_64_block, 36);
  CONCAT_4_BYTES(W[10], byte_64_block, 40);
  CONCAT_4_BYTES(W[11], byte_64_block, 44);
  CONCAT_4_BYTES(W[12], byte_64_block, 48);
  CONCAT_4_BYTES(W[13], byte_64_block, 52);
  CONCAT_4_BYTES(W[14], byte_64_block, 56);
  CONCAT_4_BYTES(W[15], byte_64_block, 60);

  for (j = 16; j < 68; j++)
  {
    // waitting to modified
    // there is something strange here,"P1(W[j-16]^W[j-9]^rol(W[j-3],15))" will get a error result
    temp = W[j - 16] ^ W[j - 9] ^ rol(W[j - 3], 15);
    W[j] = P1(temp) ^ rol(W[j - 13], 7) ^ (W[j - 6]);
    //	W[j] = P1((W[j-16]^W[j-9]^rol(W[j-3],15)))^rol(W[j-13],7)^(W[j-6]);
  }
  A = index->vector[0];
  B = index->vector[1];
  C = index->vector[2];
  D = index->vector[3];
  E = index->vector[4];
  F = index->vector[5];
  G = index->vector[6];
  H = index->vector[7];

#define T 0x79cc4519
#define FF(X, Y, Z) (X ^ Y ^ Z)
#define GG(X, Y, Z) (X ^ Y ^ Z)

  for (j = 0; j < 16; j++)
  {
    SS1 = rol(rol(A, 12) + E + rol(T, j), 7);
    SS2 = SS1 ^ (rol(A, 12));
    TT1 = FF(A, B, C) + D + SS2 + (W[j] ^ W[j + 4]);
    TT2 = GG(E, F, G) + H + SS1 + W[j];
    D = C;
    C = rol(B, 9);
    B = A;
    A = TT1;
    H = G;
    G = rol(F, 19);
    F = E;
    E = P0(TT2);
  }

#undef T
#undef FF
#undef GG

#define T 0x7a879d8a
#define FF(X, Y, Z) ((X & Y) | (X & Z) | (Y & Z))
#define GG(X, Y, Z) ((X & Y) | (~X & Z))

  for (j = 16; j < 64; j++)
  {
    SS1 = rol(rol(A, 12) + E + rol(T, j), 7);
    SS2 = SS1 ^ (rol(A, 12));
    TT1 = FF(A, B, C) + D + SS2 + (W[j] ^ W[j + 4]);
    TT2 = GG(E, F, G) + H + SS1 + W[j];
    D = C;
    C = rol(B, 9);
    B = A;
    A = TT1;
    H = G;
    G = rol(F, 19);
    F = E;
    E = P0(TT2);
  }
#undef T
#undef FF
#undef GG

  index->vector[0] ^= A;
  index->vector[1] ^= B;
  index->vector[2] ^= C;
  index->vector[3] ^= D;
  index->vector[4] ^= E;
  index->vector[5] ^= F;
  index->vector[6] ^= G;
  index->vector[7] ^= H;
}

int SM3_update(sm3_context *index, BYTE *chunk_data, UINT32 chunk_length)
{

  UINT32 left, fill;
  UINT32 i;

  if ((index == NULL) || (chunk_data == NULL) || (chunk_length < 1))
  {
    return -1;
  }

  left = index->total_bytes_Low & 0x3F;
  fill = 64 - left;
  index->total_bytes_Low += chunk_length;
  index->total_bytes_Low &= 0xFFFFFFFF;

  if (index->total_bytes_Low < chunk_length)
  {
    index->total_bytes_High++;
  }

  if ((left > 0) && (chunk_length >= fill))
  {
    for (i = 0; i < fill; i++)
    {
      index->buffer[left + i] = chunk_data[i];
    }
    SM3_CF(index, index->buffer);
    chunk_length -= fill;
    chunk_data += fill;
    left = 0;
  }

  while (chunk_length >= 64)
  {
    SM3_CF(index, chunk_data);
    chunk_length -= 64;
    chunk_data += 64;
  }

  if (chunk_length > 0)
  {
    for (i = 0; i < chunk_length; i++)
    {
      index->buffer[left + i] = chunk_data[i];
    }
  }
  return 0;
}

int SM3_final(sm3_context *index, UINT32 *SM3_hash)
{

  UINT32 last, padn;
  UINT32 high, low;
  BYTE msglen[8];
  int ret;
  if ((index == NULL) || (SM3_hash == NULL))
  {
    *SM3_hash = 0;
    return -1;
  }
  high = (index->total_bytes_Low >> 29) | (index->total_bytes_High << 3);
  low = (index->total_bytes_Low << 3);
  SPLIT_INTO_4_BYTES(high, msglen, 0);
  SPLIT_INTO_4_BYTES(low, msglen, 4);

  last = index->total_bytes_Low & 0x3F;
  padn = (last < 56) ? (56 - last) : (120 - last);
  ret = SM3_update(index, SM3_padding, padn);
  ret = SM3_update(index, msglen, 8);
  SM3_hash[0] = index->vector[0];
  SM3_hash[1] = index->vector[1];
  SM3_hash[2] = index->vector[2];
  SM3_hash[3] = index->vector[3];
  SM3_hash[4] = index->vector[4];
  SM3_hash[5] = index->vector[5];
  SM3_hash[6] = index->vector[6];
  SM3_hash[7] = index->vector[7];
  // SM3_hash[8] = index->vector[8];
  return 0;
}

int calculate_sm3(unsigned char *data, unsigned int data_size, UINT32 *SM3_hash)
{
  int result;
  int bytes_to_copy = 0;
  SM3_context index;

  result = SM3_init(&index);
  if (result)
  {
    return -1;
  }

  bytes_to_copy = data_size;
  while (bytes_to_copy > 4096)
  {
    result = SM3_update(&index, data, 4096);
    if (result)
    {
      return -1;
    }
    data += 4096;
    bytes_to_copy = bytes_to_copy - 4096;
  }

  // And the last one
  result = SM3_update(&index, data, bytes_to_copy);
  if (result)
  {
    return -1;
  }

  result = SM3_final(&index, SM3_hash);
  if (result)
    return -1;
  return 0;
}

int calculate_pathsm3(char *filepath, UINT32 *SM3_hash)
{

  int result;
  SM3_context index;
  result = SM3_init(&index);
  if (result)
  {
    return -1;
  }
  result = SM3_update(&index, filepath, strlen(filepath));
  if (result)
  {
    return -1;
  }
  result = SM3_final(&index, SM3_hash);
  if (result)
    return -1;
  return 0;
}