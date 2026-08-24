#ifndef PANVIEW_UI_H
#define PANVIEW_UI_H

#include <stdbool.h>
#include <stdint.h>
#include "ili9341.h"

typedef enum { UI_PAGE_RUN, UI_PAGE_SETTINGS, UI_PAGE_DIAGNOSTIC } UiPage;
typedef enum { UI_LANGUAGE_ZH, UI_LANGUAGE_EN } UiLanguage;
typedef enum { UI_THEME_BLACK_GOLD, UI_THEME_NEON,
               UI_THEME_MONO, UI_THEME_FLUORESCENT_GREEN } UiTheme;
typedef enum { UI_EVENT_NONE, UI_EVENT_START_STOP, UI_EVENT_OPEN_SETTINGS,
               UI_EVENT_LANGUAGE_ZH, UI_EVENT_LANGUAGE_EN,
               UI_EVENT_THEME_BLACK_GOLD, UI_EVENT_THEME_NEON,
               UI_EVENT_THEME_MONO, UI_EVENT_THEME_FLUORESCENT_GREEN,
               UI_EVENT_OPEN_DIAGNOSTIC, UI_EVENT_BACK } UiEvent;
typedef enum { UI_STATE_SEARCH, UI_STATE_TRACKING, UI_STATE_LOCKED,
               UI_STATE_LOST, UI_STATE_FAULT } UiVisualState;

typedef struct
{
  UiVisualState state;
  bool running;
  bool target_present;
  int16_t error_x;
  int16_t error_y;
  int32_t pan_position;
  int32_t pitch_position;
  int32_t pan_speed;
  int32_t pitch_speed;
  uint32_t rx_frames;
  uint32_t rx_drops;
  uint32_t parse_errors;
  uint32_t last_frame_age_ms;
  bool position_valid;
  bool pitch_zeroed;
  bool pan_limit;
  bool pitch_limit;
  uint16_t touch_raw_x;
  uint16_t touch_raw_y;
  uint16_t touch_screen_x;
  uint16_t touch_screen_y;
  uint8_t touch_count;
  bool tft_ready;
  bool touch_ready;
} PanViewUiData;

void PanViewUi_Init(UiLanguage language);
void PanViewUi_SetLanguage(UiLanguage language);
UiLanguage PanViewUi_GetLanguage(void);
void PanViewUi_SetTheme(UiTheme theme);
UiTheme PanViewUi_GetTheme(void);
UiPage PanViewUi_GetPage(void);
void PanViewUi_Navigate(UiPage page);
void PanViewUi_Draw(const PanViewUiData *data);
void PanViewUi_Refresh(const PanViewUiData *data, uint32_t tick);
UiEvent PanViewUi_HandleTouch(uint16_t x, uint16_t y, bool pressed);

#endif
