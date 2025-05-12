#include <assert.h>
#include <stdlib.h>
#include <hb-ot.h>
#include "hb-fuzzer.hh"
#include <cstdio>
#include <hb.h>

static void fuzz_script_to_tag(const uint8_t *data, size_t size) {
  if (size < 4) return;
  hb_tag_t tag = HB_TAG(data[0], data[1], data[2], data[3]);
  hb_script_t script = hb_ot_tag_to_script(tag);
  hb_tag_t iso_tag = hb_script_to_iso15924_tag(script);

  hb_tag_t script_tag_1 = 0, script_tag_2 = 0;
  hb_ot_tags_from_script(script, &script_tag_1, &script_tag_2); // Deprecated
  (void)iso_tag;
}

static void fuzz_tag_language_conversion(const uint8_t *data, size_t size) {
  if (size <= 4) return;
  const char *lang_str = (const char *)(data + 4);
  int lang_len = size - 4;
  hb_language_t lang = hb_language_from_string(lang_str, lang_len);
  const char *back_str = hb_language_to_string(lang);
  hb_language_t lang_from_tag = hb_ot_tag_to_language(hb_tag_from_string(lang_str, lang_len));

  hb_tag_t lang_tag = hb_ot_tag_from_language(lang); // Deprecated
  hb_language_t lang_from_tag_2 = hb_ot_tag_to_language(lang_tag);

  (void)back_str;
  (void)lang_from_tag;
  (void)lang_from_tag_2;
}

static void fuzz_script_and_language_to_tags(const uint8_t *data, size_t size) {
  if (size < 4) return;
  hb_tag_t tag = HB_TAG(data[0], data[1], data[2], data[3]);
  hb_script_t script = hb_ot_tag_to_script(tag);

  const char *lang_str = (const char *)(data + 4);
  int lang_len = size - 4;
  hb_language_t lang = hb_language_from_string(lang_str, lang_len);

  hb_tag_t script_tags[HB_OT_MAX_TAGS_PER_SCRIPT] = {0};
  hb_tag_t language_tags[HB_OT_MAX_TAGS_PER_LANGUAGE] = {0};
  unsigned script_count = HB_OT_MAX_TAGS_PER_SCRIPT;
  unsigned lang_count = HB_OT_MAX_TAGS_PER_LANGUAGE;

  hb_ot_tags_from_script_and_language(script, lang,
                                      &script_count, script_tags,
                                      &lang_count, language_tags);
}

static void fuzz_tags_to_script_and_language(const uint8_t *data, size_t size) {
  if (size < 8) return;
  hb_tag_t script_tag = HB_TAG(data[0], data[1], data[2], data[3]);
  hb_tag_t lang_tag = HB_TAG(data[4], data[5], data[6], data[7]);

  hb_script_t script;
  hb_language_t lang;
  hb_ot_tags_to_script_and_language(script_tag, lang_tag, &script, &lang);
}

static void fuzz_tag_direction_and_defaults(const uint8_t *data, size_t size) {
  if (size < 4) return;
  hb_tag_t tag = HB_TAG(data[0], data[1], data[2], data[3]);
  hb_script_t script = hb_ot_tag_to_script(tag);
  hb_direction_t dir = hb_script_get_horizontal_direction(script);

  // test default script/language tags
  (void)HB_OT_TAG_DEFAULT_SCRIPT;
  (void)HB_OT_TAG_DEFAULT_LANGUAGE;
  (void)dir;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  fuzz_script_to_tag(data, size);
  fuzz_tag_language_conversion(data, size);
  fuzz_script_and_language_to_tags(data, size);
  fuzz_tags_to_script_and_language(data, size);
  fuzz_tag_direction_and_defaults(data, size);
  return 0;
}
