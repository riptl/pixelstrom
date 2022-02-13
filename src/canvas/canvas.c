#include <solana_sdk.h>

#include "bin.h"

#define INS_SET_PIXEL 1

#define CHUNK_DIMENSION 32
#define CHUNK_PIXELS (CHUNK_DIMENSION * CHUNK_DIMENSION)
#define PIXEL_SIZE 3
#define ROW_SIZE (CHUNK_DIMENSION * PIXEL_SIZE)
#define CHUNK_SIZE (CHUNK_PIXELS * PIXEL_SIZE)

static const uint8_t PDA_ID_CHUNK[] = "Chunk";

// clang-format off
static const SolPubkey SYSVAR_INSTRUCTIONS = {
    .x = {
        0x06, 0xa7, 0xd5, 0x17, 0x18, 0x7b, 0xd1, 0x66,
        0x35, 0xda, 0xd4, 0x04, 0x55, 0xfd, 0xc2, 0xc0,
        0xc1, 0x24, 0xc6, 0x8f, 0x21, 0x56, 0x75, 0xa5,
        0xdb, 0xba, 0xcb, 0x5f, 0x08, 0x00, 0x00, 0x00,
    }
};
// clang-format on

struct point {
    int32_t chunk_x;
    int32_t chunk_y;
    uint8_t offset_x;
    uint8_t offset_y;
};

// coords_to_chunk maps pixel coordinates to a chunk.
inline static struct point coords_to_chunk(int32_t x, int32_t y)
{
    struct point point = {
        .chunk_x = x / CHUNK_DIMENSION,
        .chunk_y = y / CHUNK_DIMENSION,
        .offset_x = x % CHUNK_DIMENSION,
        .offset_y = y % CHUNK_DIMENSION,
    };
    return point;
}

inline static bool bounds_check_offset(int32_t v)
{
    return v >= 0 && v < CHUNK_DIMENSION;
}

inline static int32_t pixel_data_offset(int32_t offset_x, int32_t offset_y)
{
    return (offset_y * ROW_SIZE) + (offset_x * PIXEL_SIZE);
}

inline static void set_pixel(uint8_t* data, int32_t offset_x, int32_t offset_y, uint8_t r, uint8_t g, uint8_t b)
{
    sol_assert(bounds_check_offset(offset_x) && bounds_check_offset(offset_y));
    uint8_t* ptr = &data[pixel_data_offset(offset_x, offset_y)];
    ptr[0] = r;
    ptr[1] = g;
    ptr[2] = b;
}

// derive_chunk_address returns the account address of a chunk.
inline static SolPubkey derive_chunk_address(const SolPubkey* program_id, int32_t chunk_x, int32_t chunk_y)
{
    // Build seed list.
    uint8_t be_chunk_y[4];
    encode_i32(be_chunk_y, chunk_y);
    uint8_t be_chunk_x[4];
    encode_i32(be_chunk_x, chunk_x);
    SolSignerSeed seeds[] = {
        // seeds[0]: "Chunk"
        {
            .addr = PDA_ID_CHUNK,
            .len = sizeof(PDA_ID_CHUNK),
        },
        // seeds[1]: big-endian `y: i32`
        {
            .addr = (const uint8_t*)(&be_chunk_y),
            .len = sizeof(be_chunk_y),
        },
        // seeds[2]: big-endian `x: i32`
        {
            .addr = (const uint8_t*)(&be_chunk_x),
            .len = sizeof(be_chunk_x),
        }
    };

    // Derive address.
    SolPubkey address;
    uint8_t bump_seed;
    uint64_t result = sol_try_find_program_address(
        seeds,
        SOL_ARRAY_SIZE(seeds),
        program_id,
        &address,
        &bump_seed);
    sol_assert(result == SUCCESS);

    return address;
}

static uint64_t instruction_set_pixel(const SolPubkey* program_id, SolAccountInfo chunk, SolAccountInfo insns, const uint8_t* data, uint64_t data_len)
{
    // Pre-flight checks on chunk account.
    sol_assert(SolPubkey_same(chunk.owner, program_id));
    sol_assert(chunk.is_writable);
    sol_assert(chunk.data_len == CHUNK_SIZE);

    // Pre-flight checks on instruction account.
    sol_assert(SolPubkey_same(insns.key, &SYSVAR_INSTRUCTIONS));

    // TODO Verify there's only one instruction.

    // Decode data.
    sol_assert(data_len == (4 + 4 + 3));
    int32_t x = decode_i32(&data[0]);
    int32_t y = decode_i32(&data[4]);
    uint8_t r = data[8];
    uint8_t g = data[9];
    uint8_t b = data[10];

    // Locate chunk.
    struct point point = coords_to_chunk(x, y);

    // Check chunk program address.
    SolPubkey chunk_address = derive_chunk_address(program_id, point.chunk_x, point.chunk_y);
    sol_assert(SolPubkey_same(chunk.key, &chunk_address));

    // Set pixel.
    set_pixel(chunk.data, point.offset_x, point.offset_y, r, g, b);

    return 1;
}

extern uint64_t entrypoint(const uint8_t* input)
{
    SolAccountInfo accounts[2];
    SolParameters params = (SolParameters) { .ka = accounts };

    if (!sol_deserialize(input, &params, SOL_ARRAY_SIZE(accounts))) {
        return ERROR_INVALID_ARGUMENT;
    }

    sol_assert(params.data_len >= 1);
    switch (params.data[0]) {
    case INS_SET_PIXEL:
        sol_assert(params.ka_num == 2);
        return instruction_set_pixel(params.program_id, accounts[0], accounts[1], params.data, params.data_len);
    default:
        sol_panic();
    }

    return 0;
}
