#include "panview_message_bus.h"

#include "panview_messages.h"

enum
{
  /* 视觉结果只保留最新一帧，因此队列容量为 1。 */
  PANVIEW_VISION_RESULT_QUEUE_CAPACITY = 1U
};

static osMessageQueueId_t vision_result_queue;

static const osMessageQueueAttr_t vision_result_queue_attributes = {
  .name = "VisionResultQueue"
};

int PanView_MessageBus_Init(void)
{
  if (vision_result_queue != NULL)
  {
    return 0;
  }

  vision_result_queue = osMessageQueueNew(
      PANVIEW_VISION_RESULT_QUEUE_CAPACITY,
      sizeof(VisionResult),
      &vision_result_queue_attributes);

  return (vision_result_queue != NULL) ? 0 : -1;
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
