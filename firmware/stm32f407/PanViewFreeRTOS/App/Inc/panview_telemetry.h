#ifndef PANVIEW_TELEMETRY_H
#define PANVIEW_TELEMETRY_H
#include <stdint.h>
#include "panview_messages.h"
#include "panview_safety.h"
#include "panview_stepper.h"
#include "panview_task_heartbeat.h"
typedef struct { uint32_t tick_ms, free_heap_bytes; uint8_t running, fault_latched, target_present, pan_limit, pitch_limit; int16_t error_x, error_y; int32_t pan_position, pitch_position, pan_speed, pitch_speed; uint32_t uart_bytes, uart_dropped, uart_errors, uart_last_length; PanViewSafetyFault fault; PanViewHeartbeatRecord task[PANVIEW_HEARTBEAT_COUNT]; } PanViewTelemetrySnapshot;
void PanView_Telemetry_Collect(PanViewTelemetrySnapshot *snapshot);
int PanView_Telemetry_Format(const PanViewTelemetrySnapshot *snapshot, char *buffer, uint32_t buffer_size);
#endif
