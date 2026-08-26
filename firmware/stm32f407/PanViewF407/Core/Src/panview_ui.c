#include "panview_ui.h"
#include <stdio.h>
#include <string.h>

enum { UI_DEFAULT_BG = 0x18E3U, UI_DEFAULT_PANEL = 0x2965U,
       UI_DEFAULT_LINE = 0x4D76U, UI_DEFAULT_CYAN = 0x5DFFU,
       UI_DEFAULT_AMBER = 0xFD20U, UI_DEFAULT_RED = 0xF800U,
       UI_DEFAULT_GREEN = 0x07E0U, UI_DEFAULT_GRAY = 0x8410U };

static UiPage page;
static UiLanguage language;
static UiTheme theme;
static bool drawn;
static uint32_t last_refresh;
static uint32_t current_tick;
static uint32_t run_started_tick;
static bool run_active_last;
static UiVisualState last_state;
static UiPage last_page;

static uint16_t ThemeBg(void)
{
  switch (theme)
  {
    case UI_THEME_BLACK_GOLD: return 0x2121U;
    case UI_THEME_NEON: return 0x190FU;
    case UI_THEME_MONO: return 0x39C7U;
    case UI_THEME_FLUORESCENT_GREEN: return 0x1183U;
    default: return UI_DEFAULT_BG;
  }
}

static uint16_t ThemePanel(void)
{
  switch (theme)
  {
    case UI_THEME_BLACK_GOLD: return 0x39C5U;
    case UI_THEME_NEON: return 0x3147U;
    case UI_THEME_MONO: return 0x5AEBU;
    case UI_THEME_FLUORESCENT_GREEN: return 0x1AC5U;
    default: return UI_DEFAULT_PANEL;
  }
}

static uint16_t ThemeLine(void)
{
  switch (theme)
  {
    case UI_THEME_BLACK_GOLD: return 0x7BE0U;
    case UI_THEME_NEON: return 0x74BFU;
    case UI_THEME_MONO: return 0xBDF7U;
    case UI_THEME_FLUORESCENT_GREEN: return 0x5EFAU;
    default: return UI_DEFAULT_LINE;
  }
}

static uint16_t ThemeCyan(void)
{
  switch (theme)
  {
    case UI_THEME_BLACK_GOLD: return 0xFEA0U;
    case UI_THEME_NEON: return 0x7DFFU;
    case UI_THEME_MONO: return 0xFFFFU;
    case UI_THEME_FLUORESCENT_GREEN: return 0x87FFU;
    default: return UI_DEFAULT_CYAN;
  }
}

static uint16_t ThemeAmber(void)
{
  switch (theme)
  {
    case UI_THEME_BLACK_GOLD: return 0xFD20U;
    case UI_THEME_NEON: return 0xF81FU;
    case UI_THEME_MONO: return 0xFFFFU;
    case UI_THEME_FLUORESCENT_GREEN: return 0x87F0U;
    default: return UI_DEFAULT_AMBER;
  }
}

static uint16_t ThemeGray(void)
{
  switch (theme)
  {
    case UI_THEME_BLACK_GOLD: return 0xC5B1U;
    case UI_THEME_NEON: return 0xA5B9U;
    case UI_THEME_MONO: return 0xEF7DU;
    case UI_THEME_FLUORESCENT_GREEN: return 0xA6D7U;
    default: return UI_DEFAULT_GRAY;
  }
}

#define UI_BG ThemeBg()
#define UI_PANEL ThemePanel()
#define UI_LINE ThemeLine()
#define UI_CYAN ThemeCyan()
#define UI_AMBER ThemeAmber()
#define UI_GRAY ThemeGray()
#define UI_RED UI_DEFAULT_RED
#define UI_GREEN UI_DEFAULT_GREEN

static uint16_t StateColor(UiVisualState state)
{
  switch (state)
  {
    case UI_STATE_TRACKING: return UI_AMBER;
    case UI_STATE_LOCKED: return UI_GREEN;
    case UI_STATE_FAULT: return UI_RED;
    case UI_STATE_LOST: return UI_AMBER;
    case UI_STATE_SEARCH:
    default: return UI_CYAN;
  }
}

static const char *StateText(UiVisualState state)
{
  switch (state)
  {
    case UI_STATE_TRACKING: return "TRACKING";
    case UI_STATE_LOCKED: return "HIT";
    case UI_STATE_FAULT: return "FAULT";
    case UI_STATE_LOST: return "LOST";
    case UI_STATE_SEARCH:
    default: return "SEARCHING";
  }
}

static void Line(uint16_t y)
{
  ILI9341_FillRect(0U, y, ILI9341_WIDTH, 1U, UI_LINE);
}

static void Label(uint16_t x, uint16_t y, const char *text, uint16_t color)
{
  ILI9341_DrawText(x, y, text, color, UI_BG, 1U);
}

static void Panel(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  ILI9341_FillRect(x, y, width, height, UI_PANEL);
  ILI9341_FillRect(x, y, width, 1U, UI_LINE);
  ILI9341_FillRect(x, (uint16_t)(y + height - 1U), width, 1U, UI_LINE);
  ILI9341_FillRect(x, y, 1U, height, UI_LINE);
  ILI9341_FillRect((uint16_t)(x + width - 1U), y, 1U, height, UI_LINE);
}

static void Button(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   const char *text, uint16_t color, bool active)
{
  uint16_t text_width = (uint16_t)(strlen(text) * 6U);
  uint16_t text_x = (uint16_t)(x + (width > text_width ? (width - text_width) / 2U : 2U));
  ILI9341_FillRect(x, y, width, height, UI_PANEL);
  ILI9341_FillRect(x, y, width, active ? 3U : 1U, color);
  ILI9341_FillRect(x, (uint16_t)(y + height - (active ? 3U : 1U)), width,
                   active ? 3U : 1U, color);
  ILI9341_FillRect(x, y, active ? 3U : 1U, height, color);
  ILI9341_FillRect((uint16_t)(x + width - (active ? 3U : 1U)), y,
                   active ? 3U : 1U, height, color);
  ILI9341_DrawText(text_x, (uint16_t)(y + 13U), text, color, UI_PANEL, 1U);
}

static void FormatUptime(char *line, uint16_t size)
{
  uint32_t elapsed = current_tick - run_started_tick;
  uint32_t seconds = elapsed / 1000U;
  uint32_t hours = seconds / 3600U;
  uint32_t minutes = (seconds % 3600U) / 60U;
  seconds %= 60U;
  (void)snprintf(line, size, "%02lu:%02lu:%02lu",
                 (unsigned long)hours, (unsigned long)minutes,
                 (unsigned long)seconds);
}

static void DrawRunHeader(void)
{
  ILI9341_FillRect(0U, 0U, 240U, 32U, UI_PANEL);
  Line(32U);
  ILI9341_DrawText(6U, 8U, "PANVIEW", UI_CYAN, UI_PANEL, 2U);
  Button(180U, 4U, 54U, 24U, "THEME", UI_CYAN, false);
}

static void DrawStatusFocus(const PanViewUiData *data)
{
  const char *text = StateText(data->state);
  uint16_t color = StateColor(data->state);
  uint16_t text_width = (uint16_t)(strlen(text) * 12U);
  uint16_t text_x = text_width < 144U ? (uint16_t)((240U - text_width) / 2U) : 48U;
  ILI9341_FillRect(48U, 198U, 144U, 36U, UI_PANEL);
  ILI9341_FillRect(48U, 198U, 144U, 2U, color);
  ILI9341_FillRect(48U, 232U, 144U, 2U, color);
  ILI9341_FillRect(48U, 198U, 2U, 36U, color);
  ILI9341_FillRect(190U, 198U, 2U, 36U, color);
  ILI9341_DrawText(text_x, 209U, text, color, UI_PANEL, 2U);
}

static void DrawRun(const PanViewUiData *data)
{
  char line[24];
  ILI9341_FillColor(UI_BG);
  DrawRunHeader();
  Panel(8U, 40U, 224U, 100U);
  Label(16U, 50U, "UPTIME", UI_GRAY);
  FormatUptime(line, sizeof(line));
  ILI9341_DrawText(16U, 64U, line, UI_AMBER, UI_PANEL, 2U);
  Label(16U, 96U, "TARGET X", UI_GRAY);
  (void)snprintf(line, sizeof(line), "%+04d", data->error_x);
  Label(132U, 96U, line, UI_CYAN);
  Label(16U, 114U, "TARGET Y", UI_GRAY);
  (void)snprintf(line, sizeof(line), "%+04d", data->error_y);
  Label(132U, 114U, line, UI_CYAN);
  DrawStatusFocus(data);
  Button(8U, 242U, 108U, 36U, "START", UI_AMBER, data->running);
  Button(124U, 242U, 108U, 36U, "STOP", UI_GRAY, !data->running);
  ILI9341_DrawText(72U, 300U, "DESIGNED BY JZX", UI_GRAY, UI_BG, 1U);
}

static void DrawRunDynamic(const PanViewUiData *data)
{
  char line[24];
  ILI9341_FillRect(9U, 41U, 222U, 98U, UI_PANEL);
  Label(16U, 50U, "UPTIME", UI_GRAY);
  FormatUptime(line, sizeof(line));
  ILI9341_DrawText(16U, 64U, line, UI_AMBER, UI_PANEL, 2U);
  Label(16U, 96U, "TARGET X", UI_GRAY);
  (void)snprintf(line, sizeof(line), "%+04d", data->error_x);
  Label(132U, 96U, line, UI_CYAN);
  Label(16U, 114U, "TARGET Y", UI_GRAY);
  (void)snprintf(line, sizeof(line), "%+04d", data->error_y);
  Label(132U, 114U, line, UI_CYAN);
  DrawStatusFocus(data);
  Button(8U, 242U, 108U, 36U, "START", UI_AMBER, data->running);
  Button(124U, 242U, 108U, 36U, "STOP", UI_GRAY, !data->running);
}

static void DrawSettings(void)
{
  ILI9341_FillColor(UI_BG);
  ILI9341_FillRect(0U, 0U, 240U, 32U, UI_PANEL);
  Line(32U);
  ILI9341_DrawText(6U, 8U, "SETTINGS", UI_CYAN, UI_PANEL, 2U);
  Button(8U, 56U, 104U, 32U, "BLACK GOLD", UI_CYAN, theme == UI_THEME_BLACK_GOLD);
  Button(128U, 56U, 104U, 32U, "NEON", UI_CYAN, theme == UI_THEME_NEON);
  Button(8U, 96U, 104U, 32U, "MONO", UI_CYAN, theme == UI_THEME_MONO);
  Button(128U, 96U, 104U, 32U, "GREEN", UI_CYAN, theme == UI_THEME_FLUORESCENT_GREEN);
  Button(8U, 232U, 224U, 38U, "BACK", UI_GRAY, false);
}

static void DrawDiagnostic(const PanViewUiData *data)
{
  char line[42];
  ILI9341_FillColor(UI_BG);
  ILI9341_FillRect(0U, 0U, 240U, 32U, UI_PANEL);
  Line(32U);
  ILI9341_DrawText(6U, 8U, "DIAGNOSTIC", UI_CYAN, UI_PANEL, 2U);
  (void)snprintf(line, sizeof(line), "RX %lu DROP %lu ERR %lu",
                 (unsigned long)data->rx_frames,
                 (unsigned long)data->rx_drops,
                 (unsigned long)data->parse_errors);
  Label(6U, 42U, line, UI_CYAN);
  (void)snprintf(line, sizeof(line), "PAN %+06ld PITCH %+06ld",
                 (long)data->pan_position, (long)data->pitch_position);
  Label(6U, 66U, line, UI_CYAN);
  (void)snprintf(line, sizeof(line), "LIMIT PAN %s PITCH %s",
                 data->pan_limit ? "HIT" : "OK",
                 data->pitch_limit ? "HIT" : "OK");
  Label(6U, 90U, line, (data->pan_limit || data->pitch_limit) ? UI_RED : UI_GREEN);
  Button(8U, 232U, 224U, 38U, "BACK", UI_GRAY, false);
}

void PanViewUi_Init(UiLanguage initial_language)
{
  page = UI_PAGE_RUN;
  language = initial_language;
  theme = UI_THEME_BLACK_GOLD;
  drawn = false;
  last_refresh = 0U;
  current_tick = 0U;
  run_started_tick = 0U;
  run_active_last = false;
  last_page = UI_PAGE_RUN;
  last_state = UI_STATE_SEARCH;
}

void PanViewUi_SetLanguage(UiLanguage selected)
{
  language = selected;
  drawn = false;
}

UiLanguage PanViewUi_GetLanguage(void) { return language; }
UiPage PanViewUi_GetPage(void) { return page; }

void PanViewUi_SetTheme(UiTheme selected)
{
  theme = selected;
  drawn = false;
}

UiTheme PanViewUi_GetTheme(void) { return theme; }

void PanViewUi_Draw(const PanViewUiData *data)
{
  if (data == NULL) return;
  if (page == UI_PAGE_RUN) DrawRun(data);
  else if (page == UI_PAGE_SETTINGS) DrawSettings();
  else DrawDiagnostic(data);
  drawn = true;
  last_state = data->state;
  last_page = page;
}

void PanViewUi_Refresh(const PanViewUiData *data, uint32_t tick)
{
  if (data == NULL) return;
  current_tick = tick;
  if (data->running && !run_active_last)
  {
    run_started_tick = tick;
  }
  run_active_last = data->running;
  if (!drawn || page != last_page)
  {
    PanViewUi_Draw(data);
    last_refresh = tick;
    return;
  }
  if ((page == UI_PAGE_RUN) && (data->state != last_state))
  {
    DrawRunHeader();
    DrawStatusFocus(data);
    last_state = data->state;
    last_refresh = tick;
    return;
  }
  if ((page == UI_PAGE_RUN) && ((tick - last_refresh) >= 500U))
  {
    DrawRunDynamic(data);
    last_refresh = tick;
  }
}

UiEvent PanViewUi_HandleTouch(uint16_t x, uint16_t y, bool pressed)
{
  if (!pressed) return UI_EVENT_NONE;
  if (page == UI_PAGE_RUN)
  {
    if ((y < 32U) && (x >= 176U)) return UI_EVENT_THEME_NEXT;
    if ((y >= 238U) && (y < 282U))
      return x < 120U ? UI_EVENT_START : UI_EVENT_STOP;
  }
  else if (page == UI_PAGE_SETTINGS)
  {
    if ((y >= 50U) && (y < 92U))
      return x < 120U ? UI_EVENT_THEME_BLACK_GOLD : UI_EVENT_THEME_NEON;
    if ((y >= 92U) && (y < 140U))
      return x < 120U ? UI_EVENT_THEME_MONO : UI_EVENT_THEME_FLUORESCENT_GREEN;
    if (y >= 220U) return UI_EVENT_BACK;
  }
  else if (page == UI_PAGE_DIAGNOSTIC && y >= 220U)
  {
    return UI_EVENT_BACK;
  }
  return UI_EVENT_NONE;
}

void PanViewUi_Navigate(UiPage next_page)
{
  page = next_page;
  drawn = false;
}
