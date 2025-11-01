/**
 * @file loader_map_data.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-29
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef LOADER_MAP_DATA_H
#define LOADER_MAP_DATA_H

#include <stdint.h>


// The plane points must be arranged such that the cross product of the vectors 
// (p3 - p1) and (p2 - p1) is not null
typedef
struct brush_face_data_t {
  int32_t data[9];
  char texture[24];
  int32_t offset[2];
  int32_t rotation;
  float scale[2];
} brush_face_data_t;

typedef
struct loader_map_brush_data_t {
  uint32_t face_count;
  brush_face_data_t* faces;
} loader_map_brush_data_t;

typedef
struct loader_map_entity_t {
  uint32_t brush_count;
  loader_map_brush_data_t* brushes;
  char wad[64];
} loader_map_entity_t;

typedef
struct loader_map_light_data_t {
  int32_t origin[3];
  int32_t light;  // default is 200
} loader_map_light_data_t;

typedef
struct loader_map_light_t {
  uint32_t count;
  loader_map_light_data_t* lights;
} loader_map_light_t;

typedef
struct loader_map_data_t {
  loader_map_entity_t world;
  loader_map_light_t lights;
  int32_t player_start[3];
  int32_t player_angle;
} loader_map_data_t;

#endif