/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "communication_watchdog.h"
#include "debounced_button.h"
#include "frame_sequence_tracker.h"
#include "periodic_task.h"
#include "uart_rx_frame.h"
#include "uart_text_command.h"
#include "vision_frame_parser.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* 启动后通过 USART1 输出，作为串口链路可用的第一条证据。 */
static uint8_t boot_message[] = "PanView P01 boot\r\n";

enum {
  /* 单位：ms；串口发送阻塞等待上限，仅用于当前诊断日志。 */
  BOOT_LOG_TIMEOUT_MS = 100U,
  /* 单位：ms；P03 接收结果日志的串口发送等待上限。 */
  UART_RX_LOG_TIMEOUT_MS = 100U
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

enum {
  /* 单位：ms；P02 轮询与消抖设计参数，仅适用于当前 KEY0 诊断任务。 */
  KEY_SCAN_PERIOD_MS = 10U,
  KEY_DEBOUNCE_MS = 20U,
  STATUS_PERIOD_MS = 100U,
  HEARTBEAT_PERIOD_MS = 500U,
  /* 单位：ms；P03 通信超时观察参数，不是最终云台安全阈值。 */
  COMMUNICATION_TIMEOUT_MS = 1000U
};

/* KEY0 事件与心跳日志共用 USART1；心跳缓冲区避免逐字符发送。 */
static uint8_t key0_pressed_message[] = "KEY0 pressed\r\n";
static char heartbeat_message[96];

/* DMA 先写入此缓冲区；回调复制到 uart_rx_frame 后立即重启下一次接收。 */
static uint8_t uart_rx_dma_buffer[UART_RX_FRAME_CAPACITY];
static UartRxFrame uart_rx_frame;
static char uart_rx_log_message[128];
static uint8_t uart_pong_message[] = "PONG\r\n";
static uint8_t uart_unknown_command_message[] = "ERR unknown command\r\n";
static VisionFrameParser vision_frame_parser;
static VisionFrame latest_vision_frame;
static bool latest_vision_frame_valid;
static CommunicationWatchdog communication_watchdog;
static FrameSequenceTracker frame_sequence_tracker;

static const char *CommunicationStateText(CommunicationState state)
{
  switch (state)
  {
    case COMMUNICATION_ACTIVE:
      return "active";
    case COMMUNICATION_TIMEOUT:
      return "timeout";
    case COMMUNICATION_WAITING:
    default:
      return "waiting";
  }
}

static const char *FrameSequenceResultText(FrameSequenceResult result)
{
  switch (result)
  {
    case FRAME_SEQUENCE_FIRST:
      return "first";
    case FRAME_SEQUENCE_IN_ORDER:
      return "in_order";
    case FRAME_SEQUENCE_GAP:
      return "gap";
    case FRAME_SEQUENCE_DUPLICATE:
      return "duplicate";
    case FRAME_SEQUENCE_OUT_OF_ORDER:
    default:
      return "out_of_order";
  }
}

static void StartUartRxDma(void)
{
  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_dma_buffer,
                                   sizeof(uart_rx_dma_buffer)) != HAL_OK)
  {
    Error_Handler();
  }

  /* P03 以 IDLE 或缓冲区写满作为一段数据结束，不处理半满通知。 */
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
}

/*
 * HAL 在 UART 空闲或 DMA 缓冲区写满时调用此函数。
 * 这里只做短时间的数据保存与 DMA 重启；字符串格式化和串口输出留给主循环。
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  if (huart != &huart1)
  {
    return;
  }

  (void)UartRxFrame_Store(&uart_rx_frame, uart_rx_dma_buffer, size);
  StartUartRxDma();
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  /* 三个任务对象只保存各自的时间状态，实际执行仍在同一个 while(1) 中。 */
  DebouncedButton key0_button;
  PeriodicTask key_scan_task;
  PeriodicTask status_task;
  PeriodicTask heartbeat_task;
  uint32_t last_heartbeat_log_ms = HAL_GetTick();
  uint32_t key_scan_count = 0U;
  uint32_t status_task_count = 0U;
  uint32_t heartbeat_count = 0U;

  /* KEY0 为低有效：读到 RESET 表示已按下。 */
  DebouncedButton_Init(&key0_button,
                       HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET);
  PeriodicTask_Init(&key_scan_task, KEY_SCAN_PERIOD_MS, HAL_GetTick());
  PeriodicTask_Init(&status_task, STATUS_PERIOD_MS, HAL_GetTick());
  PeriodicTask_Init(&heartbeat_task, HEARTBEAT_PERIOD_MS, HAL_GetTick());
  UartRxFrame_Init(&uart_rx_frame);
  VisionFrameParser_Init(&vision_frame_parser);
  CommunicationWatchdog_Init(&communication_watchdog, COMMUNICATION_TIMEOUT_MS);
  FrameSequenceTracker_Init(&frame_sequence_tracker);
  StartUartRxDma();
  /* 所有外设初始化完成后发送启动日志；失败则进入统一错误处理。 */
  if (HAL_UART_Transmit(&huart1, boot_message,
                        sizeof(boot_message) - 1U,
                        BOOT_LOG_TIMEOUT_MS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t current_tick = HAL_GetTick();

    /* 10 ms：采样 KEY0，并将抖动过滤与主循环其他工作解耦。 */
    if (PeriodicTask_IsDue(&key_scan_task, current_tick))
    {
      bool key0_pressed;

      key_scan_count++;
      key0_pressed = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET;

      if (DebouncedButton_Update(&key0_button, key0_pressed, current_tick,
                                 KEY_DEBOUNCE_MS))
      {
        HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9);

        if (HAL_UART_Transmit(&huart1, key0_pressed_message,
                              sizeof(key0_pressed_message) - 1U,
                              BOOT_LOG_TIMEOUT_MS) != HAL_OK)
        {
          Error_Handler();
        }
      }
    }

    /* 100 ms：预留的状态任务；当前只计数，用于验证其独立运行。 */
    if (PeriodicTask_IsDue(&status_task, current_tick))
    {
      status_task_count++;
    }

    /* 500 ms：输出心跳、实际间隔和其他任务计数，作为非阻塞调度证据。 */
    if (PeriodicTask_IsDue(&heartbeat_task, current_tick))
    {
      uint32_t heartbeat_interval_ms = current_tick - last_heartbeat_log_ms;
      int heartbeat_length;
      CommunicationState communication_state;

      last_heartbeat_log_ms = current_tick;
      communication_state = CommunicationWatchdog_GetState(&communication_watchdog,
                                                            current_tick);
      heartbeat_length = snprintf(
          heartbeat_message, sizeof(heartbeat_message),
          "t=%lu dt=%lu heartbeat=%lu scans=%lu status=%lu link=%s\r\n",
          (unsigned long)current_tick,
          (unsigned long)heartbeat_interval_ms,
          (unsigned long)++heartbeat_count,
          (unsigned long)key_scan_count,
          (unsigned long)status_task_count,
          CommunicationStateText(communication_state));

      if ((heartbeat_length <= 0) ||
          (heartbeat_length >= (int)sizeof(heartbeat_message)) ||
          (HAL_UART_Transmit(&huart1,
                             (uint8_t *)heartbeat_message,
                             (uint16_t)heartbeat_length,
                             BOOT_LOG_TIMEOUT_MS) != HAL_OK))
      {
        Error_Handler();
      }
    }

    /* 主循环按需识别文本命令或喂给二进制帧解析器。 */
    if (uart_rx_frame.pending)
    {
      UartTextCommand command = UartTextCommand_Parse(uart_rx_frame.data,
                                                       uart_rx_frame.size);
      uint8_t *response = NULL;
      uint16_t response_size = 0U;

      if (command == UART_TEXT_COMMAND_PING)
      {
        response = uart_pong_message;
        response_size = sizeof(uart_pong_message) - 1U;
      }
      else if (command == UART_TEXT_COMMAND_STATUS)
      {
        int uart_rx_log_length = snprintf(
            uart_rx_log_message, sizeof(uart_rx_log_message),
            "STATUS size=%u rx=%lu drop=%lu frame_ok=%lu frame_err=%lu last=%u miss=%lu dup=%lu old=%lu link=%s\r\n",
            (unsigned int)uart_rx_frame.size,
            (unsigned long)uart_rx_frame.received_count,
            (unsigned long)uart_rx_frame.dropped_count,
            (unsigned long)vision_frame_parser.valid_count,
            (unsigned long)vision_frame_parser.error_count,
            (unsigned int)(latest_vision_frame_valid
                               ? latest_vision_frame.sequence
                               : 0U),
            (unsigned long)frame_sequence_tracker.missing_count,
            (unsigned long)frame_sequence_tracker.duplicate_count,
            (unsigned long)frame_sequence_tracker.out_of_order_count,
            CommunicationStateText(CommunicationWatchdog_GetState(
                &communication_watchdog, HAL_GetTick())));

        if ((uart_rx_log_length <= 0) ||
            (uart_rx_log_length >= (int)sizeof(uart_rx_log_message)))
        {
          Error_Handler();
        }

        response = (uint8_t *)uart_rx_log_message;
        response_size = (uint16_t)uart_rx_log_length;
      }
      else
      {
        bool frame_accepted = false;
        uint32_t error_count_before = vision_frame_parser.error_count;
        uint16_t byte_index;
        VisionFrame received_vision_frame = {0};
        FrameSequenceResult last_sequence_result = FRAME_SEQUENCE_FIRST;

        for (byte_index = 0U; byte_index < uart_rx_frame.size; byte_index++)
        {
          if (VisionFrameParser_Feed(&vision_frame_parser,
                                     uart_rx_frame.data[byte_index],
                                     &received_vision_frame))
          {
            frame_accepted = true;
            last_sequence_result = FrameSequenceTracker_Observe(
                &frame_sequence_tracker, received_vision_frame.sequence);
            CommunicationWatchdog_OnValidFrame(&communication_watchdog,
                                                HAL_GetTick());

            /* 旧帧或重复帧不应倒退当前视觉目标，但仍证明链路没有超时。 */
            if ((last_sequence_result != FRAME_SEQUENCE_DUPLICATE) &&
                (last_sequence_result != FRAME_SEQUENCE_OUT_OF_ORDER))
            {
              latest_vision_frame = received_vision_frame;
              latest_vision_frame_valid = true;
            }
          }
        }

        if (frame_accepted)
        {
          int uart_rx_log_length = snprintf(
              uart_rx_log_message, sizeof(uart_rx_log_message),
              "FRAME OK seq=%u len=%u order=%s valid=%lu err=%lu miss=%lu\r\n",
              (unsigned int)received_vision_frame.sequence,
              (unsigned int)received_vision_frame.payload_size,
              FrameSequenceResultText(last_sequence_result),
              (unsigned long)vision_frame_parser.valid_count,
              (unsigned long)vision_frame_parser.error_count,
              (unsigned long)frame_sequence_tracker.missing_count);

          if ((uart_rx_log_length <= 0) ||
              (uart_rx_log_length >= (int)sizeof(uart_rx_log_message)))
          {
            Error_Handler();
          }

          response = (uint8_t *)uart_rx_log_message;
          response_size = (uint16_t)uart_rx_log_length;
        }
        else if (vision_frame_parser.error_count != error_count_before)
        {
          int uart_rx_log_length = snprintf(
              uart_rx_log_message, sizeof(uart_rx_log_message),
              "FRAME ERR valid=%lu err=%lu\r\n",
              (unsigned long)vision_frame_parser.valid_count,
              (unsigned long)vision_frame_parser.error_count);

          if ((uart_rx_log_length <= 0) ||
              (uart_rx_log_length >= (int)sizeof(uart_rx_log_message)))
          {
            Error_Handler();
          }

          response = (uint8_t *)uart_rx_log_message;
          response_size = (uint16_t)uart_rx_log_length;
        }
        else if (!VisionFrameParser_IsReceiving(&vision_frame_parser))
        {
          response = uart_unknown_command_message;
          response_size = sizeof(uart_unknown_command_message) - 1U;
        }
      }

      if ((response_size > 0U) &&
          (HAL_UART_Transmit(&huart1, response, response_size,
                             UART_RX_LOG_TIMEOUT_MS) != HAL_OK))
      {
        Error_Handler();
      }

      UartRxFrame_Release(&uart_rx_frame);
    }
  }

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
