/*
 * 文件用途：管理 PanView 任务之间共享的消息队列句柄。
 *
 * 本步只建立队列所有权和访问接口，不负责发送或读取消息。
 */
#ifndef PANVIEW_MESSAGE_BUS_H
#define PANVIEW_MESSAGE_BUS_H

#include "cmsis_os2.h"

/* 创建消息总线中的所有队列；成功返回 0，失败返回 -1。 */
int PanView_MessageBus_Init(void);

/* 获取视觉结果单槽队列句柄；未初始化时返回 NULL。 */
osMessageQueueId_t PanView_MessageBus_GetVisionResultQueue(void);

#endif /* PANVIEW_MESSAGE_BUS_H */
