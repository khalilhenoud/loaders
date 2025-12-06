/**
 * @file loader_csv.h
 * @author khalilhenoud@gmail.com
 * @brief
 * @version 0.1
 * @date 2023-01-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef LOADER_CSV_H
#define LOADER_CSV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <loaders/internal/module.h>

#define GLYPH_COUNT 256


typedef struct allocator_t allocator_t;

typedef
struct glyph_data_t {
  uint32_t x, y;
  uint32_t width, offset;
} glyph_data_t;

typedef
struct bounds_t {
  float data[6];
} bounds_t;

typedef
struct loader_csv_font_data_t {
  uint32_t image_width, image_height;
  uint32_t cell_width, cell_height;
  uint32_t font_height, font_width;
  uint32_t start_char;
  glyph_data_t glyphs[GLYPH_COUNT];
  bounds_t bounds[GLYPH_COUNT];
} loader_csv_font_data_t;

LOADERS_API
loader_csv_font_data_t*
load_csv(const char* path, const allocator_t* allocator);

LOADERS_API
void
free_csv(loader_csv_font_data_t* data, const allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif