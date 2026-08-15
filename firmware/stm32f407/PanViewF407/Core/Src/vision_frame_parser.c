#include "vision_frame_parser.h"

#include <stddef.h>
#include <string.h>

typedef enum {
    VISION_PARSER_WAIT_HEADER_FIRST,
    VISION_PARSER_WAIT_HEADER_SECOND,
    VISION_PARSER_READ_VERSION,
    VISION_PARSER_READ_LENGTH,
    VISION_PARSER_READ_SEQUENCE,
    VISION_PARSER_READ_PAYLOAD,
    VISION_PARSER_READ_CRC_LOW,
    VISION_PARSER_READ_CRC_HIGH
} VisionParserState;

static uint16_t VisionFrame_Crc16ModbusUpdate(uint16_t crc, uint8_t byte)
{
    uint8_t bit_index;

    crc ^= byte;
    for (bit_index = 0U; bit_index < 8U; bit_index++)
    {
        if ((crc & 1U) != 0U)
        {
            crc = (crc >> 1U) ^ 0xA001U;
        }
        else
        {
            crc >>= 1U;
        }
    }

    return crc;
}

uint16_t VisionFrame_Crc16Modbus(const uint8_t *data, uint16_t size)
{
    uint16_t crc = 0xFFFFU;
    uint16_t index;

    if (data == NULL)
    {
        return crc;
    }

    for (index = 0U; index < size; index++)
    {
        crc = VisionFrame_Crc16ModbusUpdate(crc, data[index]);
    }

    return crc;
}

static void VisionFrameParser_Reset(VisionFrameParser *parser)
{
    parser->state = VISION_PARSER_WAIT_HEADER_FIRST;
    parser->payload_size = 0U;
    parser->payload_index = 0U;
    parser->calculated_crc = 0xFFFFU;
    parser->received_crc = 0U;
}

void VisionFrameParser_Init(VisionFrameParser *parser)
{
    memset(parser, 0, sizeof(*parser));
    VisionFrameParser_Reset(parser);
}

static void VisionFrameParser_Resynchronize(VisionFrameParser *parser,
                                             uint8_t byte)
{
    VisionFrameParser_Reset(parser);
    if (byte == VISION_FRAME_HEADER_FIRST)
    {
        parser->state = VISION_PARSER_WAIT_HEADER_SECOND;
    }
}

bool VisionFrameParser_Feed(VisionFrameParser *parser, uint8_t byte,
                            VisionFrame *completed)
{
    switch ((VisionParserState)parser->state)
    {
        case VISION_PARSER_WAIT_HEADER_FIRST:
            if (byte == VISION_FRAME_HEADER_FIRST)
            {
                parser->state = VISION_PARSER_WAIT_HEADER_SECOND;
            }
            break;

        case VISION_PARSER_WAIT_HEADER_SECOND:
            if (byte == VISION_FRAME_HEADER_SECOND)
            {
                parser->state = VISION_PARSER_READ_VERSION;
            }
            else if (byte != VISION_FRAME_HEADER_FIRST)
            {
                parser->state = VISION_PARSER_WAIT_HEADER_FIRST;
            }
            break;

        case VISION_PARSER_READ_VERSION:
            if (byte != VISION_FRAME_VERSION)
            {
                parser->error_count++;
                VisionFrameParser_Resynchronize(parser, byte);
                break;
            }

            parser->calculated_crc = VisionFrame_Crc16ModbusUpdate(0xFFFFU, byte);
            parser->state = VISION_PARSER_READ_LENGTH;
            break;

        case VISION_PARSER_READ_LENGTH:
            if (byte > VISION_FRAME_MAX_PAYLOAD_SIZE)
            {
                parser->error_count++;
                VisionFrameParser_Resynchronize(parser, byte);
                break;
            }

            parser->payload_size = byte;
            parser->payload_index = 0U;
            parser->calculated_crc = VisionFrame_Crc16ModbusUpdate(parser->calculated_crc,
                                                                     byte);
            parser->state = VISION_PARSER_READ_SEQUENCE;
            break;

        case VISION_PARSER_READ_SEQUENCE:
            parser->sequence = byte;
            parser->calculated_crc = VisionFrame_Crc16ModbusUpdate(parser->calculated_crc,
                                                                     byte);
            parser->state = (parser->payload_size == 0U)
                                ? VISION_PARSER_READ_CRC_LOW
                                : VISION_PARSER_READ_PAYLOAD;
            break;

        case VISION_PARSER_READ_PAYLOAD:
            parser->payload[parser->payload_index++] = byte;
            parser->calculated_crc = VisionFrame_Crc16ModbusUpdate(parser->calculated_crc,
                                                                     byte);
            if (parser->payload_index >= parser->payload_size)
            {
                parser->state = VISION_PARSER_READ_CRC_LOW;
            }
            break;

        case VISION_PARSER_READ_CRC_LOW:
            parser->received_crc = byte;
            parser->state = VISION_PARSER_READ_CRC_HIGH;
            break;

        case VISION_PARSER_READ_CRC_HIGH:
            parser->received_crc |= (uint16_t)byte << 8U;
            if (parser->received_crc == parser->calculated_crc)
            {
                completed->sequence = parser->sequence;
                completed->payload_size = parser->payload_size;
                memcpy(completed->payload, parser->payload, parser->payload_size);
                parser->valid_count++;
                VisionFrameParser_Reset(parser);
                return true;
            }

            parser->error_count++;
            VisionFrameParser_Resynchronize(parser, byte);
            break;

        default:
            VisionFrameParser_Reset(parser);
            break;
    }

    return false;
}

bool VisionFrameParser_IsReceiving(const VisionFrameParser *parser)
{
    return parser->state != VISION_PARSER_WAIT_HEADER_FIRST;
}
