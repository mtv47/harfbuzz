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

static hb_bool_t dummy_paint(hb_font_t*, void*, hb_codepoint_t, hb_paint_funcs_t*, void*, unsigned, hb_color_t, void*) {
  return true;
}

static void dummy_draw(hb_font_t*, void*, hb_codepoint_t, hb_draw_funcs_t*, void*, void*) {
  // Intentionally empty; only to trigger the function pointer path
}

// Try to cover TupleVariationHeader from hb-ot-var-common.hh
static void trigger_tuple_variation_header(hb_face_t *face, hb_font_t *font, const uint8_t *data, size_t size) {
  // Safety: needs at least one glyph
  if (size == 0) return;
  unsigned int glyph_count = hb_face_get_glyph_count(face);
  if (!glyph_count) return;

  hb_codepoint_t gid = data[0] % glyph_count;

  // Check for presence of gvar or cff2 tables
  bool has_gvar = false, has_cff2 = false;
  {
    hb_blob_t *blob = hb_face_reference_table(face, HB_TAG('g','v','a','r'));
    has_gvar = hb_blob_get_length(blob) > 0;
    hb_blob_destroy(blob);
  }
  {
    hb_blob_t *blob = hb_face_reference_table(face, HB_TAG('C','F','F','2'));
    has_cff2 = hb_blob_get_length(blob) > 0;
    hb_blob_destroy(blob);
  }

  if (!has_gvar && !has_cff2)
    return;

  // Set up draw funcs once
  hb_draw_funcs_t* draw_funcs = hb_draw_funcs_create();
  hb_draw_funcs_set_move_to_func(draw_funcs, _move_to, nullptr, nullptr);
  hb_draw_funcs_set_line_to_func(draw_funcs, _line_to, nullptr, nullptr);
  hb_draw_funcs_set_quadratic_to_func(draw_funcs, _quadratic_to, nullptr, nullptr);
  hb_draw_funcs_set_cubic_to_func(draw_funcs, _cubic_to, nullptr, nullptr);
  hb_draw_funcs_set_close_path_func(draw_funcs, _close_path, nullptr, nullptr);

  _draw_data_t draw_data = {0, 0, 0, 0, 0};
  hb_font_draw_glyph(font, gid, draw_funcs, &draw_data);
  hb_draw_funcs_destroy(draw_funcs);

  // Additional: call get_glyph_extents as another way to force gvar/cvar resolution
  hb_glyph_extents_t extents;
  hb_font_get_glyph_extents(font, gid, &extents);
}

static void additionals(hb_face_t *face, hb_font_t *font,  const uint8_t *data) {
  unsigned glyph_count = hb_face_get_glyph_count(face);
  if (glyph_count == 0) return;

  hb_font_extents_t dummy_extents;
  hb_font_get_h_extents(font, &dummy_extents);
  hb_font_get_v_extents(font, &dummy_extents);
  hb_codepoint_t cp = data[0] % glyph_count;

  hb_codepoint_t in[2] = { 'A', 'B' };
  hb_codepoint_t out[2];
  hb_font_get_nominal_glyphs(font, 2, in, sizeof(hb_codepoint_t), out, sizeof(hb_codepoint_t));

  hb_position_t adv[2];
  hb_font_get_glyph_h_advances(font, 2, in, sizeof(hb_codepoint_t), adv, sizeof(hb_position_t));

  hb_font_get_glyph_v_advances(font, 2, in, sizeof(hb_codepoint_t), adv, sizeof(hb_position_t));

  hb_font_get_glyph_h_kerning(font, cp, (cp + 1) % hb_face_get_glyph_count(face));
  hb_font_get_glyph_v_kerning(font, cp, (cp + 1) % hb_face_get_glyph_count(face));

  char name[64];
  hb_font_get_glyph_name(font, cp, name, sizeof(name));
  hb_font_get_glyph_from_name(font, name, -1, &cp);

  hb_font_funcs_t* custom_funcs = hb_font_funcs_create();
  hb_font_funcs_set_paint_glyph_func(custom_funcs, dummy_paint, nullptr, nullptr);
  hb_font_funcs_set_draw_glyph_func(custom_funcs, dummy_draw, nullptr, nullptr);
  hb_font_set_funcs(font, custom_funcs, nullptr, nullptr);
  hb_font_draw_glyph(font, cp, nullptr, nullptr); // triggers dummy_draw
  hb_font_funcs_destroy(custom_funcs);

#ifndef HB_NO_PAINT
  hb_paint_funcs_t* paint_funcs = hb_paint_funcs_create();
  hb_font_paint_glyph(font, cp, paint_funcs, nullptr, 0, 0xFF0000FF);
  hb_paint_funcs_destroy(paint_funcs);
#endif

  // Try to covers _default functions in hb-font.cc by using an unconfigured font
  hb_font_t *dummy_font = hb_font_create(face);
  hb_font_funcs_t *empty_funcs = hb_font_funcs_create();
  hb_font_set_funcs(dummy_font, empty_funcs, nullptr, nullptr);
  hb_font_funcs_destroy(empty_funcs);

  hb_codepoint_t cp_out;
  hb_position_t x, y;
  hb_glyph_extents_t extents;
  char name_d[64] = {};

  hb_font_get_nominal_glyph(dummy_font, cp, &cp_out);
  hb_font_get_variation_glyph(dummy_font, cp, cp, &cp_out);
  hb_font_get_glyph_h_advance(dummy_font, cp);
  hb_font_get_glyph_v_advance(dummy_font, cp);
  hb_font_get_glyph_h_origin(dummy_font, cp, &x, &y);
  hb_font_get_glyph_v_origin(dummy_font, cp, &x, &y);
  hb_font_get_glyph_contour_point(dummy_font, cp, 0, &x, &y);
  hb_font_get_glyph_extents(dummy_font, cp, &extents);
  hb_font_get_glyph_name(dummy_font, cp, name_d, sizeof(name_d));
  hb_font_get_glyph_from_name(dummy_font, name_d, -1, &cp_out);
  hb_font_get_glyph_h_kerning(dummy_font, cp, cp);
  hb_font_get_glyph_v_kerning(dummy_font, cp, cp);


  hb_draw_funcs_t* draw_funcs = hb_draw_funcs_create();
  hb_font_draw_glyph(dummy_font, cp, draw_funcs, nullptr);
  hb_draw_funcs_destroy(draw_funcs);

#ifndef HB_NO_PAINT
  hb_paint_funcs_t* paint_funcs2 = hb_paint_funcs_create();
  hb_font_paint_glyph(dummy_font, cp, paint_funcs2, nullptr, 0, 0xFFFFFFFF);
  hb_paint_funcs_destroy(paint_funcs2);
#endif
  hb_font_destroy(dummy_font);

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

#ifdef HB_ENABLE_DEPRECATED
  {
    unsigned int axis_count = 0;
    hb_ot_var_get_axes(face, 0, &axis_count, nullptr); // query count
    if (axis_count > 0 && axis_count <= 8) {
      hb_ot_var_axis_t axes[8];
      hb_ot_var_get_axes(face, 0, &axis_count, axes);
    }
  }
  {
    hb_ot_var_axis_t info;
    unsigned axis_index;
    // Use 'wght' as a common tag — data-driven would require bounds checks
    hb_ot_var_find_axis(face, HB_TAG('w','g','h','t'), &axis_index, &info);
  }
#endif
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
  alloc_state = _fuzzing_alloc_state (data, size);

  hb_blob_t *blob = hb_blob_create ((const char *) data, size,
				    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);

  unsigned num_coords = 0;
  if (size) num_coords = data[size - 1];
  num_coords = hb_ot_var_get_axis_count (face) > num_coords ? num_coords : hb_ot_var_get_axis_count (face);
  int *coords = (int *) calloc (num_coords, sizeof (int));
  if (size > num_coords + 1)
    for (unsigned i = 0; i < num_coords; ++i)
      coords[i] = ((int) data[size - num_coords + i - 1] - 128) * 10;
  hb_font_set_var_coords_normalized (font, coords, num_coords);
  var_calls(data, size, face, font, coords, num_coords);
  free (coords);

  unsigned glyph_count = hb_face_get_glyph_count (face);
  glyph_count = glyph_count > 16 ? 16 : glyph_count;

  _draw_data_t draw_data = {0, 0, 0, 0, 0};

  hb_draw_funcs_t *funcs = hb_draw_funcs_create ();
  hb_draw_funcs_set_move_to_func (funcs, (hb_draw_move_to_func_t) _move_to, nullptr, nullptr);
  hb_draw_funcs_set_line_to_func (funcs, (hb_draw_line_to_func_t) _line_to, nullptr, nullptr);
  hb_draw_funcs_set_quadratic_to_func (funcs, (hb_draw_quadratic_to_func_t) _quadratic_to, nullptr, nullptr);
  hb_draw_funcs_set_cubic_to_func (funcs, (hb_draw_cubic_to_func_t) _cubic_to, nullptr, nullptr);
  hb_draw_funcs_set_close_path_func (funcs, (hb_draw_close_path_func_t) _close_path, nullptr, nullptr);
  volatile unsigned counter = !glyph_count;
  hb_set_t *set = hb_set_create ();
  for (unsigned gid = 0; gid < glyph_count; ++gid)
  {
    hb_font_draw_glyph (font, gid, funcs, &draw_data);

    /* Glyph extents also may practices the similar path, call it now that is related */
    hb_glyph_extents_t extents;
    if (hb_font_get_glyph_extents (font, gid, &extents))
      counter += !!extents.width + !!extents.height + !!extents.x_bearing + !!extents.y_bearing;

    if (!counter) counter += 1;

    /* other misc calls */
    misc_calls_for_gid (face, font, set, gid);
  }
  hb_set_destroy (set);
  assert (counter);
  hb_draw_funcs_destroy (funcs);

  additionals(face, font, data);
  trigger_tuple_variation_header(face, font, data, size);

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);
  return 0;
}