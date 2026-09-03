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
