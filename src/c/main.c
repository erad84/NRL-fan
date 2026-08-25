#include "nrl.h"
#include "logos.h"

#define PERSIST_COMP 1
#define PERSIST_ROUND 2
#define PERSIST_FAV_NRL 3
#define PERSIST_FAV_NRLW 4
#define PERSIST_FAV_ORIGIN 5
#define PERSIST_VIBE 6

#if defined(PBL_PLATFORM_GABBRO)
#define MAIN_HEADER_HEIGHT 210
#define HEADER_COMP_LOGO 64
#define HEADER_FAV_LOGO 64
#define HEADER_FAV_GAP 12
#elif defined(PBL_ROUND)
#define MAIN_HEADER_HEIGHT 148
#define HEADER_COMP_LOGO 48
#define HEADER_FAV_LOGO 48
#define HEADER_FAV_GAP 10
#else
#define MAIN_HEADER_HEIGHT 140
#define HEADER_COMP_LOGO 48
#define HEADER_FAV_LOGO 48
#define HEADER_FAV_GAP 10
#endif

static int s_comp = COMP_NRL;
static char s_round[NRL_ROUND_LEN];
static char s_fav_nrl[NRL_TEAM_LEN] = "Broncos";
static char s_fav_nrlw[NRL_TEAM_LEN] = "Broncos";
static char s_fav_origin[NRL_TEAM_LEN] = "NSW";
static bool s_vibe = true;
static Window *s_main_window;
static MenuLayer *s_main_menu;
static char s_upcoming_title[NRL_NAME_LEN];
static char s_sum_pos[8];

int persist_get_comp(void) {
  return s_comp;
}

void persist_set_comp(int comp) {
  if (comp < 0 || comp >= COMP_COUNT) {
    return;
  }
  s_comp = comp;
  persist_write_int(PERSIST_COMP, s_comp);
}

static bool is_origin_comp(void) {
  return s_comp == COMP_ORIGIN_MEN || s_comp == COMP_ORIGIN_WOMEN;
}

static void format_round_label(const char *round, char *out, size_t out_len) {
  if (!round || !round[0] || out_len == 0) {
    if (out && out_len) {
      out[0] = '\0';
    }
    return;
  }
  if ((round[0] == 'R' || round[0] == 'r' || round[0] == 'G' || round[0] == 'g') &&
      round[1] >= '0' && round[1] <= '9') {
    snprintf(out, out_len, "%s %s",
             (round[0] == 'G' || round[0] == 'g') ? "Game" : "Round", round + 1);
    return;
  }
  strncpy(out, round, out_len - 1);
  out[out_len - 1] = '\0';
}

static void header_round_text(char *out, size_t out_len) {
  const char *round = persist_get_round();
  const bool origin = is_origin_comp();
  if (!round || !round[0]) {
    strncpy(out, origin ? "Game" : "Round", out_len - 1);
    out[out_len - 1] = '\0';
    return;
  }
  if (origin && strncmp(round, "Round", 5) == 0) {
    snprintf(out, out_len, "Game%s", round + 5);
    return;
  }
  if (!origin && strncmp(round, "Game", 4) == 0) {
    snprintf(out, out_len, "Round%s", round + 4);
    return;
  }
  strncpy(out, round, out_len - 1);
  out[out_len - 1] = '\0';
}

static void persist_read_fav(uint32_t key, char *buf, size_t len, const char *fallback) {
  if (persist_exists(key)) {
    persist_read_string(key, buf, len);
    if (buf[0]) {
      return;
    }
  }
  strncpy(buf, fallback, len - 1);
  buf[len - 1] = '\0';
}

void persist_load(void) {
  if (persist_exists(PERSIST_COMP)) {
    int value = persist_read_int(PERSIST_COMP);
    if (value >= 0 && value < COMP_COUNT) {
      s_comp = value;
    }
  }
  if (persist_exists(PERSIST_ROUND)) {
    char stored[NRL_ROUND_LEN];
    persist_read_string(PERSIST_ROUND, stored, sizeof(stored));
    format_round_label(stored, s_round, sizeof(s_round));
  }
  persist_read_fav(PERSIST_FAV_NRL, s_fav_nrl, sizeof(s_fav_nrl), "Broncos");
  persist_read_fav(PERSIST_FAV_NRLW, s_fav_nrlw, sizeof(s_fav_nrlw), "Broncos");
  persist_read_fav(PERSIST_FAV_ORIGIN, s_fav_origin, sizeof(s_fav_origin), "NSW");
  if (persist_exists(PERSIST_VIBE)) {
    s_vibe = persist_read_int(PERSIST_VIBE) != 0;
  }
}

const char *persist_get_round(void) {
  return s_round[0] ? s_round : NULL;
}

void persist_set_round(const char *round) {
  char formatted[NRL_ROUND_LEN];
  format_round_label(round, formatted, sizeof(formatted));
  if (!formatted[0]) {
    return;
  }
  if (strcmp(s_round, formatted) == 0) {
    return;
  }
  strncpy(s_round, formatted, sizeof(s_round) - 1);
  s_round[sizeof(s_round) - 1] = '\0';
  persist_write_string(PERSIST_ROUND, s_round);
}

void persist_set_fav(int comp, const char *nick) {
  if (!nick || !nick[0]) {
    return;
  }
  char *buf = s_fav_nrl;
  uint32_t key = PERSIST_FAV_NRL;
  if (comp == COMP_NRLW) {
    buf = s_fav_nrlw;
    key = PERSIST_FAV_NRLW;
  } else if (comp == COMP_ORIGIN_MEN || comp == COMP_ORIGIN_WOMEN) {
    buf = s_fav_origin;
    key = PERSIST_FAV_ORIGIN;
  }
  if (strcmp(buf, nick) == 0) {
    return;
  }
  strncpy(buf, nick, NRL_TEAM_LEN - 1);
  buf[NRL_TEAM_LEN - 1] = '\0';
  persist_write_string(key, buf);
}

const char *persist_get_fav(void) {
  if (s_comp == COMP_NRLW) {
    return s_fav_nrlw;
  }
  if (s_comp == COMP_ORIGIN_MEN || s_comp == COMP_ORIGIN_WOMEN) {
    return s_fav_origin;
  }
  return s_fav_nrl;
}

const char *persist_fav_animal(void) {
  const char *nick = persist_get_fav();
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

bool persist_get_vibe(void) {
  return s_vibe;
}

void persist_set_vibe(bool on) {
  s_vibe = on;
  persist_write_int(PERSIST_VIBE, on ? 1 : 0);
}

const char *comp_label(int comp) {
  switch (comp) {
    case COMP_NRLW: return "NRLW";
    case COMP_ORIGIN_MEN: return "Origin";
    case COMP_ORIGIN_WOMEN: return "Origin W";
    default: return "NRL";
  }
}

static void format_ordinal(const char *pos, char *out, size_t out_len) {
  if (!pos || !pos[0] || pos[0] == '-') {
    strncpy(out, "--", out_len - 1);
    out[out_len - 1] = '\0';
    return;
  }
  if (pos[0] < '0' || pos[0] > '9') {
    strncpy(out, pos, out_len - 1);
    out[out_len - 1] = '\0';
    return;
  }
  int v = atoi(pos);
  const char *suf = "th";
  int mod = v % 100;
  if (mod < 11 || mod > 13) {
    switch (v % 10) {
      case 1: suf = "st"; break;
      case 2: suf = "nd"; break;
      case 3: suf = "rd"; break;
    }
  }
  snprintf(out, out_len, "%d%s", v, suf);
}

static uint16_t main_num_rows(MenuLayer *layer, uint16_t section, void *context) {
  (void)layer;
  (void)section;
  (void)context;
  return 7;
}

static int16_t main_cell_height(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)context;
  if (index->row == 0) {
    return MAIN_HEADER_HEIGHT;
  }
  return 44;
}

static void main_draw_header_content(GContext *ctx, GRect bounds, bool highlight) {
#if defined(PBL_COLOR)
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorWhite);
#else
  graphics_context_set_text_color(ctx, highlight ? GColorWhite : GColorBlack);
#endif

  const int inset = PBL_IF_ROUND_ELSE(18, 6);
  const int top = bounds.origin.y + PBL_IF_ROUND_ELSE(10, 4);
  GBitmap *comp_bmp = logo_for_comp(s_comp);
  if (comp_bmp) {
    draw_logo(ctx, comp_bmp,
              GRect((bounds.size.w - HEADER_COMP_LOGO) / 2, top,
                    HEADER_COMP_LOGO, HEADER_COMP_LOGO), highlight);
  }

  char round_text[NRL_ROUND_LEN];
  header_round_text(round_text, sizeof(round_text));
  graphics_draw_text(ctx, round_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(inset, top + HEADER_COMP_LOGO + 2, bounds.size.w - inset * 2, 28),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  const char *fav_code = club_code_for_nick(persist_get_fav());
  GBitmap *fav_bmp = logo_for_code(fav_code);
  char pos[12];
  format_ordinal(s_sum_pos, pos, sizeof(pos));
  GFont pos_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GSize pos_size = graphics_text_layout_get_content_size(
      pos, pos_font, GRect(0, 0, bounds.size.w, 30),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  int group_w = pos_size.w;
  if (fav_bmp) {
    group_w += HEADER_FAV_LOGO + HEADER_FAV_GAP;
  }
  int x = (bounds.size.w - group_w) / 2;
  int row_y = top + HEADER_COMP_LOGO + 32;
  if (fav_bmp) {
    draw_logo(ctx, fav_bmp, GRect(x, row_y, HEADER_FAV_LOGO, HEADER_FAV_LOGO), highlight);
    x += HEADER_FAV_LOGO + HEADER_FAV_GAP;
  }
  graphics_draw_text(ctx, pos, pos_font,
                     GRect(x, row_y + (HEADER_FAV_LOGO - 28) / 2, pos_size.w + 8, 30),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void main_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *index, void *context) {
  (void)context;
  GRect bounds = layer_get_bounds(cell_layer);
  const bool highlight = menu_cell_layer_is_highlighted(cell_layer);
  if (index->row == 0) {
    main_draw_header_content(ctx, bounds, highlight);
    return;
  }

  const bool origin = is_origin_comp();
  const char *title = "";
  const char *subtitle = NULL;
  switch (index->row) {
    case 1:
      title = persist_fav_animal();
      subtitle = "upcoming rounds";
      break;
    case 2: title = origin ? "Series" : "Ladder"; break;
    case 3: title = "Live"; break;
    case 4: title = "My Team Results"; break;
    case 5: title = "History"; break;
    case 6:
      title = "Competition";
      subtitle = comp_label(s_comp);
      break;
  }
  menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
}

static void main_select(MenuLayer *layer, MenuIndex *index, void *context) {
  (void)layer;
  (void)context;
  switch (index->row) {
    case 0:
      break;
    case 1:
      snprintf(s_upcoming_title, sizeof(s_upcoming_title), "%s upcoming rounds",
               persist_fav_animal());
      screens_show_list(s_upcoming_title, REQ_UPCOMING);
      break;
    case 2:
      screens_show_list(is_origin_comp() ? "Series" : "Ladder", REQ_LADDER);
      break;
    case 3: screens_show_list("Live", REQ_LIVE); break;
    case 4: screens_show_list("My Team Results", REQ_RESULTS); break;
    case 5: screens_show_history(); break;
    case 6: screens_show_comp_menu(); break;
  }
}

void main_reload(void) {
  if (s_main_menu) {
    menu_layer_reload_data(s_main_menu);
  }
}

static void parse_summary_lines(const char *chunk) {
  const char *p = chunk;
  while (*p) {
    const char *nl = strchr(p, '\n');
    int len = nl ? (int)(nl - p) : (int)strlen(p);
    char line[48];
    if (len > 47) {
      len = 47;
    }
    memcpy(line, p, len);
    line[len] = '\0';
    if (strncmp(line, "POS|", 4) == 0) {
      strncpy(s_sum_pos, line + 4, sizeof(s_sum_pos) - 1);
      s_sum_pos[sizeof(s_sum_pos) - 1] = '\0';
    }
    if (!nl) {
      break;
    }
    p = nl + 1;
  }
}

void main_handle_summary(DictionaryIterator *iter) {
  Tuple *status_t = dict_find(iter, MESSAGE_KEY_STATUS);
  if (status_t && status_t->value->int32 == STATUS_ERROR) {
    return;
  }
  Tuple *title_t = dict_find(iter, MESSAGE_KEY_TITLE);
  if (title_t) {
    persist_set_round(title_t->value->cstring);
  }
  Tuple *chunk_t = dict_find(iter, MESSAGE_KEY_CHUNK);
  if (chunk_t) {
    parse_summary_lines(chunk_t->value->cstring);
  }
  main_reload();
}

static void main_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_main_menu = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_main_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = main_num_rows,
    .get_cell_height = main_cell_height,
    .draw_row = main_draw_row,
    .select_click = main_select,
  });
  menu_layer_set_click_config_onto_window(s_main_menu, window);
#if defined(PBL_ROUND)
  menu_layer_set_center_focused(s_main_menu, true);
#endif
#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_main_menu, GColorBlack, GColorWhite);
  menu_layer_set_highlight_colors(s_main_menu, GColorDarkGreen, GColorWhite);
  window_set_background_color(window, GColorBlack);
#endif
  layer_add_child(root, menu_layer_get_layer(s_main_menu));
}

static void main_appear(Window *window) {
  (void)window;
  comm_request(REQ_SUMMARY, persist_get_comp());
  main_reload();
}

static void main_unload(Window *window) {
  (void)window;
  menu_layer_destroy(s_main_menu);
  s_main_menu = NULL;
}

static void init(void) {
  persist_load();
  logos_init();
  comm_init();
  screens_init();
#if defined(PBL_TOUCH)
  app_touch_navigation_enable(true);
#endif

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_load,
    .appear = main_appear,
    .unload = main_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  screens_deinit();
  comm_deinit();
  logos_deinit();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
