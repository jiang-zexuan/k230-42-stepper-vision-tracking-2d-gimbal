#ifndef COMMUNICATION_WATCHDOG_H
#define COMMUNICATION_WATCHDOG_H

#include <stdint.h>

typedef enum {
    COMMUNICATION_WAITING,
    COMMUNICATION_ACTIVE,
    COMMUNICATION_TIMEOUT
} CommunicationState;

/*
 * 仅由 CRC 正确的完整帧刷新通信时间。
 * timeout_ms 的单位为 ms；P03 用它观察链路状态，P08 会按实测需求设定安全阈值。
 */
typedef struct {
    uint32_t timeout_ms;
    uint32_t last_valid_frame_ms;
    uint8_t has_valid_frame;
} CommunicationWatchdog;

void CommunicationWatchdog_Init(CommunicationWatchdog *watchdog,
                                uint32_t timeout_ms);
void CommunicationWatchdog_OnValidFrame(CommunicationWatchdog *watchdog,
                                         uint32_t now_ms);
CommunicationState CommunicationWatchdog_GetState(
    const CommunicationWatchdog *watchdog, uint32_t now_ms);

#endif
