#include "communication_watchdog.h"

void CommunicationWatchdog_Init(CommunicationWatchdog *watchdog,
                                uint32_t timeout_ms)
{
    watchdog->timeout_ms = timeout_ms;
    watchdog->last_valid_frame_ms = 0U;
    watchdog->has_valid_frame = 0U;
}

void CommunicationWatchdog_OnValidFrame(CommunicationWatchdog *watchdog,
                                         uint32_t now_ms)
{
    watchdog->last_valid_frame_ms = now_ms;
    watchdog->has_valid_frame = 1U;
}

CommunicationState CommunicationWatchdog_GetState(
    const CommunicationWatchdog *watchdog, uint32_t now_ms)
{
    if (watchdog->has_valid_frame == 0U)
    {
        return COMMUNICATION_WAITING;
    }

    if ((now_ms - watchdog->last_valid_frame_ms) >= watchdog->timeout_ms)
    {
        return COMMUNICATION_TIMEOUT;
    }

    return COMMUNICATION_ACTIVE;
}
