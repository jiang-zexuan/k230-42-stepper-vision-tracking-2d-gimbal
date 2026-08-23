#include "panview_ui.h"
#include <stdio.h>

enum { UI_DEFAULT_BG = 0x18E3U, UI_DEFAULT_PANEL = 0x2965U,
       UI_DEFAULT_LINE = 0x4D76U, UI_DEFAULT_CYAN = 0x5DFFU,
       UI_DEFAULT_AMBER = 0xFD20U, UI_DEFAULT_RED = 0xF800U,
       UI_DEFAULT_GREEN = 0x07E0U, UI_DEFAULT_GRAY = 0x8410U };

static UiPage page;
static UiLanguage language;
static bool drawn;
static uint32_t last_refresh;
static UiVisualState last_state;
static UiPage last_page;
static UiTheme theme;

static uint16_t ThemeBg(void) { switch (theme) {
  case UI_THEME_BLACK_GOLD: return 0x2121U; case UI_THEME_NEON: return 0x190FU;
  case UI_THEME_MONO: return 0x39C7U; case UI_THEME_FLUORESCENT_GREEN: return 0x1183U;
  default: return UI_DEFAULT_BG; } }
static uint16_t ThemePanel(void) { switch (theme) {
  case UI_THEME_BLACK_GOLD: return 0x39C5U; case UI_THEME_NEON: return 0x3147U;
  case UI_THEME_MONO: return 0x5AEBU; case UI_THEME_FLUORESCENT_GREEN: return 0x1AC5U;
  default: return UI_DEFAULT_PANEL; } }
static uint16_t ThemeLine(void) { switch (theme) {
  case UI_THEME_BLACK_GOLD: return 0x7BE0U; case UI_THEME_NEON: return 0x74BFU;
  case UI_THEME_MONO: return 0xBDF7U; case UI_THEME_FLUORESCENT_GREEN: return 0x5EFAU;
  default: return UI_DEFAULT_LINE; } }
static uint16_t ThemeCyan(void) { switch (theme) {
  case UI_THEME_BLACK_GOLD: return 0xFEA0U; case UI_THEME_NEON: return 0x7DFFU;
  case UI_THEME_MONO: return 0xFFFFU; case UI_THEME_FLUORESCENT_GREEN: return 0x87FFU;
  default: return UI_DEFAULT_CYAN; } }
static uint16_t ThemeAmber(void) { switch (theme) {
  case UI_THEME_BLACK_GOLD: return 0xFD20U; case UI_THEME_NEON: return 0xF81FU;
  case UI_THEME_MONO: return 0xFFFFU; case UI_THEME_FLUORESCENT_GREEN: return 0x87F0U;
  default: return UI_DEFAULT_AMBER; } }
static uint16_t ThemeGray(void) { switch (theme) {
  case UI_THEME_BLACK_GOLD: return 0xC5B1U; case UI_THEME_NEON: return 0xA5B9U;
  case UI_THEME_MONO: return 0xEF7DU; case UI_THEME_FLUORESCENT_GREEN: return 0xA6D7U;
  default: return UI_DEFAULT_GRAY; } }
static uint16_t ThemeLost(void) { switch (theme) {
  case UI_THEME_BLACK_GOLD: return 0xFC80U; case UI_THEME_NEON: return 0xFA60U;
  case UI_THEME_MONO: return 0xFBE0U; case UI_THEME_FLUORESCENT_GREEN: return 0xFD20U;
  default: return UI_DEFAULT_AMBER; } }
#define UI_BG ThemeBg()
#define UI_PANEL ThemePanel()
#define UI_LINE ThemeLine()
#define UI_CYAN ThemeCyan()
#define UI_AMBER ThemeAmber()
#define UI_RED UI_DEFAULT_RED
#define UI_GREEN UI_DEFAULT_GREEN
#define UI_GRAY ThemeGray()
#define UI_LOST ThemeLost()

static const uint16_t state_zh[][2] = {
  {0x641CU, 0x7D22U}, {0x8DDFU, 0x8E2AU}, {0x9501U, 0x5B9AU},
  {0x4E22U, 0x5931U}, {0x6545U, 0x969CU}
};

static void DrawCn(uint16_t x, uint16_t y, const uint16_t *text,
                   uint16_t count, uint16_t foreground, uint16_t background)
{ ILI9341_DrawText16(x, y, text, count, foreground, background); }

static uint16_t StateColor(UiVisualState state)
{
  switch (state) { case UI_STATE_LOCKED: return UI_GREEN;
    case UI_STATE_TRACKING: return UI_AMBER; case UI_STATE_LOST: return UI_LOST;
    case UI_STATE_FAULT: return UI_RED; default: return UI_CYAN; }
}

static const char *StateText(UiVisualState state)
{
  switch (state) { case UI_STATE_TRACKING: return "TRACKING";
    case UI_STATE_LOCKED: return "LOCKED"; case UI_STATE_LOST: return "LOST";
    case UI_STATE_FAULT: return "FAULT"; default: return "SEARCH"; }
}

static void Line(uint16_t y) { ILI9341_FillRect(0U, y, ILI9341_WIDTH, 1U, UI_LINE); }
static void Label(uint16_t x, uint16_t y, const char *text, uint16_t color)
{ ILI9341_DrawText(x, y, text, color, UI_BG, 1U); }
static void Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                   const char *text, uint16_t color)
{
  ILI9341_FillRect(x, y, w, h, UI_PANEL);
  ILI9341_FillRect(x, y, w, 2U, color); ILI9341_FillRect(x, (uint16_t)(y+h-2U), w, 2U, color);
  ILI9341_FillRect(x, y, 2U, h, color); ILI9341_FillRect((uint16_t)(x+w-2U), y, 2U, h, color);
  ILI9341_DrawText((uint16_t)(x + 8U), (uint16_t)(y + 12U), text, color, UI_PANEL, 1U);
}

static void Panel(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  ILI9341_FillRect(x, y, width, height, UI_PANEL);
  ILI9341_FillRect(x, y, width, 1U, UI_LINE);
  ILI9341_FillRect(x, (uint16_t)(y + height - 1U), width, 1U, UI_LINE);
  ILI9341_FillRect(x, y, 1U, height, UI_LINE);
  ILI9341_FillRect((uint16_t)(x + width - 1U), y, 1U, height, UI_LINE);
}

static void DrawRunHeader(void)
{
  ILI9341_FillRect(0U, 0U, 240U, 32U, UI_PANEL);
  Line(32U);
  ILI9341_DrawText(6U, 8U, "PANVIEW", UI_CYAN, UI_PANEL, 2U);
}

static void DrawStatusFocus(const PanViewUiData *data)
{
  uint16_t color = StateColor(data->state);
  uint16_t text_x;
  ILI9341_FillRect(60U, 210U, 120U, 24U, UI_PANEL);
  ILI9341_FillRect(60U, 210U, 120U, 1U, color);
  ILI9341_FillRect(60U, 233U, 120U, 1U, color);
  ILI9341_FillRect(60U, 210U, 1U, 24U, color);
  ILI9341_FillRect(179U, 210U, 1U, 24U, color);
  if (language == UI_LANGUAGE_ZH)
    DrawCn(104U, 214U, state_zh[data->state], 2U, color, UI_PANEL);
  else
  {
    switch (data->state)
    {
      case UI_STATE_TRACKING: text_x = 96U; break;
      case UI_STATE_SEARCH: text_x = 102U; break;
      case UI_STATE_LOCKED: text_x = 102U; break;
      case UI_STATE_LOST: text_x = 108U; break;
      case UI_STATE_FAULT:
      default: text_x = 108U; break;
    }
    ILI9341_DrawText(text_x, 218U, StateText(data->state), color, UI_PANEL, 1U);
  }
}

void PanViewUi_Init(UiLanguage initial_language)
{ page = UI_PAGE_RUN; language = initial_language; theme = UI_THEME_BLACK_GOLD; drawn = false; last_refresh = 0U; last_page = UI_PAGE_RUN; }
void PanViewUi_SetLanguage(UiLanguage selected) { language = selected; drawn = false; }
UiLanguage PanViewUi_GetLanguage(void) { return language; }
UiPage PanViewUi_GetPage(void) { return page; }
void PanViewUi_SetTheme(UiTheme selected) { theme = selected; drawn = false; }
UiTheme PanViewUi_GetTheme(void) { return theme; }

static void DrawRun(const PanViewUiData *d)
{
  char line[40];
  ILI9341_FillColor(UI_BG);
  DrawRunHeader();
  Panel(8U, 40U, 224U, 42U);
  if (language == UI_LANGUAGE_ZH) {
    static const uint16_t target[] = {0x76EEU, 0x6807U};
    DrawCn(16U, 46U, target, 2U, d->target_present ? UI_GREEN : UI_GRAY, UI_PANEL);
  } else {
    (void)snprintf(line, sizeof(line), "TARGET: %s", d->target_present ? "DETECTED" : "NONE"); Label(16U, 48U, line, d->target_present ? UI_GREEN : UI_GRAY);
  }
  (void)snprintf(line, sizeof(line), "ERR X: %d  Y: %d", d->error_x, d->error_y); Label(16U, 66U, line, UI_CYAN);
  Panel(8U, 88U, 224U, 96U);
  (void)snprintf(line, sizeof(line), "PAN   POS %+06ld", (long)d->pan_position); Label(16U, 94U, line, UI_CYAN);
  (void)snprintf(line, sizeof(line), "PITCH POS %+06ld", (long)d->pitch_position); Label(16U, 118U, line, UI_CYAN);
  (void)snprintf(line, sizeof(line), "PAN SPEED   %04ld", (long)d->pan_speed); Label(16U, 142U, line, UI_AMBER);
  (void)snprintf(line, sizeof(line), "PITCH SPEED %04ld", (long)d->pitch_speed); Label(16U, 166U, line, UI_AMBER);
  (void)snprintf(line, sizeof(line), "LINK RX %lu DROP %lu", (unsigned long)d->rx_frames, (unsigned long)d->rx_drops); Label(16U, 192U, line, UI_GRAY);
  DrawStatusFocus(d);
  Button(8U, 242U, 108U, 36U, d->running ? "STOP" : "START", UI_AMBER);
  Button(124U, 242U, 108U, 36U, "SET", UI_CYAN);
  ILI9341_DrawText(72U, 300U, "DESIGNED BY JZX", UI_GRAY, UI_BG, 1U);
}

static void DrawRunDynamic(const PanViewUiData *d)
{
  char line[40];
  ILI9341_FillRect(9U, 41U, 222U, 40U, UI_PANEL);
  if (language == UI_LANGUAGE_ZH) {
    static const uint16_t target[] = {0x76EEU, 0x6807U};
    DrawCn(16U, 46U, target, 2U, d->target_present ? UI_GREEN : UI_GRAY, UI_PANEL);
  } else {
    (void)snprintf(line, sizeof(line), "TARGET: %s", d->target_present ? "DETECTED" : "NONE");
    Label(16U, 48U, line, d->target_present ? UI_GREEN : UI_GRAY);
  }
  (void)snprintf(line, sizeof(line), "ERR X: %d  Y: %d", d->error_x, d->error_y); Label(16U, 66U, line, UI_CYAN);
  ILI9341_FillRect(9U, 89U, 222U, 94U, UI_PANEL);
  (void)snprintf(line, sizeof(line), "PAN   POS %+06ld", (long)d->pan_position); Label(16U, 94U, line, UI_CYAN);
  (void)snprintf(line, sizeof(line), "PITCH POS %+06ld", (long)d->pitch_position); Label(16U, 118U, line, UI_CYAN);
  (void)snprintf(line, sizeof(line), "PAN SPEED   %04ld", (long)d->pan_speed); Label(16U, 142U, line, UI_AMBER);
  (void)snprintf(line, sizeof(line), "PITCH SPEED %04ld", (long)d->pitch_speed); Label(16U, 166U, line, UI_AMBER);
  (void)snprintf(line, sizeof(line), "LINK RX %lu DROP %lu", (unsigned long)d->rx_frames, (unsigned long)d->rx_drops); Label(16U, 192U, line, UI_GRAY);
  DrawStatusFocus(d);
}

static void DrawSettings(void)
{
  static const uint16_t zh_settings[] = {0x8BBEU};
  static const uint16_t zh_language[] = {0x8BBEU, 0x4E2DU, 0x6587U};
  static const uint16_t zh_diagnostic[] = {0x8BCAU, 0x65ADU};
  static const uint16_t zh_back[] = {0x8FD4U, 0x56DEU};
  ILI9341_FillColor(UI_BG); ILI9341_FillRect(0U, 0U, 240U, 32U, UI_PANEL); Line(32U);
  ILI9341_DrawText(6U, 8U, "SETTINGS", UI_CYAN, UI_PANEL, 2U);
  Label(8U, 52U, "LANG / LANGUAGE", UI_CYAN);
  Button(8U, 92U, 104U, 38U, "CHINESE", language == UI_LANGUAGE_ZH ? UI_AMBER : UI_CYAN);
  Button(128U, 92U, 104U, 38U, "ENGLISH", language == UI_LANGUAGE_EN ? UI_AMBER : UI_CYAN);
  Label(8U, 138U, "THEME", UI_CYAN);
  Button(8U, 154U, 104U, 28U, "BLACK GOLD", theme == UI_THEME_BLACK_GOLD ? UI_AMBER : UI_GRAY);
  Button(128U, 154U, 104U, 28U, "NEON", theme == UI_THEME_NEON ? UI_AMBER : UI_GRAY);
  Button(8U, 186U, 104U, 28U, "MONO", theme == UI_THEME_MONO ? UI_AMBER : UI_GRAY);
  Button(128U, 186U, 104U, 28U, "GREEN", theme == UI_THEME_FLUORESCENT_GREEN ? UI_AMBER : UI_GRAY);
  Button(8U, 220U, 224U, 32U, "DIAGNOSTIC", UI_CYAN);
  Button(8U, 264U, 224U, 32U, "BACK", UI_GRAY);
  if (language == UI_LANGUAGE_ZH) {
    DrawCn(154U, 8U, zh_settings, 1U, UI_CYAN, UI_PANEL);
    DrawCn(154U, 103U, zh_language + 1U, 2U, UI_AMBER, UI_PANEL);
    DrawCn(180U, 228U, zh_diagnostic, 2U, UI_CYAN, UI_PANEL);
    DrawCn(182U, 272U, zh_back, 2U, UI_GRAY, UI_PANEL);
  }
}

static void DrawDiagnostic(const PanViewUiData *d)
{
  static const uint16_t zh_back[] = {0x8FD4U, 0x56DEU};
  char line[42]; ILI9341_FillColor(UI_BG); ILI9341_FillRect(0U,0U,240U,32U,UI_PANEL); Line(32U);
  ILI9341_DrawText(6U, 8U, "DIAGNOSTIC", UI_CYAN, UI_PANEL, 2U);
  (void)snprintf(line,sizeof(line),"RX %lu DROP %lu ERR %lu",(unsigned long)d->rx_frames,(unsigned long)d->rx_drops,(unsigned long)d->parse_errors); Label(6U,42U,line,UI_CYAN);
  (void)snprintf(line,sizeof(line),"AGE %lums POS %s",(unsigned long)d->last_frame_age_ms,d->position_valid?"OK":"INVALID"); Label(6U,58U,line,UI_GRAY);
  (void)snprintf(line,sizeof(line),"PAN %+06ld PITCH %+06ld",(long)d->pan_position,(long)d->pitch_position); Label(6U,82U,line,UI_CYAN);
  (void)snprintf(line,sizeof(line),"SPD %04ld / %04ld",(long)d->pan_speed,(long)d->pitch_speed); Label(6U,98U,line,UI_AMBER);
  (void)snprintf(line,sizeof(line),"LIMIT PAN %s PITCH %s",d->pan_limit?"HIT":"OK",d->pitch_limit?"HIT":"OK"); Label(6U,122U,line,(d->pan_limit||d->pitch_limit)?UI_RED:UI_GREEN);
  (void)snprintf(line,sizeof(line),"TOUCH raw %u,%u",d->touch_raw_x,d->touch_raw_y); Label(6U,146U,line,UI_GRAY);
  (void)snprintf(line,sizeof(line),"MAP  %u,%u N=%u",d->touch_screen_x,d->touch_screen_y,d->touch_count); Label(6U,162U,line,UI_GRAY);
  (void)snprintf(line,sizeof(line),"TFT %s TOUCH %s",d->tft_ready?"OK":"FAIL",d->touch_ready?"OK":"FAIL"); Label(6U,186U,line,UI_GREEN);
  Button(8U, 232U, 224U, 38U, "BACK", UI_GRAY);
  if (language == UI_LANGUAGE_ZH)
    DrawCn(182U, 244U, zh_back, 2U, UI_GRAY, UI_PANEL);
}

void PanViewUi_Draw(const PanViewUiData *data)
{ if (data == NULL) return; if (page == UI_PAGE_RUN) DrawRun(data); else if (page == UI_PAGE_SETTINGS) DrawSettings(); else DrawDiagnostic(data); drawn = true; last_state = data->state; last_page = page; }
void PanViewUi_Refresh(const PanViewUiData *data, uint32_t tick)
{
  if (data == NULL) return;
  if (!drawn || page != last_page)
  { PanViewUi_Draw(data); last_refresh = tick; return; }
  if ((page == UI_PAGE_RUN) && (data->state != last_state))
  { DrawRunHeader(); DrawStatusFocus(data); last_state = data->state; last_refresh = tick; return; }
  if (page != UI_PAGE_RUN)
    return;
  if ((tick - last_refresh) >= 500U)
  { DrawRunDynamic(data); last_refresh = tick; }
}

UiEvent PanViewUi_HandleTouch(uint16_t x, uint16_t y, bool pressed)
{
  if (!pressed) return UI_EVENT_NONE;
  if (page == UI_PAGE_RUN) { if (y >= 238U && y < 282U) return x < 120U ? UI_EVENT_START_STOP : UI_EVENT_OPEN_SETTINGS; }
  else if (page == UI_PAGE_SETTINGS) {
    if (y >= 88U && y < 140U) return x < 120U ? UI_EVENT_LANGUAGE_ZH : UI_EVENT_LANGUAGE_EN;
    if (y >= 150U && y < 182U) return x < 120U ? UI_EVENT_THEME_BLACK_GOLD : UI_EVENT_THEME_NEON;
    if (y >= 182U && y < 218U) return x < 120U ? UI_EVENT_THEME_MONO : UI_EVENT_THEME_FLUORESCENT_GREEN;
    if (y >= 218U && y < 258U) return UI_EVENT_OPEN_DIAGNOSTIC;
    if (y >= 258U) return UI_EVENT_BACK;
  }
  else if (page == UI_PAGE_DIAGNOSTIC && y >= 225U) return UI_EVENT_BACK;
  return UI_EVENT_NONE;
}

void PanViewUi_Navigate(UiPage next_page) { page = next_page; drawn = false; }
