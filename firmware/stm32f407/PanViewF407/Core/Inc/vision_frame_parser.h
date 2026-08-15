#ifndef VISION_FRAME_PARSER_H
#define VISION_FRAME_PARSER_H

#include <stdbool.h>
#include <stdint.h>

enum {
    /* 协议设计值：P03 v1 的固定帧头与版本。 */
    VISION_FRAME_HEADER_FIRST = 0xAAU,
    VISION_FRAME_HEADER_SECOND = 0x55U,
    VISION_FRAME_VERSION = 1U,
    /* 单位：byte；受当前 64 byte DMA 接收缓冲区和 P03 v1 设计限制。 */
    VISION_FRAME_MAX_PAYLOAD_SIZE = 48U
};

typedef struct {
    uint8_t sequence;
    uint8_t payload_size;
    uint8_t payload[VISION_FRAME_MAX_PAYLOAD_SIZE];
} VisionFrame;

/*
 * 流式解析器状态。无论一帧被拆成多少次 DMA 回调，均可逐字节持续推进。
 * error_count 仅统计格式或 CRC 错误；普通垃圾字节等待帧头时不计为错误。
 */
typedef struct {
    uint8_t state;
    uint8_t sequence;
    uint8_t payload_size;
    uint8_t payload_index;
    uint8_t payload[VISION_FRAME_MAX_PAYLOAD_SIZE];
    uint16_t calculated_crc;
    uint16_t received_crc;
    uint32_t valid_count;
    uint32_t error_count;
} VisionFrameParser;

/* CRC16/MODBUS：初值 0xFFFF，多项式 0xA001，返回值为正常数值形式。 */
uint16_t VisionFrame_Crc16Modbus(const uint8_t *data, uint16_t size);

void VisionFrameParser_Init(VisionFrameParser *parser);

/* 每输入一个字节调用一次；仅在完整且 CRC 正确的一帧到达时返回 true。 */
bool VisionFrameParser_Feed(VisionFrameParser *parser, uint8_t byte,
                            VisionFrame *completed);

/* true 表示已发现帧头，仍在等待该帧的后续字节。 */
bool VisionFrameParser_IsReceiving(const VisionFrameParser *parser);

#endif
