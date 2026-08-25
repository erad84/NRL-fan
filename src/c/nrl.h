#pragma once

#include <pebble.h>

#define NRL_MAX_ITEMS 32
#define NRL_LINE_LEN 48
#define NRL_TITLE_LEN 32
#define NRL_NAME_LEN 32
#define NRL_ROUND_LEN 16
#define NRL_TEAM_LEN 16
#define NRL_YEAR_COUNT 12

enum {
  COMP_NRL = 0,
  COMP_NRLW = 1,
  COMP_ORIGIN_MEN = 2,
  COMP_ORIGIN_WOMEN = 3,
  COMP_COUNT = 4
};

enum {
  REQ_MY_TEAM = 1,
  REQ_LIVE = 2,
  REQ_UPCOMING = 3,
  REQ_RESULTS = 4,
  REQ_LADDER = 5,
  REQ_HISTORY = 6,
  REQ_STATS = 7,
  REQ_SUMMARY = 8,
  REQ_PIN = 9
};

enum {
  STATUS_OK = 0,
  STATUS_LOADING = 1,
  STATUS_ERROR = 2
};

typedef struct {
  char lines[NRL_MAX_ITEMS][NRL_LINE_LEN];
  uint8_t count;
  uint8_t status;
  uint8_t pending_req;
  char title[NRL_TITLE_LEN];
  char error[NRL_LINE_LEN];
} ListData;

void persist_load(void);
int persist_get_comp(void);
void persist_set_comp(int comp);
const char *persist_get_round(void);
void persist_set_round(const char *round);
void persist_set_fav(int comp, const char *nick);
const char *persist_get_fav(void);
const char *persist_fav_animal(void);
bool persist_get_vibe(void);
void persist_set_vibe(bool on);
const char *comp_label(int comp);
const char *club_code_for_nick(const char *nick);

void main_handle_summary(DictionaryIterator *iter);
void main_reload(void);

void comm_init(void);
void comm_deinit(void);
void comm_request(int req, int comp);
void comm_request_ex(int req, int comp, int year, const char *team);
void comm_request_pin(int row, bool remaining);

void screens_init(void);
void screens_deinit(void);
void screens_show_list(const char *name, int req);
void screens_show_history(void);
void screens_show_comp_menu(void);
void screens_handle_payload(DictionaryIterator *iter);
void screens_handle_send_failed(void);
