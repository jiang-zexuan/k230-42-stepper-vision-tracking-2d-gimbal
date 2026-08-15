#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/vision_frame_parser.h"

static uint16_t build_frame(uint8_t *output, uint8_t sequence,
                            const uint8_t *payload, uint8_t payload_size)
{
    uint16_t crc;
    uint16_t index;

    output[0] = 0xAAU;
    output[1] = 0x55U;
    output[2] = VISION_FRAME_VERSION;
    output[3] = payload_size;
    output[4] = sequence;
    for (index = 0U; index < payload_size; index++) {
        output[5U + index] = payload[index];
    }

    crc = VisionFrame_Crc16Modbus(&output[2], (uint16_t)(3U + payload_size));
    output[5U + payload_size] = (uint8_t)(crc & 0xFFU);
    output[6U + payload_size] = (uint8_t)(crc >> 8U);
    return (uint16_t)(7U + payload_size);
}

static bool feed_bytes(VisionFrameParser *parser, const uint8_t *bytes,
                       uint16_t size, VisionFrame *completed)
{
    bool accepted = false;
    uint16_t index;

    for (index = 0U; index < size; index++) {
        if (VisionFrameParser_Feed(parser, bytes[index], completed)) {
            accepted = true;
        }
    }

    return accepted;
}

static void test_crc16_modbus_matches_standard_vector(void)
{
    const uint8_t input[] = "123456789";

    assert(VisionFrame_Crc16Modbus(input, sizeof(input) - 1U) == 0x4B37U);
}

static void test_parser_accepts_a_frame_split_across_two_dma_events(void)
{
    VisionFrameParser parser;
    VisionFrame completed;
    const uint8_t payload[] = {0x10U, 0x20U};
    uint8_t bytes[16];
    uint16_t size = build_frame(bytes, 7U, payload, sizeof(payload));

    VisionFrameParser_Init(&parser);

    assert(feed_bytes(&parser, bytes, 4U, &completed) == false);
    assert(VisionFrameParser_IsReceiving(&parser) == true);
    assert(feed_bytes(&parser, &bytes[4], (uint16_t)(size - 4U), &completed) == true);
    assert(VisionFrameParser_IsReceiving(&parser) == false);
    assert(completed.sequence == 7U);
    assert(completed.payload_size == sizeof(payload));
    assert(completed.payload[0] == 0x10U);
    assert(completed.payload[1] == 0x20U);
}

static void test_parser_accepts_two_frames_in_one_dma_event(void)
{
    VisionFrameParser parser;
    VisionFrame completed;
    const uint8_t first_payload[] = {0x01U};
    const uint8_t second_payload[] = {0x02U};
    uint8_t bytes[32];
    uint16_t first_size = build_frame(bytes, 1U, first_payload, sizeof(first_payload));
    uint16_t second_size = build_frame(&bytes[first_size], 2U, second_payload,
                                       sizeof(second_payload));

    VisionFrameParser_Init(&parser);

    assert(feed_bytes(&parser, bytes, (uint16_t)(first_size + second_size),
                      &completed) == true);
    assert(parser.valid_count == 2U);
    assert(completed.sequence == 2U);
    assert(completed.payload[0] == 0x02U);
}

static void test_parser_resynchronizes_after_garbage_bytes(void)
{
    VisionFrameParser parser;
    VisionFrame completed;
    const uint8_t payload[] = {0x7FU};
    uint8_t bytes[16];
    uint16_t size = build_frame(bytes, 3U, payload, sizeof(payload));
    const uint8_t garbage[] = {0x00U, 0xFFU, 0x12U};

    VisionFrameParser_Init(&parser);

    assert(feed_bytes(&parser, garbage, sizeof(garbage), &completed) == false);
    assert(feed_bytes(&parser, bytes, size, &completed) == true);
    assert(completed.sequence == 3U);
}

static void test_parser_rejects_bad_crc_then_accepts_the_next_valid_frame(void)
{
    VisionFrameParser parser;
    VisionFrame completed;
    const uint8_t payload[] = {0x33U};
    uint8_t bytes[16];
    uint16_t size = build_frame(bytes, 4U, payload, sizeof(payload));

    VisionFrameParser_Init(&parser);

    bytes[size - 1U] ^= 0x01U;
    assert(feed_bytes(&parser, bytes, size, &completed) == false);
    assert(parser.error_count == 1U);

    size = build_frame(bytes, 5U, payload, sizeof(payload));
    assert(feed_bytes(&parser, bytes, size, &completed) == true);
    assert(completed.sequence == 5U);
}

int main(void)
{
    test_crc16_modbus_matches_standard_vector();
    test_parser_accepts_a_frame_split_across_two_dma_events();
    test_parser_accepts_two_frames_in_one_dma_event();
    test_parser_resynchronizes_after_garbage_bytes();
    test_parser_rejects_bad_crc_then_accepts_the_next_valid_frame();
    puts("vision_frame_parser_test: PASS");
    return 0;
}
