#include "panview_stepper.h"

#include "main.h"
#include "panview_speed_ramp.h"
#include "tim.h"

#define PAN_POSITION_MIN_STEPS (-800)
#define PAN_POSITION_MAX_STEPS (800)
#define PITCH_POSITION_MIN_STEPS (-800)
#define PITCH_POSITION_MAX_STEPS (800)

/*
 * TIM3 和 TIM4 都挂在 APB1 定时器时钟上。
 * 当前 CubeMX 时钟树中 APB1 定时器时钟为 84 MHz，不能使用 HSI 的
 * 16 MHz 代替，否则实际脉冲速度会变成命令速度的 84 / 16 = 5.25 倍。
 */
#define STEPPER_TIMER_CLOCK_HZ 84000000UL

/*
 * 一个 STEP 脉冲周期使用 2000 个定时器计数。
 * 这样在当前 1~820 step/s 范围内，计算出的 16 位 PSC 都不会溢出。
 */
#define STEPPER_TIMER_PERIOD_COUNTS 2000UL

/*
 * 水平轴最大速度暂定为 820 step/s，俯仰轴暂定为 800 step/s。
 * 定时器换算已经使用正确的 84 MHz 时钟，因此这里表示真实脉冲速度。
 */
#define PAN_MAX_SPEED_STEPS_PER_SECOND 820
#define PITCH_MAX_SPEED_STEPS_PER_SECOND 800
#define DIRECTION_CHANGE_WAIT_MS 10U
#define SPEED_RAMP_CHANGE_PER_PERIOD 60

static volatile int32_t pan_position_steps;
static volatile int32_t pitch_position_steps;
static volatile int8_t pan_running_direction;
static volatile int8_t pitch_running_direction;
static volatile uint8_t pan_running;
static volatile uint8_t pitch_running;
/* 复位后为 0；用户按 KEY_UP 确认零点后才变为 1。 */
static volatile uint8_t system_running;
static uint32_t pan_restart_tick;
static uint32_t pitch_restart_tick;
static int32_t pan_applied_speed;
static int32_t pitch_applied_speed;

static int32_t absolute_speed(int32_t speed)
{
  return speed >= 0 ? speed : -speed;
}

static void stop_pan(void)
{
  if (pan_running != 0U)
  {
    (void)HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    (void)HAL_TIM_Base_Stop_IT(&htim4);
    pan_running = 0U;
    pan_restart_tick = HAL_GetTick() + DIRECTION_CHANGE_WAIT_MS;
  }
  /* 运行状态下只停止脉冲并保持力矩；暂停状态下才真正失能。 */
  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin,
                    system_running != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
  pan_running_direction = 0;
}

static void stop_pitch(void)
{
  if (pitch_running != 0U)
  {
    (void)HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
    (void)HAL_TIM_Base_Stop_IT(&htim3);
    pitch_running = 0U;
    pitch_restart_tick = HAL_GetTick() + DIRECTION_CHANGE_WAIT_MS;
  }

  /* 俯仰轴受重力影响，运行状态下停止脉冲时也要保持力矩。 */
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin,
                    system_running != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
  pitch_running_direction = 0;
}

static int32_t limit_signed_speed(int32_t speed, int32_t maximum_speed)
{
  if (speed > maximum_speed)
  {
    return maximum_speed;
  }
  if (speed < -maximum_speed)
  {
    return -maximum_speed;
  }
  return speed;
}

static int32_t execute_pan(int32_t requested_speed, uint8_t valid)
{
  int32_t magnitude = absolute_speed(requested_speed);
  int8_t direction = requested_speed > 0 ? 1 : (requested_speed < 0 ? -1 : 0);
  uint8_t limit_blocked =
      ((pan_position_steps >= PAN_POSITION_MAX_STEPS) && (direction > 0)) ||
      ((pan_position_steps <= PAN_POSITION_MIN_STEPS) && (direction < 0));

  if ((valid == 0U) || (magnitude == 0) || (limit_blocked != 0U))
  {
    stop_pan();
    return 0;
  }

  if (magnitude > PAN_MAX_SPEED_STEPS_PER_SECOND)
  {
    magnitude = PAN_MAX_SPEED_STEPS_PER_SECOND;
  }

  if ((pan_running != 0U) && (pan_running_direction != direction))
  {
    stop_pan();
    return 0;
  }

  if ((int32_t)(HAL_GetTick() - pan_restart_tick) < 0)
  {
    return 0;
  }

  HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port, MOTOR_DIR_Pin,
                    direction > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  __HAL_TIM_SET_PRESCALER(
      &htim4,
      (uint16_t)(STEPPER_TIMER_CLOCK_HZ /
                 ((uint32_t)magnitude * STEPPER_TIMER_PERIOD_COUNTS) - 1UL));
  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_SET);

  if ((pan_running == 0U) &&
      (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1) == HAL_OK) &&
      (HAL_TIM_Base_Start_IT(&htim4) == HAL_OK))
  {
    pan_running = 1U;
    pan_running_direction = direction;
  }

  return pan_running != 0U ? direction * magnitude : 0;
}

static int32_t execute_pitch(int32_t requested_speed, uint8_t valid)
{
  int32_t magnitude = absolute_speed(requested_speed);
  int8_t direction = requested_speed > 0 ? 1 : (requested_speed < 0 ? -1 : 0);
  uint8_t limit_blocked =
      ((pitch_position_steps >= PITCH_POSITION_MAX_STEPS) && (direction > 0)) ||
      ((pitch_position_steps <= PITCH_POSITION_MIN_STEPS) && (direction < 0));

  if ((valid == 0U) || (magnitude == 0) || (limit_blocked != 0U))
  {
    stop_pitch();
    return 0;
  }

  if (magnitude > PITCH_MAX_SPEED_STEPS_PER_SECOND)
  {
    magnitude = PITCH_MAX_SPEED_STEPS_PER_SECOND;
  }

  if ((pitch_running != 0U) && (pitch_running_direction != direction))
  {
    stop_pitch();
    return 0;
  }

  if ((int32_t)(HAL_GetTick() - pitch_restart_tick) < 0)
  {
    return 0;
  }

  HAL_GPIO_WritePin(PITCH_DIR_GPIO_Port, PITCH_DIR_Pin,
                    direction > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  __HAL_TIM_SET_PRESCALER(
      &htim3,
      (uint16_t)(STEPPER_TIMER_CLOCK_HZ /
                 ((uint32_t)magnitude * STEPPER_TIMER_PERIOD_COUNTS) - 1UL));
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_SET);

  if ((pitch_running == 0U) &&
      (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4) == HAL_OK) &&
      (HAL_TIM_Base_Start_IT(&htim3) == HAL_OK))
  {
    pitch_running = 1U;
    pitch_running_direction = direction;
  }

  return pitch_running != 0U ? direction * magnitude : 0;
}

void PanView_Stepper_Init(void)
{
  /*
   * ARR=1999 表示每 2000 个计数产生一次更新事件，也就是一个 STEP 周期。
   * CCR 取周期的一半，让 STEP 输出保持约 50% 占空比。
   */
  __HAL_TIM_SET_AUTORELOAD(&htim4, STEPPER_TIMER_PERIOD_COUNTS - 1UL);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1,
                        STEPPER_TIMER_PERIOD_COUNTS / 2UL);
  __HAL_TIM_SET_AUTORELOAD(&htim3, STEPPER_TIMER_PERIOD_COUNTS - 1UL);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4,
                        STEPPER_TIMER_PERIOD_COUNTS / 2UL);

  pan_position_steps = 0;
  pitch_position_steps = 0;
  pan_running_direction = 0;
  pitch_running_direction = 0;
  pan_running = 0U;
  pitch_running = 0U;
  system_running = 0U;
  pan_restart_tick = 0U;
  pitch_restart_tick = 0U;
  pan_applied_speed = 0;
  pitch_applied_speed = 0;

  /* 复位后必须等待用户确认机械零点，因此双轴保持失能。 */
  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_RESET);
}

void PanView_Stepper_ToggleSystemRun(void)
{
  if (system_running != 0U)
  {
    /* 运行中按键：先切到暂停状态，让停止函数同时释放使能。 */
    system_running = 0U;
    pan_applied_speed = 0;
    pitch_applied_speed = 0;
    stop_pan();
    stop_pitch();
    return;
  }

  /*
   * 暂停中按键：当前机械位置就是新的软件零点。
   * 先确保定时器已经停止，再清零计数，避免中断同时修改位置。
   */
  stop_pan();
  stop_pitch();
  pan_position_steps = 0;
  pitch_position_steps = 0;
  pan_applied_speed = 0;
  pitch_applied_speed = 0;
  pan_restart_tick = HAL_GetTick();
  pitch_restart_tick = HAL_GetTick();
  system_running = 1U;

  /* 开始跟踪后，即使暂时没有目标也保持双轴力矩。 */
  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port, MOTOR_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PITCH_EN_GPIO_Port, PITCH_EN_Pin, GPIO_PIN_SET);
}

void PanView_Stepper_Execute(const MotionCommand *command)
{
  int32_t pan_target_speed = 0;
  int32_t pitch_target_speed = 0;
  uint8_t valid = 0U;

  if (system_running == 0U)
  {
    /* 尚未确认零点或已经暂停：忽略所有运动命令并保持失能。 */
    pan_applied_speed = 0;
    pitch_applied_speed = 0;
    stop_pan();
    stop_pitch();
    return;
  }

  if (command != 0)
  {
    pan_target_speed = limit_signed_speed(
        command->pan_speed_steps_per_second,
        PAN_MAX_SPEED_STEPS_PER_SECOND);
    pitch_target_speed = limit_signed_speed(
        command->pitch_speed_steps_per_second,
        PITCH_MAX_SPEED_STEPS_PER_SECOND);
    valid = command->valid;
  }

  if (valid == 0U)
  {
    /* 无效命令代表停止要求，安全停止不能等待减速斜坡。 */
    pan_applied_speed = 0;
    pitch_applied_speed = 0;
  }
  else
  {
    pan_applied_speed = PanView_SpeedRamp_Step(
        pan_applied_speed, pan_target_speed,
        SPEED_RAMP_CHANGE_PER_PERIOD);
    pitch_applied_speed = PanView_SpeedRamp_Step(
        pitch_applied_speed, pitch_target_speed,
        SPEED_RAMP_CHANGE_PER_PERIOD);
  }

  pan_applied_speed = execute_pan(pan_applied_speed, valid);
  pitch_applied_speed = execute_pitch(pitch_applied_speed, valid);
}

void PanView_Stepper_GetStatus(PanViewStepperStatus *status)
{
  if (status == 0)
  {
    return;
  }

  status->pan_position_steps = pan_position_steps;
  status->pitch_position_steps = pitch_position_steps;
  status->pan_applied_speed_steps_per_second = pan_applied_speed;
  status->pitch_applied_speed_steps_per_second = pitch_applied_speed;
  status->pan_running = pan_running;
  status->pitch_running = pitch_running;
  status->system_running = system_running;
}

void PanView_Stepper_OnTimerElapsed(TIM_HandleTypeDef *htim)
{
  if ((htim == &htim4) && (pan_running != 0U))
  {
    pan_position_steps += pan_running_direction;
  }
  else if ((htim == &htim3) && (pitch_running != 0U))
  {
    pitch_position_steps += pitch_running_direction;
  }
}
