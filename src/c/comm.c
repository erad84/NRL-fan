#include "nrl.h"

#include <stdlib.h>

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *def = dict_find(iter, MESSAGE_KEY_DEFAULT_COMP);
  if (def) {
    int value = (def->type == TUPLE_CSTRING) ? atoi(def->value->cstring)
                                             : (int)def->value->int32;
    persist_set_comp(value);
  }

  Tuple *fav_nrl = dict_find(iter, MESSAGE_KEY_FAV_NRL);
  if (fav_nrl && fav_nrl->type == TUPLE_CSTRING) {
    persist_set_fav(COMP_NRL, fav_nrl->value->cstring);
  }
  Tuple *fav_nrlw = dict_find(iter, MESSAGE_KEY_FAV_NRLW);
  if (fav_nrlw && fav_nrlw->type == TUPLE_CSTRING) {
    persist_set_fav(COMP_NRLW, fav_nrlw->value->cstring);
  }
  Tuple *fav_origin = dict_find(iter, MESSAGE_KEY_FAV_ORIGIN);
  if (fav_origin && fav_origin->type == TUPLE_CSTRING) {
    persist_set_fav(COMP_ORIGIN_MEN, fav_origin->value->cstring);
    persist_set_fav(COMP_ORIGIN_WOMEN, fav_origin->value->cstring);
  }

  Tuple *req_t = dict_find(iter, MESSAGE_KEY_REQ);
  if (req_t && req_t->value->int32 == REQ_SUMMARY) {
    main_handle_summary(iter);
    return;
  }

  if (dict_find(iter, MESSAGE_KEY_STATUS) || dict_find(iter, MESSAGE_KEY_CHUNK)) {
    screens_handle_payload(iter);
  }
}

static void inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Inbox dropped: %d", (int)reason);
}

static void outbox_failed(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox failed: %d", (int)reason);
  screens_handle_send_failed();
}

void comm_init(void) {
  app_message_register_inbox_received(inbox_received);
  app_message_register_inbox_dropped(inbox_dropped);
  app_message_register_outbox_failed(outbox_failed);
  /* Do not use app_message_inbox_size_maximum() — firmware logs and it
     reserves ~8 KB even though list chunks stay under 400 bytes. */
#if defined(PBL_PLATFORM_APLITE)
  app_message_open(512, 128);
#else
  app_message_open(1024, 256);
#endif
}

void comm_deinit(void) {
  app_message_deregister_callbacks();
}

void comm_request(int req, int comp) {
  comm_request_ex(req, comp, 0, NULL);
}

void comm_request_ex(int req, int comp, int year, const char *team) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox begin: %d", (int)result);
    screens_handle_send_failed();
    return;
  }
  dict_write_int32(iter, MESSAGE_KEY_REQ, req);
  dict_write_int32(iter, MESSAGE_KEY_COMP, comp);
  if (year > 0) {
    dict_write_int32(iter, MESSAGE_KEY_YEAR, year);
  }
  if (team && team[0]) {
    dict_write_cstring(iter, MESSAGE_KEY_TEAM, team);
  }
  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send: %d", (int)result);
    screens_handle_send_failed();
  }
}

void comm_request_pin(int row, bool remaining) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox begin: %d", (int)result);
    screens_handle_send_failed();
    return;
  }
  dict_write_int32(iter, MESSAGE_KEY_REQ, REQ_PIN);
  dict_write_int32(iter, MESSAGE_KEY_COMP, persist_get_comp());
  dict_write_int32(iter, MESSAGE_KEY_ROW, row);
  dict_write_int32(iter, MESSAGE_KEY_PIN_ALL, remaining ? 1 : 0);
  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send: %d", (int)result);
    screens_handle_send_failed();
  }
}
