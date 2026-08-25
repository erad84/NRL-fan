#include "logos.h"

#include "nrl.h"

typedef struct {
  const char *code;
  uint32_t resource;
#if defined(PBL_BW)
  uint32_t resource_w;
#endif
} LogoEntry;

static const LogoEntry s_entries[] = {
#if defined(PBL_BW)
  {"BRO", RESOURCE_ID_IMAGE_LOGO_BRO, RESOURCE_ID_IMAGE_LOGO_BRO_WHITE},
  {"BUL", RESOURCE_ID_IMAGE_LOGO_BUL, RESOURCE_ID_IMAGE_LOGO_BUL_WHITE},
  {"COW", RESOURCE_ID_IMAGE_LOGO_COW, RESOURCE_ID_IMAGE_LOGO_COW_WHITE},
  {"DOL", RESOURCE_ID_IMAGE_LOGO_DOL, RESOURCE_ID_IMAGE_LOGO_DOL_WHITE},
  {"DRA", RESOURCE_ID_IMAGE_LOGO_DRA, RESOURCE_ID_IMAGE_LOGO_DRA_WHITE},
  {"EEL", RESOURCE_ID_IMAGE_LOGO_EEL, RESOURCE_ID_IMAGE_LOGO_EEL_WHITE},
  {"KNI", RESOURCE_ID_IMAGE_LOGO_KNI, RESOURCE_ID_IMAGE_LOGO_KNI_WHITE},
  {"PAN", RESOURCE_ID_IMAGE_LOGO_PAN, RESOURCE_ID_IMAGE_LOGO_PAN_WHITE},
  {"SOU", RESOURCE_ID_IMAGE_LOGO_SOU, RESOURCE_ID_IMAGE_LOGO_SOU_WHITE},
  {"CAN", RESOURCE_ID_IMAGE_LOGO_CAN, RESOURCE_ID_IMAGE_LOGO_CAN_WHITE},
  {"SYD", RESOURCE_ID_IMAGE_LOGO_SYD, RESOURCE_ID_IMAGE_LOGO_SYD_WHITE},
  {"MAN", RESOURCE_ID_IMAGE_LOGO_MAN, RESOURCE_ID_IMAGE_LOGO_MAN_WHITE},
  {"CRO", RESOURCE_ID_IMAGE_LOGO_CRO, RESOURCE_ID_IMAGE_LOGO_CRO_WHITE},
  {"MEL", RESOURCE_ID_IMAGE_LOGO_MEL, RESOURCE_ID_IMAGE_LOGO_MEL_WHITE},
  {"GLD", RESOURCE_ID_IMAGE_LOGO_GLD, RESOURCE_ID_IMAGE_LOGO_GLD_WHITE},
  {"WAR", RESOURCE_ID_IMAGE_LOGO_WAR, RESOURCE_ID_IMAGE_LOGO_WAR_WHITE},
  {"WST", RESOURCE_ID_IMAGE_LOGO_WST, RESOURCE_ID_IMAGE_LOGO_WST_WHITE},
  {"NSW", RESOURCE_ID_IMAGE_LOGO_NSW, RESOURCE_ID_IMAGE_LOGO_NSW_WHITE},
  {"QLD", RESOURCE_ID_IMAGE_LOGO_QLD, RESOURCE_ID_IMAGE_LOGO_QLD_WHITE},
#else
  {"BRO", RESOURCE_ID_IMAGE_LOGO_BRO},
  {"BUL", RESOURCE_ID_IMAGE_LOGO_BUL},
  {"COW", RESOURCE_ID_IMAGE_LOGO_COW},
  {"DOL", RESOURCE_ID_IMAGE_LOGO_DOL},
  {"DRA", RESOURCE_ID_IMAGE_LOGO_DRA},
  {"EEL", RESOURCE_ID_IMAGE_LOGO_EEL},
  {"KNI", RESOURCE_ID_IMAGE_LOGO_KNI},
  {"PAN", RESOURCE_ID_IMAGE_LOGO_PAN},
  {"SOU", RESOURCE_ID_IMAGE_LOGO_SOU},
  {"CAN", RESOURCE_ID_IMAGE_LOGO_CAN},
  {"SYD", RESOURCE_ID_IMAGE_LOGO_SYD},
  {"MAN", RESOURCE_ID_IMAGE_LOGO_MAN},
  {"CRO", RESOURCE_ID_IMAGE_LOGO_CRO},
  {"MEL", RESOURCE_ID_IMAGE_LOGO_MEL},
  {"GLD", RESOURCE_ID_IMAGE_LOGO_GLD},
  {"WAR", RESOURCE_ID_IMAGE_LOGO_WAR},
  {"WST", RESOURCE_ID_IMAGE_LOGO_WST},
  {"NSW", RESOURCE_ID_IMAGE_LOGO_NSW},
  {"QLD", RESOURCE_ID_IMAGE_LOGO_QLD},
#endif
};

#define LOGO_COUNT ((int)(sizeof(s_entries) / sizeof(s_entries[0])))

#if defined(PBL_PLATFORM_APLITE)
#define TEAM_LOGO_CACHE 6
static uint16_t s_stamp[LOGO_COUNT];
static uint16_t s_now;
#endif

static GBitmap *s_bitmaps[LOGO_COUNT];
#if defined(PBL_BW)
static GBitmap *s_white[LOGO_COUNT];
#endif
static GBitmap *s_nrl;
static GBitmap *s_nrlw;
static GBitmap *s_origin;
static GBitmap *s_origin_w;
static GBitmap *s_trophy;
#if defined(PBL_BW)
static GBitmap *s_nrl_w;
static GBitmap *s_nrlw_w;
static GBitmap *s_origin_men_w;
static GBitmap *s_origin_w_w;
static GBitmap *s_trophy_w;
#endif

#if defined(PBL_PLATFORM_APLITE)
static int loaded_team_count(void) {
  int n = 0;
  for (int i = 0; i < LOGO_COUNT; i++) {
    if (s_bitmaps[i] || s_white[i]) {
      n++;
    }
  }
  return n;
}

static void evict_lru(void) {
  int victim = -1;
  uint16_t best = 0xFFFF;
  for (int i = 0; i < LOGO_COUNT; i++) {
    if ((s_bitmaps[i] || s_white[i]) && s_stamp[i] <= best) {
      best = s_stamp[i];
      victim = i;
    }
  }
  if (victim >= 0) {
    if (s_bitmaps[victim]) {
      gbitmap_destroy(s_bitmaps[victim]);
      s_bitmaps[victim] = NULL;
    }
    if (s_white[victim]) {
      gbitmap_destroy(s_white[victim]);
      s_white[victim] = NULL;
    }
  }
}
#endif

void logos_init(void) {
#if !defined(PBL_PLATFORM_APLITE)
  for (int i = 0; i < LOGO_COUNT; i++) {
    s_bitmaps[i] = gbitmap_create_with_resource(s_entries[i].resource);
#if defined(PBL_BW)
    s_white[i] = gbitmap_create_with_resource(s_entries[i].resource_w);
#endif
  }
  s_nrl = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_NRL);
  s_nrlw = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_NRLW);
  s_origin = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_ORIGIN);
  s_origin_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_ORIGIN_W);
  s_trophy = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TROPHY);
#if defined(PBL_BW)
  s_nrl_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_NRL_WHITE);
  s_nrlw_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_NRLW_WHITE);
  s_origin_men_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_ORIGIN_WHITE);
  s_origin_w_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_ORIGIN_W_WHITE);
  s_trophy_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TROPHY_WHITE);
#endif
#endif
}

void logos_deinit(void) {
  for (int i = 0; i < LOGO_COUNT; i++) {
    if (s_bitmaps[i]) {
      gbitmap_destroy(s_bitmaps[i]);
      s_bitmaps[i] = NULL;
    }
#if defined(PBL_BW)
    if (s_white[i]) {
      gbitmap_destroy(s_white[i]);
      s_white[i] = NULL;
    }
#endif
  }
  if (s_nrl) {
    gbitmap_destroy(s_nrl);
    s_nrl = NULL;
  }
  if (s_nrlw) {
    gbitmap_destroy(s_nrlw);
    s_nrlw = NULL;
  }
  if (s_origin) {
    gbitmap_destroy(s_origin);
    s_origin = NULL;
  }
  if (s_origin_w) {
    gbitmap_destroy(s_origin_w);
    s_origin_w = NULL;
  }
  if (s_trophy) {
    gbitmap_destroy(s_trophy);
    s_trophy = NULL;
  }
#if defined(PBL_BW)
  if (s_nrl_w) {
    gbitmap_destroy(s_nrl_w);
    s_nrl_w = NULL;
  }
  if (s_nrlw_w) {
    gbitmap_destroy(s_nrlw_w);
    s_nrlw_w = NULL;
  }
  if (s_origin_men_w) {
    gbitmap_destroy(s_origin_men_w);
    s_origin_men_w = NULL;
  }
  if (s_origin_w_w) {
    gbitmap_destroy(s_origin_w_w);
    s_origin_w_w = NULL;
  }
  if (s_trophy_w) {
    gbitmap_destroy(s_trophy_w);
    s_trophy_w = NULL;
  }
#endif
}

GBitmap *logo_trophy(void) {
#if defined(PBL_PLATFORM_APLITE)
  if (!s_trophy) {
    s_trophy = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TROPHY);
  }
#endif
  return s_trophy;
}

GBitmap *logo_for_code(const char *code) {
  if (!code || !code[0]) {
    return NULL;
  }
  for (int i = 0; i < LOGO_COUNT; i++) {
    if (strcmp(code, s_entries[i].code) == 0) {
      if (!s_bitmaps[i]) {
#if defined(PBL_PLATFORM_APLITE)
        if (loaded_team_count() >= TEAM_LOGO_CACHE) {
          evict_lru();
        }
#endif
        s_bitmaps[i] = gbitmap_create_with_resource(s_entries[i].resource);
      }
#if defined(PBL_PLATFORM_APLITE)
      s_stamp[i] = ++s_now;
#endif
      return s_bitmaps[i];
    }
  }
  return NULL;
}

GBitmap *logo_for_comp(int comp) {
  GBitmap **slot = &s_nrl;
  uint32_t resource = RESOURCE_ID_IMAGE_LOGO_NRL;
  switch (comp) {
    case COMP_NRLW:
      slot = &s_nrlw;
      resource = RESOURCE_ID_IMAGE_LOGO_NRLW;
      break;
    case COMP_ORIGIN_WOMEN:
      slot = &s_origin_w;
      resource = RESOURCE_ID_IMAGE_LOGO_ORIGIN_W;
      break;
    case COMP_ORIGIN_MEN:
      slot = &s_origin;
      resource = RESOURCE_ID_IMAGE_LOGO_ORIGIN;
      break;
    default:
      break;
  }
#if defined(PBL_PLATFORM_APLITE)
  if (!*slot) {
    *slot = gbitmap_create_with_resource(resource);
  }
#else
  (void)resource;
#endif
  return *slot;
}

#if defined(PBL_BW)
static GBitmap *ensure_white_at(int i) {
  if (!s_white[i]) {
#if defined(PBL_PLATFORM_APLITE)
    if (loaded_team_count() >= TEAM_LOGO_CACHE) {
      evict_lru();
    }
#endif
    s_white[i] = gbitmap_create_with_resource(s_entries[i].resource_w);
  }
#if defined(PBL_PLATFORM_APLITE)
  s_stamp[i] = ++s_now;
#endif
  return s_white[i];
}

static GBitmap *white_for_bitmap(GBitmap *bmp) {
  if (!bmp) {
    return NULL;
  }
  for (int i = 0; i < LOGO_COUNT; i++) {
    if (bmp == s_bitmaps[i] || bmp == s_white[i]) {
      return ensure_white_at(i);
    }
  }
  if (bmp == s_nrl || bmp == s_nrl_w) {
#if defined(PBL_PLATFORM_APLITE)
    if (!s_nrl_w) {
      s_nrl_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_NRL_WHITE);
    }
#endif
    return s_nrl_w;
  }
  if (bmp == s_nrlw || bmp == s_nrlw_w) {
#if defined(PBL_PLATFORM_APLITE)
    if (!s_nrlw_w) {
      s_nrlw_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_NRLW_WHITE);
    }
#endif
    return s_nrlw_w;
  }
  if (bmp == s_origin || bmp == s_origin_men_w) {
#if defined(PBL_PLATFORM_APLITE)
    if (!s_origin_men_w) {
      s_origin_men_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_ORIGIN_WHITE);
    }
#endif
    return s_origin_men_w;
  }
  if (bmp == s_origin_w || bmp == s_origin_w_w) {
#if defined(PBL_PLATFORM_APLITE)
    if (!s_origin_w_w) {
      s_origin_w_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_ORIGIN_W_WHITE);
    }
#endif
    return s_origin_w_w;
  }
  if (bmp == s_trophy || bmp == s_trophy_w) {
#if defined(PBL_PLATFORM_APLITE)
    if (!s_trophy_w) {
      s_trophy_w = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TROPHY_WHITE);
    }
#endif
    return s_trophy_w;
  }
  return NULL;
}
#endif

void draw_logo(GContext *ctx, GBitmap *bmp, GRect box, bool highlight) {
  if (!bmp) {
    return;
  }
#if defined(PBL_COLOR)
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  (void)highlight;
#else
  if (highlight) {
    GBitmap *white = white_for_bitmap(bmp);
    if (white) {
      bmp = white;
    }
  }
  /* White-on-black 1-bit: OR paints ink without filling a black square. */
  graphics_context_set_compositing_mode(ctx, highlight ? GCompOpOr : GCompOpAnd);
#endif
  GSize size = gbitmap_get_bounds(bmp).size;
  int dw = box.size.w;
  int dh = box.size.h;
  if (size.w > 0 && size.h > 0) {
    if (size.w * dh > size.h * dw) {
      dh = (size.h * dw) / size.w;
    } else if (size.h > 0) {
      dw = (size.w * dh) / size.h;
    }
  }
  GRect dest = {
    .origin = {
      box.origin.x + (box.size.w - dw) / 2,
      box.origin.y + (box.size.h - dh) / 2
    },
    .size = { .w = dw, .h = dh }
  };
  graphics_draw_bitmap_in_rect(ctx, bmp, dest);
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}
