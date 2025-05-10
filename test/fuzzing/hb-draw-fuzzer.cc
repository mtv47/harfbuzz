#include <assert.h>
#include <stdlib.h>

#include <hb-ot.h>

#include "hb-fuzzer.hh"

struct _draw_data_t
{
  unsigned path_len;
  float path_start_x;
  float path_start_y;
  float path_last_x;
  float path_last_y;
};

#include <cstdio>
static void
_move_to (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data_,
	  hb_draw_state_t *st,
	  float to_x, float to_y,
	  void *user_data HB_UNUSED)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (!st->path_open);
  draw_data->path_start_x = draw_data->path_last_x = to_x;
  draw_data->path_start_y = draw_data->path_last_y = to_y;
}

static void
_line_to (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data_,
	  hb_draw_state_t *st,
	  float to_x, float to_y,
	  void *user_data HB_UNUSED)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (st->path_open);
  ++draw_data->path_len;
  draw_data->path_last_x = to_x;
  draw_data->path_last_y = to_y;
}

static void
_quadratic_to (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data_,
	       hb_draw_state_t *st,
	       float control_x HB_UNUSED, float control_y HB_UNUSED,
	       float to_x, float to_y,
	       void *user_data HB_UNUSED)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (st->path_open);
  ++draw_data->path_len;
  draw_data->path_last_x = to_x;
  draw_data->path_last_y = to_y;
}

static void
_cubic_to (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data_,
	   hb_draw_state_t *st,
	   float control1_x HB_UNUSED, float control1_y HB_UNUSED,
	   float control2_x HB_UNUSED, float control2_y HB_UNUSED,
	   float to_x, float to_y,
	   void *user_data HB_UNUSED)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (st->path_open);
  ++draw_data->path_len;
  draw_data->path_last_x = to_x;
  draw_data->path_last_y = to_y;
}

static void
_close_path (hb_draw_funcs_t *dfuncs HB_UNUSED, void *draw_data_,
	     hb_draw_state_t *st,
	     void *user_data HB_UNUSED)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (st->path_open && draw_data->path_len != 0);
  draw_data->path_len = 0;
  assert (draw_data->path_start_x == draw_data->path_last_x &&
	  draw_data->path_start_y == draw_data->path_last_y);
}

static void var_calls(const uint8_t *data, size_t size, hb_face_t *face,
                      hb_font_t *font, int* coords, unsigned numCoords) {
  hb_ot_var_has_data(face);

  unsigned int instance_count = hb_ot_var_get_named_instance_count(face);

  if (instance_count > 0) {
    unsigned int rand_index = data[0] % instance_count;
    hb_font_set_var_named_instance(font, rand_index);
    hb_font_get_var_named_instance(font);
    unsigned int num_coords_norm = numCoords;
    hb_font_get_var_coords_normalized(font, &num_coords_norm);
    if (size > 8)
    {
      const unsigned int max_tags = 4;
      hb_variation_t variations[max_tags];
      unsigned int var_count = size / 8;
      if (var_count > max_tags) var_count = max_tags;

      for (unsigned int i = 0; i < var_count; i++) {
        variations[i].tag = HB_TAG(data[i * 4 + 0], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
        variations[i].value = ((int8_t)data[i * 8 + 4]) / 64.0f;
        hb_font_set_variation(font, variations[i].tag, variations[i].value);
      }
      hb_font_set_variations(font, variations, var_count);
      hb_ot_var_normalize_variations(face, variations,
        var_count, coords, numCoords);
    }
  }
}

/* Similar to test-ot-face.c's #test_font() */
static void misc_calls_for_gid (hb_face_t *face, hb_font_t *font, hb_set_t *set, hb_codepoint_t cp)
{
  /* Other gid specific misc calls */
  hb_face_collect_variation_unicodes (face, cp, set);

  hb_codepoint_t g;
  hb_font_get_nominal_glyph (font, cp, &g);
  hb_font_get_variation_glyph (font, cp, cp, &g);
  hb_font_get_glyph_h_advance (font, cp);
  hb_font_get_glyph_v_advance (font, cp);
  hb_position_t x, y;
  hb_font_get_glyph_h_origin (font, cp, &x, &y);
  hb_font_get_glyph_v_origin (font, cp, &x, &y);
  hb_font_get_glyph_contour_point (font, cp, 0, &x, &y);
  char buf[64];
  hb_font_get_glyph_name (font, cp, buf, sizeof (buf));

  hb_ot_color_palette_get_name_id (face, cp);
  hb_ot_color_palette_color_get_name_id (face, cp);
  hb_ot_color_palette_get_flags (face, cp);
  hb_ot_color_palette_get_colors (face, cp, 0, nullptr, nullptr);
  hb_ot_color_glyph_get_layers (face, cp, 0, nullptr, nullptr);
  hb_blob_destroy (hb_ot_color_glyph_reference_svg (face, cp));
  hb_blob_destroy (hb_ot_color_glyph_reference_png (font, cp));

  hb_ot_layout_get_ligature_carets (font, HB_DIRECTION_LTR, cp, 0, nullptr, nullptr);

  hb_ot_math_get_glyph_italics_correction (font, cp);
  hb_ot_math_get_glyph_top_accent_attachment (font, cp);
  hb_ot_math_is_glyph_extended_shape (face, cp);
  hb_ot_math_get_glyph_kerning (font, cp, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0);
  hb_ot_math_get_glyph_variants (font, cp, HB_DIRECTION_TTB, 0, nullptr, nullptr);
  hb_ot_math_get_glyph_assembly (font, cp, HB_DIRECTION_BTT, 0, nullptr, nullptr, nullptr);
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  if (size >= 3) {
    // Generate baseline_tag directly from fuzz data (interpreted as HB_TAG)
    hb_ot_layout_baseline_tag_t tag = static_cast<hb_ot_layout_baseline_tag_t>(
      HB_TAG(data[0], data[1], data[2], data[3]));
    hb_ot_tag_to_language(tag);
  }
  return 0;
}
