#include "panview_message_bus.h"

#include "panview_messages.h"

enum
{
  /* 视觉结果只保留最新一帧，因此队列容量为 1。 */
  PANVIEW_VISION_RESULT_QUEUE_CAPACITY = 1U,

  /* 运动执行同样只关心最新命令，不累计已经过时的速度。 */
  PANVIEW_MOTION_COMMAND_QUEUE_CAPACITY = 1U
};

static osMessageQueueId_t vision_result_queue;
static osMessageQueueId_t motion_command_queue;

static const osMessageQueueAttr_t vision_result_queue_attributes = {
  .name = "VisionResultQueue"
};

static const osMessageQueueAttr_t motion_command_queue_attributes = {
  .name = "MotionCommandQueue"
};

int PanView_MessageBus_Init(void)
{
  if (vision_result_queue == NULL)
  {
    vision_result_queue = osMessageQueueNew(
        PANVIEW_VISION_RESULT_QUEUE_CAPACITY,
        sizeof(VisionResult),
        &vision_result_queue_attributes);
  }

  if (motion_command_queue == NULL)
  {
    motion_command_queue = osMessageQueueNew(
        PANVIEW_MOTION_COMMAND_QUEUE_CAPACITY,
        sizeof(MotionCommand),
        &motion_command_queue_attributes);
  }

  return ((vision_result_queue != NULL) && (motion_command_queue != NULL))
             ? 0
             : -1;
}

osMessageQueueId_t PanView_MessageBus_GetVisionResultQueue(void)
{
  return vision_result_queue;
}

int PanView_MessageBus_PublishVisionResult(const VisionResult *result)
{
  if ((vision_result_queue == NULL) || (result == NULL))
  {
    return -1;
  }

  /* 队列容量为 1：新结果到来时先丢弃旧结果，只保留最新值。 */
  (void)osMessageQueueReset(vision_result_queue);
  return (osMessageQueuePut(vision_result_queue, result, 0U, 0U) == osOK)
             ? 0
             : -1;
}

int PanView_MessageBus_ReadVisionResult(VisionResult *result)
{
  if ((vision_result_queue == NULL) || (result == NULL))
  {
    return -1;
  }

  return (osMessageQueueGet(vision_result_queue, result, NULL, 0U) == osOK)
             ? 0
             : -1;
}

int PanView_MessageBus_PublishMotionCommand(const MotionCommand *command)
{
  if ((motion_command_queue == NULL) || (command == NULL))
  {
    return -1;
  }

  /* 单槽队列实现“最新值”语义：先清除旧命令，再放入本轮命令。 */
  (void)osMessageQueueReset(motion_command_queue);
  return (osMessageQueuePut(motion_command_queue, command, 0U, 0U) == osOK)
             ? 0
             : -1;
}

int PanView_MessageBus_ReadMotionCommand(MotionCommand *command)
{
  if ((motion_command_queue == NULL) || (command == NULL))
  {
    return -1;
  }

  return (osMessageQueueGet(motion_command_queue, command, NULL, 0U) == osOK)
             ? 0
             : -1;
}
