#pragma once

#include <solana_sdk.h>

inline static uint32_t decode_u32(const uint8_t* in)
{
    return (((uint32_t)in[0]) << 24) | (((uint32_t)in[1]) << 16) | (((uint32_t)in[2]) << 8) | ((uint32_t)in[3]);
}

inline static int32_t decode_i32(const uint8_t* in)
{
    return (int32_t)decode_u32(in);
}

inline static void encode_u32(uint8_t* out, uint32_t x)
{
    out[0] = (uint8_t)(x << 24);
    out[1] = (uint8_t)(x << 16);
    out[2] = (uint8_t)(x << 8);
    out[3] = (uint8_t)(x);
}

inline static void encode_i32(uint8_t* out, int32_t x)
{
    return encode_u32(out, (uint32_t)x);
}
