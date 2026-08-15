#include <assert.h>
#include <stdio.h>

#include "../PanViewF407/Core/Inc/communication_watchdog.h"

static void test_watchdog_waits_for_the_first_valid_frame(void)
{
    CommunicationWatchdog watchdog;

    CommunicationWatchdog_Init(&watchdog, 1000U);

    assert(CommunicationWatchdog_GetState(&watchdog, 5000U) ==
           COMMUNICATION_WAITING);
}

static void test_watchdog_times_out_only_after_the_configured_interval(void)
{
    CommunicationWatchdog watchdog;

    CommunicationWatchdog_Init(&watchdog, 1000U);
    CommunicationWatchdog_OnValidFrame(&watchdog, 100U);

    assert(CommunicationWatchdog_GetState(&watchdog, 1099U) ==
           COMMUNICATION_ACTIVE);
    assert(CommunicationWatchdog_GetState(&watchdog, 1100U) ==
           COMMUNICATION_TIMEOUT);
}

static void test_new_valid_frame_recovers_from_timeout(void)
{
    CommunicationWatchdog watchdog;

    CommunicationWatchdog_Init(&watchdog, 1000U);
    CommunicationWatchdog_OnValidFrame(&watchdog, 0U);
    assert(CommunicationWatchdog_GetState(&watchdog, 1000U) ==
           COMMUNICATION_TIMEOUT);

    CommunicationWatchdog_OnValidFrame(&watchdog, 1200U);
    assert(CommunicationWatchdog_GetState(&watchdog, 1200U) ==
           COMMUNICATION_ACTIVE);
}

int main(void)
{
    test_watchdog_waits_for_the_first_valid_frame();
    test_watchdog_times_out_only_after_the_configured_interval();
    test_new_valid_frame_recovers_from_timeout();
    puts("communication_watchdog_test: PASS");
    return 0;
}
