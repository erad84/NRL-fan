#include "nrl.h"
#include "logos.h"

#include <time.h>

#if defined(PBL_PLATFORM_GABBRO)
#define LIST_CELL_HEIGHT 64
#define PIN_CELL_HEIGHT 80
#define ROW_PAD 18
#elif defined(PBL_ROUND)
#define LIST_CELL_HEIGHT 52
#define PIN_CELL_HEIGHT 68
#define ROW_PAD 10
#else
#define LIST_CELL_HEIGHT 52
#define PIN_CELL_HEIGHT 56
#define ROW_PAD 2
#endif
#define STATS_CELL_HEIGHT 28
#define LOGO_GAP 2
#define DISCLOSE_W 10
#define PIN_TIMEOUT_MS 20000

typedef struct {
  const char *code;
  const char *nick;
} Club;

static const Club NRL_CLUBS[] = {
  {"BRO", "Broncos"},
  {"BUL", "Bulldogs"},
  {"COW", "Cowboys"},
  {"DOL", "Dolphins"},
  {"DRA", "Dragons"},
  {"EEL", "Eels"},
  {"KNI", "Knights"},
  {"PAN", "Panthers"},
  {"SOU", "Rabbitohs"},
  {"CAN", "Raiders"},
  {"SYD", "Roosters"},
  {"MAN", "Sea Eagles"},
  {"CRO", "Sharks"},
  {"MEL", "Storm"},
  {"GLD", "Titans"},
  {"WAR", "Warriors"},
  {"WST", "Wests Tigers"}
};

static const Club NRLW_CLUBS[] = {
  {"BRO", "Broncos"},
  {"CAN", "Raiders"},
  {"BUL", "Bulldogs"},
  {"CRO", "Sharks"},
  {"GLD", "Titans"},
  {"KNI", "Knights"},
  {"WAR", "Warriors"},
  {"COW", "Cowboys"},
  {"EEL", "Eels"},
  {"DRA", "Dragons"},
  {"SYD", "Roosters"},
  {"WST", "Wests Tigers"}
};

static const Club ORIGIN_CLUBS[] = {
  {"NSW", "NSW"},
  {"QLD", "QLD"}
};

static Window *s_list_window;
static MenuLayer *s_list_menu;
static Window *s_detail_window;
static MenuLayer *s_detail_menu;
static Window *s_year_window;
static MenuLayer *s_year_menu;
static Window *s_team_window;
static MenuLayer *s_team_menu;
static Window *s_comp_window;
static MenuLayer *s_comp_menu;
static AppTimer *s_live_timer;
static ListData s_list;
static ListData s_detail;
static char s_list_name[NRL_NAME_LEN];
static char s_detail_name[NRL_NAME_LEN];
static char s_hist_title[NRL_NAME_LEN];
static int s_list_req;
static int s_req_year;
static char s_req_team[NRL_TEAM_LEN];
static int s_hist_year;
static char s_hist_team[NRL_TEAM_LEN];
static int s_years[NRL_YEAR_COUNT];
static Window *s_live_window;
static Layer *s_live_layer;
static char s_live_line[NRL_LINE_LEN];
static char s_live_home[4];
static char s_live_away[4];
static char s_live_hs[6];
static char s_live_as[6];
static char s_live_state[8];
static Window *s_pin_window;
static MenuLayer *s_pin_menu;
static ActionBarLayer *s_pin_bar;
static GBitmap *s_pin_check;
static uint16_t s_pin_row;
static bool s_pin_is_bye;
static bool s_pin_waiting;
static bool s_pin_done;
static bool s_pin_ok;
static AppTimer *s_pin_timeout;
static char s_pin_header[NRL_TITLE_LEN];
static char s_pin_status[NRL_TITLE_LEN];
static char s_pin_error[20];

static const Club *clubs_for_comp(int *count) {
  int comp = persist_get_comp();
  if (comp == COMP_NRLW) {
    *count = (int)(sizeof(NRLW_CLUBS) / sizeof(NRLW_CLUBS[0]));
    return NRLW_CLUBS;
  }
  if (comp == COMP_ORIGIN_MEN || comp == COMP_ORIGIN_WOMEN) {
    *count = (int)(sizeof(ORIGIN_CLUBS) / sizeof(ORIGIN_CLUBS[0]));
    return ORIGIN_CLUBS;
  }
  *count = (int)(sizeof(NRL_CLUBS) / sizeof(NRL_CLUBS[0]));
  return NRL_CLUBS;
}

static const char *club_animal(const char *nick) {
  if (!nick || !nick[0]) {
    return "Team";
  }
  if (strcmp(nick, "Wests Tigers") == 0) {
    return "Tigers";
  }
  if (strcmp(nick, "NSW") == 0) {
    return "Blues";
  }
  if (strcmp(nick, "QLD") == 0) {
    return "Maroons";
  }
  return nick;
}

static const char *nick_from_code(const char *code) {
  int count = 0;
  const Club *clubs = clubs_for_comp(&count);
  for (int i = 0; i < count; i++) {
    if (strcmp(clubs[i].code, code) == 0) {
      return clubs[i].nick;
    }
  }
  return code;
}

const char *club_code_for_nick(const char *nick) {
  if (!nick || !nick[0]) {
    return "";
  }
  const Club *tables[] = { NRL_CLUBS, NRLW_CLUBS, ORIGIN_CLUBS };
  const int counts[] = {
    (int)(sizeof(NRL_CLUBS) / sizeof(NRL_CLUBS[0])),
    (int)(sizeof(NRLW_CLUBS) / sizeof(NRLW_CLUBS[0])),
    (int)(sizeof(ORIGIN_CLUBS) / sizeof(ORIGIN_CLUBS[0]))
  };
  for (int t = 0; t < 3; t++) {
    for (int i = 0; i < counts[t]; i++) {
      if (strcmp(tables[t][i].nick, nick) == 0 ||
          strcmp(club_animal(tables[t][i].nick), nick) == 0) {
        return tables[t][i].code;
      }
    }
  }
  return "";
}

static void fill_years(void) {
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  int year = tm ? (tm->tm_year + 1900) : 2026;
  for (int i = 0; i < NRL_YEAR_COUNT; i++) {
    s_years[i] = year - i;
  }
}

static void split_line(const char *line, char out[][20], int max_parts, int *count) {
  int n = 0;
  int start = 0;
  int i = 0;
  *count = 0;
  while (line[i] != '\0' && n < max_parts) {
    if (line[i] == '|') {
      int len = i - start;
      if (len > 19) {
        len = 19;
      }
      memcpy(out[n], line + start, len);
      out[n][len] = '\0';
      n++;
      start = i + 1;
    }
    i++;
  }
  if (n < max_parts) {
    int len = i - start;
    if (len > 19) {
      len = 19;
    }
    memcpy(out[n], line + start, len);
    out[n][len] = '\0';
    n++;
  }
  *count = n;
}

static bool parse_odds_fixed(const char *s, int *h, int *a) {
  int hi = 0;
  int ai = 0;
  if (!s || *s < '0' || *s > '9') {
    return false;
  }
  while (*s >= '0' && *s <= '9') {
    hi = hi * 10 + (*s - '0');
    s++;
  }
  if (*s != '-') {
    return false;
  }
  s++;
  if (*s < '0' || *s > '9') {
    return false;
  }
  while (*s >= '0' && *s <= '9') {
    ai = ai * 10 + (*s - '0');
    s++;
  }
  if (*s != '\0' || hi < 101 || ai < 101) {
    return false;
  }
  *h = hi;
  *a = ai;
  return true;
}

static bool format_odds_pair(const char *field, char *home, size_t home_len, char *away,
                             size_t away_len) {
  int h = 0;
  int a = 0;
  int hp = 0;
  home[0] = '\0';
  away[0] = '\0';
  if (!parse_odds_fixed(field, &h, &a)) {
    return false;
  }
  if (persist_get_odds_raw()) {
    snprintf(home, home_len, "%d.%02d", h / 100, h % 100);
    snprintf(away, away_len, "%d.%02d", a / 100, a % 100);
  } else {
    hp = (100 * a) / (h + a);
    if (hp < 1) {
      hp = 1;
    }
    if (hp > 99) {
      hp = 99;
    }
    snprintf(home, home_len, "%d%%", hp);
    snprintf(away, away_len, "%d%%", 100 - hp);
  }
  return true;
}

static bool line_is_header(const char *line) {
  return line && strncmp(line, "HDR|", 4) == 0;
}

static bool line_is_bye(const char *line) {
  char p[8][20];
  int n = 0;
  split_line(line, p, 8, &n);
  return n >= 6 && strcmp(p[5], "BYE") == 0;
}

static bool line_is_pinned(const char *line) {
  char p[9][20];
  int n = 0;
  if (!line) {
    return false;
  }
  split_line(line, p, 9, &n);
  return n >= 9 && strcmp(p[8], "1") == 0;
}

static bool finals_list_open(void) {
  return s_list_req == REQ_HISTORY && strncmp(s_list_name, "Finals", 6) == 0;
}

static void format_round_copy(const char *src, char *out, size_t out_len) {
  const bool origin = persist_get_comp() == COMP_ORIGIN_MEN ||
                      persist_get_comp() == COMP_ORIGIN_WOMEN;
  if (!src || !src[0]) {
    strncpy(out, origin ? "Game" : "Round", out_len - 1);
    out[out_len - 1] = '\0';
    return;
  }
  if (origin && strncmp(src, "Round", 5) == 0) {
    snprintf(out, out_len, "Game%s", src + 5);
    return;
  }
  if ((src[0] == 'R' || src[0] == 'r' || src[0] == 'G' || src[0] == 'g') &&
      src[1] >= '0' && src[1] <= '9') {
    snprintf(out, out_len, "%s %s",
             (src[0] == 'G' || src[0] == 'g' || origin) ? "Game" : "Round", src + 1);
    return;
  }
  strncpy(out, src, out_len - 1);
  out[out_len - 1] = '\0';
}

static void format_row(int req, const char *line, char *title, size_t title_len, char *sub,
                       size_t sub_len, char *bot, size_t bot_len, char *home, size_t home_len,
                       char *away, size_t away_len, bool *is_ladder) {
  char p[9][20];
  int n = 0;
  split_line(line, p, 9, &n);

  title[0] = '\0';
  sub[0] = '\0';
  bot[0] = '\0';
  home[0] = '\0';
  away[0] = '\0';
  *is_ladder = false;

  if (line_is_header(line)) {
    strncpy(title, line + 4, title_len - 1);
    title[title_len - 1] = '\0';
    return;
  }

  if (req == REQ_DRAW && n >= 2) {
    strncpy(title, p[1], title_len - 1);
    title[title_len - 1] = '\0';
    return;
  }

  if (n == 2) {
    strncpy(title, p[0], title_len - 1);
    title[title_len - 1] = '\0';
    strncpy(sub, p[1], sub_len - 1);
    sub[sub_len - 1] = '\0';
    return;
  }

  if (req == REQ_LADDER && n >= 8) {
    *is_ladder = true;
    strncpy(home, p[1], home_len - 1);
    home[home_len - 1] = '\0';
    snprintf(title, title_len, "%s  %s   %s", p[0], p[1], p[7]);
    snprintf(sub, sub_len, "%s  %s-%s-%s  %s", p[2], p[3], p[5], p[4], p[6]);
    return;
  }

  if (n >= 7) {
    strncpy(home, p[1], home_len - 1);
    home[home_len - 1] = '\0';
    strncpy(away, p[3], away_len - 1);
    away[away_len - 1] = '\0';
    if (req != REQ_HISTORY && req != REQ_STATS && req != REQ_DRAW_ROUND) {
      persist_set_round(p[0]);
    }

    if (req == REQ_LIVE) {
      if (strcmp(p[5], "BYE") == 0) {
        snprintf(title, title_len, "%s  BYE", p[1]);
        return;
      }
      snprintf(title, title_len, "%s  v  %s", p[1], p[3]);
      if (strcmp(p[2], "-") != 0 || strcmp(p[4], "-") != 0) {
        snprintf(sub, sub_len, "%s-%s", p[2], p[4]);
      } else {
        snprintf(sub, sub_len, "UPCOMING");
      }
      if (strcmp(p[5], "UP") == 0) {
        strncpy(bot, p[6], bot_len - 1);
        bot[bot_len - 1] = '\0';
      } else if (p[6][0] != '\0') {
        snprintf(bot, bot_len, "%s  %s", p[5], p[6]);
      } else {
        strncpy(bot, p[5], bot_len - 1);
        bot[bot_len - 1] = '\0';
      }
      return;
    }

    if (req == REQ_DRAW_ROUND) {
      if (strcmp(p[5], "BYE") == 0) {
        snprintf(title, title_len, "%s  BYE", p[1]);
        return;
      }
      snprintf(title, title_len, "%s  v  %s", p[1], p[3]);
      if (p[6][0]) {
        strncpy(sub, p[6], sub_len - 1);
        sub[sub_len - 1] = '\0';
      }
      if (strcmp(p[2], "-") != 0 || strcmp(p[4], "-") != 0) {
        snprintf(bot, bot_len, "%s-%s", p[2], p[4]);
      } else if (strcmp(p[5], "UP") == 0 && n >= 8 && p[7][0]) {
        char home_odds[8];
        char away_odds[8];
        if (format_odds_pair(p[7], home_odds, sizeof(home_odds), away_odds, sizeof(away_odds))) {
          snprintf(bot, bot_len, "%s Odds %s", home_odds, away_odds);
        }
      } else if (strcmp(p[5], "LIVE") == 0 || strcmp(p[5], "HT") == 0) {
        strncpy(bot, p[5], bot_len - 1);
        bot[bot_len - 1] = '\0';
      }
      return;
    }

    if (req == REQ_RESULTS || req == REQ_HISTORY || req == REQ_UPCOMING) {
      if (finals_list_open()) {
        snprintf(title, title_len, "%s  v  %s", p[1], p[3]);
        if (strcmp(p[2], "-") == 0) {
          snprintf(sub, sub_len, "UPCOMING");
        } else {
          snprintf(sub, sub_len, "%s-%s", p[2], p[4]);
        }
        strncpy(bot, p[6], bot_len - 1);
        bot[bot_len - 1] = '\0';
        return;
      }
      format_round_copy(p[0], title, title_len);
      if (strcmp(p[5], "BYE") == 0) {
        snprintf(sub, sub_len, "BYE");
      } else if (req == REQ_UPCOMING || strcmp(p[2], "-") == 0) {
        snprintf(sub, sub_len, "%s  v  %s", p[1], p[3]);
      } else {
        snprintf(sub, sub_len, "%s-%s", p[2], p[4]);
      }
      strncpy(bot, p[6], bot_len - 1);
      bot[bot_len - 1] = '\0';
      return;
    }

    if (strcmp(p[5], "BYE") == 0) {
      snprintf(title, title_len, "%s  BYE", p[1]);
      format_round_copy(p[0], sub, sub_len);
      return;
    }
    if (strcmp(p[2], "-") == 0) {
      snprintf(title, title_len, "%s  v  %s", p[1], p[3]);
    } else {
      snprintf(title, title_len, "%s  %s-%s  %s", p[1], p[2], p[4], p[3]);
    }
    if (p[6][0] != '\0') {
      snprintf(sub, sub_len, "%s  %s  %s", p[0], p[5], p[6]);
    } else {
      snprintf(sub, sub_len, "%s  %s", p[0], p[5]);
    }
    return;
  }

  snprintf(title, title_len, "%s", line);
}

static void cancel_live_timer(void) {
  if (s_live_timer) {
    app_timer_cancel(s_live_timer);
    s_live_timer = NULL;
  }
}

static void live_tick(void *data) {
  (void)data;
  s_live_timer = NULL;
  const bool watching = s_live_window && window_stack_contains_window(s_live_window);
  if (s_list_req == REQ_LIVE &&
      (window_stack_contains_window(s_list_window) || watching)) {
    comm_request(REQ_LIVE, persist_get_comp());
    s_live_timer = app_timer_register(watching ? 30000 : 60000, live_tick, NULL);
  }
}

static void start_live_timer(void) {
  cancel_live_timer();
  if (s_list_req == REQ_LIVE) {
    const bool watching = s_live_window && window_stack_contains_window(s_live_window);
    s_live_timer = app_timer_register(watching ? 30000 : 60000, live_tick, NULL);
  }
}

static void draw_list_row(GContext *ctx, const Layer *cell_layer, ListData *list, int req,
                          uint16_t row) {
  if (list->status == STATUS_LOADING) {
    menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
    return;
  }
  if (list->status == STATUS_ERROR) {
    menu_cell_basic_draw(ctx, cell_layer, "Can't load",
                         list->error[0] ? list->error : "Try again", NULL);
    return;
  }
  if (list->count == 0) {
    menu_cell_basic_draw(ctx, cell_layer, req == REQ_DRAW ? "No rounds" : "No games", NULL, NULL);
    return;
  }

  char title[64];
  char sub[64];
  char bot[32];
  char home[8];
  char away[8];
  bool is_ladder = false;
  format_row(req, list->lines[row], title, sizeof(title), sub, sizeof(sub),
             bot, sizeof(bot), home, sizeof(home), away, sizeof(away), &is_ladder);

  GRect bounds = layer_get_bounds(cell_layer);
  const bool highlight = menu_cell_layer_is_highlighted(cell_layer);
#if defined(PBL_COLOR)
  graphics_context_set_text_color(ctx, GColorWhite);
#else
  graphics_context_set_text_color(ctx, highlight ? GColorWhite : GColorBlack);
#endif
  if (line_is_header(list->lines[row])) {
    graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(bounds.origin.x + 4, bounds.origin.y + 4,
                             bounds.size.w - 8, bounds.size.h - 6),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }

  GBitmap *home_bmp = logo_for_code(home);
  GBitmap *away_bmp = is_ladder ? NULL : logo_for_code(away);
  const int icon = bounds.size.h - (LOGO_GAP * 2);
  const int pad = ROW_PAD;
  const bool disclose = (req == REQ_UPCOMING || req == REQ_DRAW);
  const bool pinned = disclose && line_is_pinned(list->lines[row]);
  int left = bounds.origin.x + pad;
  int right = bounds.origin.x + bounds.size.w - pad;
  if (disclose) {
    right -= DISCLOSE_W;
  }
  if (home_bmp) {
    draw_logo(ctx, home_bmp, GRect(left, bounds.origin.y + LOGO_GAP, icon, icon), highlight);
    left += icon + 2;
  }
  if (away_bmp) {
    right -= icon;
    draw_logo(ctx, away_bmp, GRect(right, bounds.origin.y + LOGO_GAP, icon, icon), highlight);
    right -= 2;
  }

  GRect text_box = GRect(left, bounds.origin.y, right - left, bounds.size.h);
  const GTextAlignment align = (home_bmp && away_bmp) ? GTextAlignmentCenter : GTextAlignmentLeft;
  const GFont title_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  const GFont sub_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  const GFont mid_font = (req == REQ_DRAW_ROUND)
      ? fonts_get_system_font(FONT_KEY_GOTHIC_18)
      : title_font;
  if (bot[0]) {
    graphics_draw_text(ctx, title, title_font,
                       GRect(text_box.origin.x, text_box.origin.y - 2, text_box.size.w, 22),
                       GTextOverflowModeTrailingEllipsis, align, NULL);
    graphics_draw_text(ctx, sub, mid_font,
                       GRect(text_box.origin.x, text_box.origin.y + 16, text_box.size.w, 22),
                       GTextOverflowModeTrailingEllipsis, align, NULL);
    graphics_draw_text(ctx, bot, sub_font,
                       GRect(text_box.origin.x, text_box.origin.y + 36, text_box.size.w, 16),
                       GTextOverflowModeTrailingEllipsis, align, NULL);
  } else if (sub[0]) {
    graphics_draw_text(ctx, title, title_font,
                       GRect(text_box.origin.x, text_box.origin.y + 4, text_box.size.w, 22),
                       GTextOverflowModeTrailingEllipsis, align, NULL);
    graphics_draw_text(ctx, sub, sub_font,
                       GRect(text_box.origin.x, text_box.origin.y + 26, text_box.size.w, 18),
                       GTextOverflowModeTrailingEllipsis, align, NULL);
  } else {
    graphics_draw_text(ctx, title, title_font, text_box,
                       GTextOverflowModeTrailingEllipsis, align, NULL);
  }

  if (disclose) {
#if defined(PBL_COLOR)
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_text_color(ctx, GColorWhite);
#else
    graphics_context_set_stroke_color(ctx, highlight ? GColorWhite : GColorBlack);
    graphics_context_set_text_color(ctx, highlight ? GColorWhite : GColorBlack);
#endif
    if (pinned) {
      const int x = bounds.origin.x + bounds.size.w - 12;
      const int y = bounds.origin.y + bounds.size.h / 2;
      graphics_draw_line(ctx, GPoint(x, y), GPoint(x + 3, y + 5));
      graphics_draw_line(ctx, GPoint(x + 1, y), GPoint(x + 4, y + 5));
      graphics_draw_line(ctx, GPoint(x + 3, y + 5), GPoint(x + 10, y - 5));
      graphics_draw_line(ctx, GPoint(x + 4, y + 5), GPoint(x + 11, y - 5));
    } else {
      graphics_draw_text(ctx, ">", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                         GRect(bounds.origin.x + bounds.size.w - 12,
                               bounds.origin.y + bounds.size.h / 2 - 12, 12, 24),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    }
  }
}

static uint16_t list_num_rows(MenuLayer *layer, uint16_t section, void *context) {
  (void)layer;
  (void)section;
  (void)context;
  if (s_list.status != STATUS_OK || s_list.count == 0) {
    return 1;
  }
  return s_list.count;
}

static int16_t list_cell_height(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)context;
  if (s_list.status == STATUS_OK && index->row < s_list.count &&
      line_is_header(s_list.lines[index->row])) {
    return 28;
  }
  if (s_list_req == REQ_DRAW) {
    return 44;
  }
  return LIST_CELL_HEIGHT;
}

static void list_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *index, void *context) {
  (void)context;
  draw_list_row(ctx, cell_layer, &s_list, s_list_req, index->row);
}

static int16_t list_header_height(MenuLayer *layer, uint16_t section, void *context) {
  (void)layer;
  (void)section;
  (void)context;
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void list_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section, void *context) {
  (void)section;
  (void)context;
  const char *header = s_list.title[0] ? s_list.title : s_list_name;
  menu_cell_basic_header_draw(ctx, cell_layer, header);
}

static void screens_show_stats(const char *code);
static void screens_show_draw_round(const char *value, const char *label);

static void screens_show_live_game(uint16_t row);
static void live_game_refresh(void);
static void screens_show_pin_actions(uint16_t row);

static void list_select(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)context;
  if (s_list.status == STATUS_OK && index->row < s_list.count &&
      line_is_header(s_list.lines[index->row])) {
    return;
  }
  if (s_list_req == REQ_LIVE && s_list.status == STATUS_OK && s_list.count > 0) {
    screens_show_live_game(index->row);
    return;
  }
  if (s_list_req == REQ_DRAW && s_list.status == STATUS_OK && s_list.count > 0) {
    char p[8][20];
    int n = 0;
    split_line(s_list.lines[index->row], p, 8, &n);
    if (n >= 2 && p[0][0]) {
      screens_show_draw_round(p[0], p[1]);
    }
    return;
  }
  if (s_list_req == REQ_UPCOMING && s_list.status == STATUS_OK && s_list.count > 0) {
    screens_show_pin_actions(index->row);
    return;
  }
  if (s_list_req == REQ_LADDER && s_list.status == STATUS_OK && s_list.count > 0) {
    char p[8][20];
    int n = 0;
    split_line(s_list.lines[index->row], p, 8, &n);
    if (n >= 2) {
      screens_show_stats(p[1]);
    }
    return;
  }
  s_list.status = STATUS_LOADING;
  s_list.count = 0;
  menu_layer_reload_data(s_list_menu);
  comm_request_ex(s_list_req, persist_get_comp(), s_req_year, s_req_team[0] ? s_req_team : NULL);
}

static void style_menu(MenuLayer *menu, Window *window) {
#if defined(PBL_ROUND)
  menu_layer_set_center_focused(menu, true);
#endif
#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(menu, GColorBlack, GColorWhite);
  menu_layer_set_highlight_colors(menu, GColorDarkGreen, GColorWhite);
  window_set_background_color(window, GColorBlack);
#endif
}

static void list_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_list_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_list_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = list_num_rows,
    .get_cell_height = list_cell_height,
    .draw_row = list_draw_row,
    .get_header_height = list_header_height,
    .draw_header = list_draw_header,
    .select_click = list_select,
  });
  menu_layer_set_click_config_onto_window(s_list_menu, window);
  style_menu(s_list_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_list_menu));
}

static void list_unload(Window *window) {
  (void)window;
  cancel_live_timer();
  menu_layer_destroy(s_list_menu);
  s_list_menu = NULL;
}

static uint16_t detail_num_rows(MenuLayer *layer, uint16_t section, void *context) {
  (void)layer;
  (void)section;
  (void)context;
  if (s_detail.status != STATUS_OK || s_detail.count == 0) {
    return 1;
  }
  return s_detail.count;
}

static int16_t detail_cell_height(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)index;
  (void)context;
  if (s_detail.pending_req == REQ_DRAW_ROUND) {
    return LIST_CELL_HEIGHT;
  }
  if (s_detail.status != STATUS_OK || s_detail.count == 0) {
    return LIST_CELL_HEIGHT;
  }
  return STATS_CELL_HEIGHT;
}

static void detail_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *index, void *context) {
  (void)context;
  if (s_detail.pending_req == REQ_DRAW_ROUND) {
    draw_list_row(ctx, cell_layer, &s_detail, REQ_DRAW_ROUND, index->row);
    return;
  }
  if (s_detail.status != STATUS_OK || s_detail.count == 0) {
    draw_list_row(ctx, cell_layer, &s_detail, REQ_STATS, index->row);
    return;
  }
  char title[64];
  char sub[64];
  char bot[32];
  char home[8];
  char away[8];
  bool is_ladder = false;
  format_row(REQ_STATS, s_detail.lines[index->row], title, sizeof(title), sub, sizeof(sub),
             bot, sizeof(bot), home, sizeof(home), away, sizeof(away), &is_ladder);
  GRect bounds = layer_get_bounds(cell_layer);
  const bool highlight = menu_cell_layer_is_highlighted(cell_layer);
#if defined(PBL_COLOR)
  (void)highlight;
  graphics_context_set_text_color(ctx, GColorWhite);
#else
  graphics_context_set_text_color(ctx, highlight ? GColorWhite : GColorBlack);
#endif
  const int pad = 4;
  graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                     GRect(bounds.origin.x + pad, bounds.origin.y + 2, bounds.size.w - 56, 24),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, sub, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(bounds.origin.x + bounds.size.w - 52, bounds.origin.y + 2, 48, 24),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
}

static void detail_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section, void *context) {
  (void)section;
  (void)context;
  const char *header = s_detail.title[0] ? s_detail.title : s_detail_name;
  menu_cell_basic_header_draw(ctx, cell_layer, header);
}

static void detail_select(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)index;
  (void)context;
  if (s_detail.pending_req == REQ_DRAW_ROUND) {
    return;
  }
  s_detail.status = STATUS_LOADING;
  s_detail.count = 0;
  menu_layer_reload_data(s_detail_menu);
  comm_request_ex(REQ_STATS, persist_get_comp(), 0, s_req_team);
}

static void detail_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_detail_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_detail_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = detail_num_rows,
    .get_cell_height = detail_cell_height,
    .draw_row = detail_draw_row,
    .get_header_height = list_header_height,
    .draw_header = detail_draw_header,
    .select_click = detail_select,
  });
  menu_layer_set_click_config_onto_window(s_detail_menu, window);
  style_menu(s_detail_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_detail_menu));
}

static void detail_unload(Window *window) {
  (void)window;
  menu_layer_destroy(s_detail_menu);
  s_detail_menu = NULL;
}

static void screens_show_stats(const char *code) {
  const char *nick = nick_from_code(code);
  snprintf(s_detail_name, sizeof(s_detail_name), "%s", club_animal(nick));
  strncpy(s_req_team, nick, sizeof(s_req_team) - 1);
  s_req_team[sizeof(s_req_team) - 1] = '\0';
  s_detail.pending_req = REQ_STATS;
  s_detail.status = STATUS_LOADING;
  s_detail.count = 0;
  s_detail.title[0] = '\0';
  s_detail.error[0] = '\0';
  if (!window_stack_contains_window(s_detail_window)) {
    window_stack_push(s_detail_window, true);
  } else if (s_detail_menu) {
    menu_layer_reload_data(s_detail_menu);
  }
  comm_request_ex(REQ_STATS, persist_get_comp(), 0, s_req_team);
}

static void screens_show_draw_round(const char *value, const char *label) {
  snprintf(s_detail_name, sizeof(s_detail_name), "%s", label && label[0] ? label : "Round");
  strncpy(s_req_team, value, sizeof(s_req_team) - 1);
  s_req_team[sizeof(s_req_team) - 1] = '\0';
  s_detail.pending_req = REQ_DRAW_ROUND;
  s_detail.status = STATUS_LOADING;
  s_detail.count = 0;
  s_detail.title[0] = '\0';
  s_detail.error[0] = '\0';
  if (!window_stack_contains_window(s_detail_window)) {
    window_stack_push(s_detail_window, true);
  } else if (s_detail_menu) {
    menu_layer_reload_data(s_detail_menu);
  }
  comm_request_ex(REQ_DRAW_ROUND, persist_get_comp(), 0, s_req_team);
}

static uint16_t year_num_rows(MenuLayer *layer, uint16_t section, void *context) {
  (void)layer;
  (void)section;
  (void)context;
  return NRL_YEAR_COUNT;
}

static void year_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *index, void *context) {
  (void)context;
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", s_years[index->row]);
  menu_cell_basic_draw(ctx, cell_layer, buf, NULL, NULL);
}

static void screens_show_team_menu(void);

static void year_select(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)context;
  s_hist_year = s_years[index->row];
  screens_show_team_menu();
}

static void year_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_year_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_year_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = year_num_rows,
    .draw_row = year_draw_row,
    .select_click = year_select,
  });
  menu_layer_set_click_config_onto_window(s_year_menu, window);
  style_menu(s_year_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_year_menu));
}

static void year_unload(Window *window) {
  (void)window;
  menu_layer_destroy(s_year_menu);
  s_year_menu = NULL;
}

static uint16_t team_num_rows(MenuLayer *layer, uint16_t section, void *context) {
  (void)layer;
  (void)section;
  (void)context;
  int count = 0;
  clubs_for_comp(&count);
  return (uint16_t)(count + 1);
}

static int16_t team_cell_height(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)index;
  (void)context;
  return LIST_CELL_HEIGHT;
}

static void team_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *index, void *context) {
  (void)context;
  int count = 0;
  const Club *clubs = clubs_for_comp(&count);
  GRect bounds = layer_get_bounds(cell_layer);
  const bool highlight = menu_cell_layer_is_highlighted(cell_layer);
  const int icon = bounds.size.h - (LOGO_GAP * 2);
  GBitmap *bmp = NULL;
  const char *label = NULL;
  if (index->row == 0) {
    bmp = logo_trophy();
    label = "Finals";
  } else {
    const Club *club = &clubs[index->row - 1];
    bmp = logo_for_code(club->code);
    label = club_animal(club->nick);
  }
  int left = bounds.origin.x + ROW_PAD;
  if (bmp) {
    draw_logo(ctx, bmp, GRect(left, bounds.origin.y + LOGO_GAP, icon, icon), highlight);
    left += icon + 4;
  }
#if defined(PBL_COLOR)
  graphics_context_set_text_color(ctx, GColorWhite);
#else
  graphics_context_set_text_color(ctx, highlight ? GColorWhite : GColorBlack);
#endif
  graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(left, bounds.origin.y + 12, bounds.size.w - left - ROW_PAD, 24),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void screens_show_list_ex(const char *name, int req, int year, const char *team);

static void team_select(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)context;
  if (index->row == 0) {
    strncpy(s_hist_team, "Finals", sizeof(s_hist_team) - 1);
    s_hist_team[sizeof(s_hist_team) - 1] = '\0';
    snprintf(s_hist_title, sizeof(s_hist_title), "Finals %d", s_hist_year);
    screens_show_list_ex(s_hist_title, REQ_HISTORY, s_hist_year, s_hist_team);
    return;
  }
  int count = 0;
  const Club *clubs = clubs_for_comp(&count);
  const Club *club = &clubs[index->row - 1];
  strncpy(s_hist_team, club->nick, sizeof(s_hist_team) - 1);
  s_hist_team[sizeof(s_hist_team) - 1] = '\0';
  snprintf(s_hist_title, sizeof(s_hist_title), "%s %d", club_animal(club->nick), s_hist_year);
  screens_show_list_ex(s_hist_title, REQ_HISTORY, s_hist_year, s_hist_team);
}

static void team_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_team_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_team_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = team_num_rows,
    .get_cell_height = team_cell_height,
    .draw_row = team_draw_row,
    .select_click = team_select,
  });
  menu_layer_set_click_config_onto_window(s_team_menu, window);
  style_menu(s_team_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_team_menu));
}

static void team_unload(Window *window) {
  (void)window;
  menu_layer_destroy(s_team_menu);
  s_team_menu = NULL;
}

static void screens_show_team_menu(void) {
  window_stack_push(s_team_window, true);
}

static uint16_t comp_num_rows(MenuLayer *layer, uint16_t section, void *context) {
  (void)layer;
  (void)section;
  (void)context;
  return COMP_COUNT;
}

static int16_t comp_cell_height(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)index;
  (void)context;
  return LIST_CELL_HEIGHT;
}

static void comp_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *index, void *context) {
  (void)context;
  const char *title = comp_label((int)index->row);
  const char *sub = (persist_get_comp() == (int)index->row) ? "Selected" : NULL;
  GRect bounds = layer_get_bounds(cell_layer);
  const bool highlight = menu_cell_layer_is_highlighted(cell_layer);
  const int icon = bounds.size.h - (LOGO_GAP * 2);
  GBitmap *bmp = logo_for_comp((int)index->row);
  int left = bounds.origin.x + ROW_PAD;
  if (bmp) {
    draw_logo(ctx, bmp, GRect(left, bounds.origin.y + LOGO_GAP, icon, icon), highlight);
    left += icon + 4;
  }
#if defined(PBL_COLOR)
  graphics_context_set_text_color(ctx, GColorWhite);
#else
  graphics_context_set_text_color(ctx, highlight ? GColorWhite : GColorBlack);
#endif
  graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(left, bounds.origin.y + 4, bounds.size.w - left - ROW_PAD, 22),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (sub) {
    graphics_draw_text(ctx, sub, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(left, bounds.origin.y + 26, bounds.size.w - left - ROW_PAD, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
}

static void comp_select(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)context;
  persist_set_comp((int)index->row);
  window_stack_pop(true);
}

static void comp_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_comp_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_comp_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = comp_num_rows,
    .get_cell_height = comp_cell_height,
    .draw_row = comp_draw_row,
    .select_click = comp_select,
  });
  menu_layer_set_click_config_onto_window(s_comp_menu, window);
  style_menu(s_comp_menu, window);
  menu_layer_set_selected_index(s_comp_menu, (MenuIndex){ .section = 0, .row = persist_get_comp() },
                                MenuRowAlignCenter, false);
  layer_add_child(root, menu_layer_get_layer(s_comp_menu));
}

static void comp_unload(Window *window) {
  (void)window;
  menu_layer_destroy(s_comp_menu);
  s_comp_menu = NULL;
}

static void parse_match_codes(const char *line, char *home, char *away, char *hs, char *as) {
  char p[8][20];
  int n = 0;
  split_line(line, p, 8, &n);
  home[0] = '\0';
  away[0] = '\0';
  hs[0] = '\0';
  as[0] = '\0';
  if (n >= 5) {
    strncpy(home, p[1], 7);
    home[7] = '\0';
    strncpy(hs, p[2], 7);
    hs[7] = '\0';
    strncpy(away, p[3], 7);
    away[7] = '\0';
    strncpy(as, p[4], 7);
    as[7] = '\0';
  }
}

static void live_game_apply_line(const char *line, bool vibrate) {
  char p[8][20];
  int n = 0;
  char home[8];
  char away[8];
  char hs[8];
  char as[8];
  split_line(line, p, 8, &n);
  parse_match_codes(line, home, away, hs, as);
  const char *state = (n >= 6) ? p[5] : "";
  if (vibrate && persist_get_vibe()) {
    const bool score_changed =
        (hs[0] && strcmp(hs, s_live_hs) != 0) || (as[0] && strcmp(as, s_live_as) != 0);
    const bool started = strcmp(s_live_state, "UP") == 0 &&
                         (strcmp(state, "LIVE") == 0 || strcmp(state, "HT") == 0);
    const bool fulltime = strcmp(state, "FT") == 0 && strcmp(s_live_state, "FT") != 0 &&
                          s_live_state[0] != '\0';
    if (score_changed || started || fulltime) {
      vibes_short_pulse();
    }
  }
  strncpy(s_live_line, line, sizeof(s_live_line) - 1);
  s_live_line[sizeof(s_live_line) - 1] = '\0';
  strncpy(s_live_home, home, sizeof(s_live_home) - 1);
  strncpy(s_live_away, away, sizeof(s_live_away) - 1);
  strncpy(s_live_hs, hs, sizeof(s_live_hs) - 1);
  strncpy(s_live_as, as, sizeof(s_live_as) - 1);
  strncpy(s_live_state, state, sizeof(s_live_state) - 1);
  s_live_state[sizeof(s_live_state) - 1] = '\0';
  if (s_live_layer) {
    layer_mark_dirty(s_live_layer);
  }
}

static void live_game_refresh(void) {
  if (!s_live_window || !window_stack_contains_window(s_live_window)) {
    return;
  }
  for (int i = 0; i < s_list.count; i++) {
    char home[8];
    char away[8];
    char hs[8];
    char as[8];
    parse_match_codes(s_list.lines[i], home, away, hs, as);
    if (strcmp(home, s_live_home) == 0 && strcmp(away, s_live_away) == 0) {
      live_game_apply_line(s_list.lines[i], true);
      return;
    }
  }
  /* Keep the card open. If this match dropped off a finished round list, show FT. */
  if (s_list.status != STATUS_OK || s_list.count == 0) {
    return;
  }
  if (strcmp(s_live_state, "LIVE") == 0 || strcmp(s_live_state, "HT") == 0) {
    char p[8][20];
    int n = 0;
    char line[NRL_LINE_LEN];
    split_line(s_live_line, p, 8, &n);
    if (n >= 6) {
      snprintf(line, sizeof(line), "%s|%s|%s|%s|%s|FT|%s", p[0], p[1], p[2], p[3], p[4],
               n >= 7 ? p[6] : "");
      live_game_apply_line(line, true);
    }
  }
}

static void live_layer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  char p[9][20];
  int n = 0;
  split_line(s_live_line, p, 9, &n);
#if defined(PBL_COLOR)
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
#else
  graphics_context_set_text_color(ctx, GColorBlack);
#endif

#if defined(PBL_PLATFORM_GABBRO)
  const int inset = 28;
  const int logo = 64;
  const int name_w = 72;
#elif defined(PBL_ROUND)
  const int inset = 20;
  const int logo = 42;
  const int name_w = 48;
#elif defined(PBL_PLATFORM_APLITE)
  const int inset = 4;
  const int logo = 36;
  const int name_w = 48;
#else
  const int inset = 4;
  const int logo = 48;
  const int name_w = 56;
#endif
  char round[20];
  format_round_copy(n > 0 ? p[0] : "", round, sizeof(round));
  graphics_draw_text(ctx, round, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(inset, bounds.origin.y + PBL_IF_ROUND_ELSE(8, 2),
                           bounds.size.w - inset * 2, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  GBitmap *home_bmp = logo_for_code(s_live_home);
  GBitmap *away_bmp = logo_for_code(s_live_away);
  const int y_logo = bounds.origin.y + PBL_IF_ROUND_ELSE(28, 20);
  if (home_bmp) {
    draw_logo(ctx, home_bmp, GRect(inset, y_logo, logo, logo), false);
  }
  if (away_bmp) {
    draw_logo(ctx, away_bmp, GRect(bounds.size.w - inset - logo, y_logo, logo, logo), false);
  }

  graphics_draw_text(ctx, s_live_home, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(inset, y_logo + logo, name_w, 22),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, s_live_away, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(bounds.size.w - inset - name_w, y_logo + logo, name_w, 22),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

  char score[24];
  if (n >= 5 && strcmp(p[2], "-") != 0) {
    snprintf(score, sizeof(score), "%s - %s", p[2], p[4]);
  } else {
    snprintf(score, sizeof(score), "-");
  }
  graphics_draw_text(ctx, score, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                     GRect(inset, y_logo + logo + 20, bounds.size.w - inset * 2, 32),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  char clock[32];
  clock[0] = '\0';
  if (n >= 7) {
    snprintf(clock, sizeof(clock), "%s", p[6]);
  }
  const char *status = NULL;
  if (n >= 6) {
    if (strcmp(p[5], "UP") == 0) {
      status = "Upcoming";
    } else if (strcmp(p[5], "HT") == 0) {
      status = "Half Time";
    } else if (strcmp(p[5], "FT") == 0) {
      status = "Full Time";
    } else if (strcmp(p[5], "BYE") != 0) {
      status = "Live";
    }
  }
  const int clock_y = y_logo + logo + 52;
  graphics_draw_text(ctx, clock, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                     GRect(inset, clock_y, bounds.size.w - inset * 2, 22),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  if (status) {
    graphics_draw_text(ctx, status, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(inset, clock_y + 20, bounds.size.w - inset * 2, 22),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }

  char home_pct[8];
  char away_pct[8];
  home_pct[0] = '\0';
  away_pct[0] = '\0';
  if (n >= 8 && p[7][0]) {
    format_odds_pair(p[7], home_pct, sizeof(home_pct), away_pct, sizeof(away_pct));
  }
  const int chance_y = clock_y + (status ? 40 : 20);
  if (home_pct[0]) {
    graphics_draw_text(ctx, home_pct, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(inset, chance_y, 36, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, "Odds", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(inset + 34, chance_y, bounds.size.w - inset * 2 - 68, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, away_pct, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(bounds.size.w - inset - 36, chance_y, 36, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
  }

  const char *vibe = persist_get_vibe() ? "Goal vibe ON" : "Goal vibe OFF";
#if defined(PBL_COLOR)
  graphics_context_set_text_color(ctx, GColorWhite);
#endif
  /* Down is the lower-right hardware button (~4 o'clock on round). */
#if defined(PBL_ROUND)
  const int vibe_h = 20;
  const int vibe_w = bounds.size.w / 2;
  int vibe_y = bounds.origin.y + (bounds.size.h * 3) / 4 - vibe_h / 2;
  const int below_status = clock_y + (status ? 44 : 24) + (home_pct[0] ? 20 : 0);
  if (vibe_y < below_status) {
    vibe_y = below_status;
  }
  if (vibe_y + vibe_h > bounds.origin.y + bounds.size.h - 2) {
    vibe_y = bounds.origin.y + bounds.size.h - vibe_h - 2;
  }
  graphics_draw_text(ctx, vibe, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(bounds.size.w - inset - vibe_w, vibe_y, vibe_w, vibe_h),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
#else
  graphics_draw_text(ctx, vibe, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(inset, bounds.origin.y + bounds.size.h - 20,
                           bounds.size.w - inset * 2, 18),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
#endif
}

static void live_down_click(ClickRecognizerRef rec, void *ctx) {
  (void)rec;
  (void)ctx;
  persist_set_vibe(!persist_get_vibe());
  if (persist_get_vibe()) {
    vibes_short_pulse();
  }
  if (s_live_layer) {
    layer_mark_dirty(s_live_layer);
  }
}

static void live_click_config(void *ctx) {
  (void)ctx;
  window_single_click_subscribe(BUTTON_ID_DOWN, live_down_click);
}

static void live_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
#if defined(PBL_COLOR)
  window_set_background_color(window, GColorBlack);
#endif
  s_live_layer = layer_create(bounds);
  layer_set_update_proc(s_live_layer, live_layer_update);
  layer_add_child(root, s_live_layer);
  window_set_click_config_provider(window, live_click_config);
}

static void live_unload(Window *window) {
  (void)window;
  layer_destroy(s_live_layer);
  s_live_layer = NULL;
}

static void screens_show_live_game(uint16_t row) {
  if (row >= s_list.count) {
    return;
  }
  live_game_apply_line(s_list.lines[row], false);
  if (!window_stack_contains_window(s_live_window)) {
    window_stack_push(s_live_window, true);
  } else if (s_live_layer) {
    layer_mark_dirty(s_live_layer);
  }
  comm_request(REQ_LIVE, persist_get_comp());
  start_live_timer();
}

static uint16_t pin_num_rows(MenuLayer *layer, uint16_t section, void *context) {
  (void)layer;
  (void)section;
  (void)context;
  if (s_pin_waiting || s_pin_done) {
    return 1;
  }
  if (s_pin_is_bye) {
    return 1;
  }
  return 2;
}

static int16_t pin_cell_height(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)index;
  (void)context;
  return PIN_CELL_HEIGHT;
}

static void pin_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *index, void *context) {
  (void)context;
  GRect bounds = layer_get_bounds(cell_layer);
  const bool highlight = menu_cell_layer_is_highlighted(cell_layer);
#if defined(PBL_COLOR)
  (void)highlight;
  graphics_context_set_text_color(ctx, GColorWhite);
#else
  graphics_context_set_text_color(ctx, highlight ? GColorWhite : GColorBlack);
#endif
  const char *text;
  if (s_pin_waiting) {
    text = "Pinning...";
  } else if (s_pin_done) {
    text = s_pin_ok
      ? (s_pin_status[0] ? s_pin_status : "Pinned")
      : (s_pin_error[0] ? s_pin_error : "Pin failed");
  } else if (s_pin_is_bye || index->row == 1) {
    text = "Add rest of season?";
  } else {
    text = "Add this game?";
  }
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                     GRect(bounds.origin.x + ROW_PAD, bounds.origin.y + 4,
                           bounds.size.w - ROW_PAD * 2, bounds.size.h - 6),
                     GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void pin_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section, void *context) {
  (void)section;
  (void)context;
  menu_cell_basic_header_draw(ctx, cell_layer,
                              s_pin_header[0] ? s_pin_header : "Timeline");
}

static void pin_cancel_timeout(void) {
  if (s_pin_timeout) {
    app_timer_cancel(s_pin_timeout);
    s_pin_timeout = NULL;
  }
}

static void pin_on_timeout(void *data) {
  (void)data;
  s_pin_timeout = NULL;
  if (!s_pin_waiting) {
    return;
  }
  s_pin_waiting = false;
  s_pin_done = true;
  s_pin_ok = false;
  strncpy(s_pin_error, "Pin timeout", sizeof(s_pin_error) - 1);
  s_pin_error[sizeof(s_pin_error) - 1] = '\0';
  if (s_pin_menu) {
    menu_layer_reload_data(s_pin_menu);
  }
}

static void pin_select_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  if (s_pin_waiting || !s_pin_menu) {
    return;
  }
  if (s_pin_done) {
    window_stack_pop(true);
    return;
  }
  MenuIndex idx = menu_layer_get_selected_index(s_pin_menu);
  const bool remaining = s_pin_is_bye || idx.row == 1;
  s_pin_waiting = true;
  s_pin_done = false;
  s_pin_ok = false;
  s_pin_status[0] = '\0';
  s_pin_error[0] = '\0';
  menu_layer_reload_data(s_pin_menu);
  pin_cancel_timeout();
  s_pin_timeout = app_timer_register(PIN_TIMEOUT_MS, pin_on_timeout, NULL);
  comm_request_pin((int)s_pin_row, remaining);
}

static void pin_menu_select(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)index;
  pin_select_click(NULL, context);
}

#if !defined(PBL_ROUND)
static void pin_up_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  if (!s_pin_menu || s_pin_waiting || s_pin_done) {
    return;
  }
  MenuIndex idx = menu_layer_get_selected_index(s_pin_menu);
  if (idx.row > 0) {
    idx.row--;
    menu_layer_set_selected_index(s_pin_menu, idx, MenuRowAlignCenter, true);
  }
}

static void pin_down_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  if (!s_pin_menu || s_pin_waiting || s_pin_done) {
    return;
  }
  const uint16_t last = pin_num_rows(s_pin_menu, 0, NULL);
  MenuIndex idx = menu_layer_get_selected_index(s_pin_menu);
  if (last > 0 && idx.row + 1 < last) {
    idx.row++;
    menu_layer_set_selected_index(s_pin_menu, idx, MenuRowAlignCenter, true);
  }
}

static void pin_click_config(void *context) {
  (void)context;
  window_single_click_subscribe(BUTTON_ID_UP, pin_up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, pin_down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, pin_select_click);
}
#endif

static void pin_handle_result(DictionaryIterator *iter) {
  if (!s_pin_window || !window_stack_contains_window(s_pin_window)) {
    return;
  }
  s_pin_waiting = false;
  pin_cancel_timeout();
  Tuple *status_t = dict_find(iter, MESSAGE_KEY_STATUS);
  s_pin_ok = status_t && status_t->value->int32 == STATUS_OK;
  s_pin_status[0] = '\0';
  s_pin_error[0] = '\0';
  if (s_pin_ok) {
    vibes_short_pulse();
    window_stack_pop(true);
    if (s_list_req == REQ_UPCOMING) {
      s_list.pending_req = REQ_UPCOMING;
      s_list.status = STATUS_LOADING;
      s_list.count = 0;
      s_list.error[0] = '\0';
      if (s_list_menu) {
        menu_layer_reload_data(s_list_menu);
      }
      comm_request_ex(REQ_UPCOMING, persist_get_comp(), s_req_year,
                      s_req_team[0] ? s_req_team : NULL);
    }
    return;
  }
  s_pin_done = true;
  Tuple *err = dict_find(iter, MESSAGE_KEY_ERROR);
  strncpy(s_pin_error, err ? err->value->cstring : "Pin failed",
          sizeof(s_pin_error) - 1);
  s_pin_error[sizeof(s_pin_error) - 1] = '\0';
  if (s_pin_menu) {
    menu_layer_reload_data(s_pin_menu);
  }
}

static void pin_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
#if !defined(PBL_ROUND)
  bounds.size.w -= ACTION_BAR_WIDTH;
#endif

  s_pin_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_pin_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = pin_num_rows,
    .get_cell_height = pin_cell_height,
    .draw_row = pin_draw_row,
    .get_header_height = list_header_height,
    .draw_header = pin_draw_header,
    .select_click = pin_menu_select,
  });
  style_menu(s_pin_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_pin_menu));

#if defined(PBL_ROUND)
  /* Action bars clip too much of the 180px circle; Select on the menu instead. */
  menu_layer_set_click_config_onto_window(s_pin_menu, window);
#else
  s_pin_check = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_CHECK);
  s_pin_bar = action_bar_layer_create();
  action_bar_layer_set_icon(s_pin_bar, BUTTON_ID_SELECT, s_pin_check);
  action_bar_layer_add_to_window(s_pin_bar, window);
  action_bar_layer_set_click_config_provider(s_pin_bar, pin_click_config);
#endif
}

static void pin_unload(Window *window) {
  (void)window;
  pin_cancel_timeout();
  if (s_pin_menu) {
    menu_layer_destroy(s_pin_menu);
    s_pin_menu = NULL;
  }
  if (s_pin_bar) {
    action_bar_layer_destroy(s_pin_bar);
    s_pin_bar = NULL;
  }
  if (s_pin_check) {
    gbitmap_destroy(s_pin_check);
    s_pin_check = NULL;
  }
  s_pin_waiting = false;
}

static void screens_show_pin_actions(uint16_t row) {
  s_pin_row = row;
  s_pin_waiting = false;
  s_pin_done = false;
  s_pin_ok = false;
  s_pin_status[0] = '\0';
  s_pin_error[0] = '\0';
  s_pin_is_bye = line_is_bye(s_list.lines[row]);

  char title[64];
  char sub[64];
  char bot[32];
  char home[8];
  char away[8];
  bool is_ladder = false;
  format_row(REQ_UPCOMING, s_list.lines[row], title, sizeof(title), sub, sizeof(sub),
             bot, sizeof(bot), home, sizeof(home), away, sizeof(away), &is_ladder);
  if (s_pin_is_bye || !home[0]) {
    strncpy(s_pin_header, title, sizeof(s_pin_header) - 1);
  } else {
    snprintf(s_pin_header, sizeof(s_pin_header), "%s v %s", home, away);
  }
  s_pin_header[sizeof(s_pin_header) - 1] = '\0';

  if (!window_stack_contains_window(s_pin_window)) {
    window_stack_push(s_pin_window, true);
  } else if (s_pin_menu) {
    menu_layer_reload_data(s_pin_menu);
  }
}

void screens_init(void) {
  memset(&s_list, 0, sizeof(s_list));
  memset(&s_detail, 0, sizeof(s_detail));
  s_list_window = window_create();
  window_set_window_handlers(s_list_window, (WindowHandlers) {
    .load = list_load,
    .unload = list_unload,
  });
  s_detail_window = window_create();
  window_set_window_handlers(s_detail_window, (WindowHandlers) {
    .load = detail_load,
    .unload = detail_unload,
  });
  s_year_window = window_create();
  window_set_window_handlers(s_year_window, (WindowHandlers) {
    .load = year_load,
    .unload = year_unload,
  });
  s_team_window = window_create();
  window_set_window_handlers(s_team_window, (WindowHandlers) {
    .load = team_load,
    .unload = team_unload,
  });
  s_comp_window = window_create();
  window_set_window_handlers(s_comp_window, (WindowHandlers) {
    .load = comp_load,
    .unload = comp_unload,
  });
  s_live_window = window_create();
  window_set_window_handlers(s_live_window, (WindowHandlers) {
    .load = live_load,
    .unload = live_unload,
  });
  s_pin_window = window_create();
  window_set_window_handlers(s_pin_window, (WindowHandlers) {
    .load = pin_load,
    .unload = pin_unload,
  });
}

void screens_deinit(void) {
  cancel_live_timer();
  window_destroy(s_list_window);
  window_destroy(s_detail_window);
  window_destroy(s_year_window);
  window_destroy(s_team_window);
  window_destroy(s_comp_window);
  window_destroy(s_live_window);
  window_destroy(s_pin_window);
  s_list_window = NULL;
  s_detail_window = NULL;
  s_year_window = NULL;
  s_team_window = NULL;
  s_comp_window = NULL;
  s_live_window = NULL;
  s_pin_window = NULL;
}

static void screens_show_list_ex(const char *name, int req, int year, const char *team) {
  strncpy(s_list_name, name, sizeof(s_list_name) - 1);
  s_list_name[sizeof(s_list_name) - 1] = '\0';
  s_list_req = req;
  s_req_year = year;
  if (team && team[0]) {
    strncpy(s_req_team, team, sizeof(s_req_team) - 1);
    s_req_team[sizeof(s_req_team) - 1] = '\0';
  } else {
    s_req_team[0] = '\0';
  }
  s_list.pending_req = (uint8_t)req;
  s_list.status = STATUS_LOADING;
  s_list.count = 0;
  s_list.title[0] = '\0';
  s_list.error[0] = '\0';

  if (!window_stack_contains_window(s_list_window)) {
    window_stack_push(s_list_window, true);
  } else if (s_list_menu) {
    menu_layer_reload_data(s_list_menu);
  }

  comm_request_ex(req, persist_get_comp(), s_req_year, s_req_team[0] ? s_req_team : NULL);
  start_live_timer();
}

void screens_show_list(const char *name, int req) {
  screens_show_list_ex(name, req, 0, NULL);
}

void screens_show_history(void) {
  fill_years();
  window_stack_push(s_year_window, true);
}

void screens_show_comp_menu(void) {
  window_stack_push(s_comp_window, true);
}

static void append_chunk_lines(ListData *list, const char *chunk) {
  const char *p = chunk;
  while (*p && list->count < NRL_MAX_ITEMS) {
    const char *nl = strchr(p, '\n');
    int len = nl ? (int)(nl - p) : (int)strlen(p);
    if (len > NRL_LINE_LEN - 1) {
      len = NRL_LINE_LEN - 1;
    }
    if (len > 0) {
      memcpy(list->lines[list->count], p, len);
      list->lines[list->count][len] = '\0';
      list->count++;
    }
    if (!nl) {
      break;
    }
    p = nl + 1;
  }
}

static void draw_highlight_current(void) {
  if (!s_list_menu || s_list_req != REQ_DRAW || s_list.status != STATUS_OK || s_list.count == 0) {
    return;
  }
  int current = -1;
  const char *round = persist_get_round();
  for (int i = 0; i < s_list.count; i++) {
    char p[8][20];
    int n = 0;
    split_line(s_list.lines[i], p, 8, &n);
    if (n >= 3 && strcmp(p[2], "1") == 0) {
      current = i;
      break;
    }
  }
  if (current < 0 && round && round[0]) {
    for (int i = 0; i < s_list.count; i++) {
      char p[8][20];
      int n = 0;
      split_line(s_list.lines[i], p, 8, &n);
      if (n >= 2 && strcmp(p[1], round) == 0) {
        current = i;
        break;
      }
    }
  }
  if (current >= 0) {
    menu_layer_set_selected_index(s_list_menu, (MenuIndex){ .section = 0, .row = (uint16_t)current },
                                 MenuRowAlignCenter, false);
  }
}

static void apply_payload(ListData *list, MenuLayer *menu, DictionaryIterator *iter) {
  Tuple *status_t = dict_find(iter, MESSAGE_KEY_STATUS);
  if (status_t && status_t->value->int32 == STATUS_ERROR) {
    list->status = STATUS_ERROR;
    Tuple *err = dict_find(iter, MESSAGE_KEY_ERROR);
    if (err) {
      strncpy(list->error, err->value->cstring, sizeof(list->error) - 1);
      list->error[sizeof(list->error) - 1] = '\0';
    }
    if (menu) {
      menu_layer_reload_data(menu);
    }
    return;
  }

  Tuple *title_t = dict_find(iter, MESSAGE_KEY_TITLE);
  if (title_t) {
    strncpy(list->title, title_t->value->cstring, sizeof(list->title) - 1);
    list->title[sizeof(list->title) - 1] = '\0';
  }

  Tuple *index_t = dict_find(iter, MESSAGE_KEY_CHUNK_INDEX);
  int index = index_t ? (int)index_t->value->int32 : 0;
  if (index == 0) {
    list->count = 0;
  }

  Tuple *chunk_t = dict_find(iter, MESSAGE_KEY_CHUNK);
  if (chunk_t) {
    append_chunk_lines(list, chunk_t->value->cstring);
  }

  Tuple *count_t = dict_find(iter, MESSAGE_KEY_CHUNK_COUNT);
  int count = count_t ? (int)count_t->value->int32 : 1;
  if (index + 1 >= count) {
    list->status = STATUS_OK;
  }

  if (menu) {
    menu_layer_reload_data(menu);
  }
}

void screens_handle_payload(DictionaryIterator *iter) {
  Tuple *req_t = dict_find(iter, MESSAGE_KEY_REQ);
  int req = req_t ? (int)req_t->value->int32 : 0;
  if (req == REQ_PIN) {
    pin_handle_result(iter);
    return;
  }
  if (req == s_detail.pending_req &&
      (s_detail.pending_req == REQ_STATS || s_detail.pending_req == REQ_DRAW_ROUND) &&
      window_stack_contains_window(s_detail_window)) {
    apply_payload(&s_detail, s_detail_menu, iter);
    return;
  }
  if (req_t && req_t->value->int32 != s_list.pending_req) {
    return;
  }
  apply_payload(&s_list, s_list_menu, iter);
  if (req == REQ_LIVE) {
    live_game_refresh();
  }
  if (req == REQ_DRAW && s_list.status == STATUS_OK) {
    draw_highlight_current();
  }
}

void screens_handle_send_failed(void) {
  if (s_pin_window && window_stack_contains_window(s_pin_window) && s_pin_waiting) {
    pin_cancel_timeout();
    s_pin_waiting = false;
    s_pin_done = true;
    s_pin_ok = false;
    strncpy(s_pin_error, "Phone offline", sizeof(s_pin_error) - 1);
    s_pin_error[sizeof(s_pin_error) - 1] = '\0';
    if (s_pin_menu) {
      menu_layer_reload_data(s_pin_menu);
    }
    return;
  }
  if (window_stack_contains_window(s_detail_window) && s_detail.status == STATUS_LOADING) {
    s_detail.status = STATUS_ERROR;
    strncpy(s_detail.error, "Phone offline", sizeof(s_detail.error) - 1);
    if (s_detail_menu) {
      menu_layer_reload_data(s_detail_menu);
    }
    return;
  }
  s_list.status = STATUS_ERROR;
  strncpy(s_list.error, "Phone offline", sizeof(s_list.error) - 1);
  if (s_list_menu) {
    menu_layer_reload_data(s_list_menu);
  }
}

void screens_settings_changed(void) {
  if (s_list_menu && window_stack_contains_window(s_list_window)) {
    menu_layer_reload_data(s_list_menu);
  }
  if (s_detail_menu && window_stack_contains_window(s_detail_window)) {
    menu_layer_reload_data(s_detail_menu);
  }
  if (s_live_layer && window_stack_contains_window(s_live_window)) {
    layer_mark_dirty(s_live_layer);
  }
}
