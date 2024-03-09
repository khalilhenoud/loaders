/**
 * @file loader_map.c
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-29
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <library/allocator/allocator.h>
#include <loaders/loader_map.h>
#include <loaders/utils.h>


// TODO: Support the rest of the entities in the map file.
/*
typedef
struct {
  point3f points[3];
} brush_face_data_t;

typedef
struct {
  uint32_t plane_count;
  brush_face_data_t* faces;
} loader_map_brush_data_t;

typedef
struct {
  uint32_t brush_count;
  loader_map_brush_data_t* brushes;
} loader_map_shape_t;

typedef
struct {
  loader_map_shape_t world;
  point3f player_start;
} loader_map_data_t;
*/

// invalid if start = NULL.
typedef 
struct {
  const char* start;
  size_t size;
} chunk_t;

typedef chunk_t sub_chunk_t;

inline
uint32_t
within(const chunk_t* chunk, const char* ptr)
{
  return ptr < (chunk->start + chunk->size) && ptr >= chunk->start;
}

static
chunk_t
read_chunk(
  const chunk_t *outer, 
  uintptr_t offset)
{
  chunk_t result = { NULL, 0};
  chunk_t chunk = { outer->start + offset, outer->size - offset};
  const char *open_bracket = strchr(chunk.start, '{');
  if (!open_bracket || open_bracket > (outer->start + outer->size))
    return result;

  int32_t count = 1;
	char ch;
  const char *end_bracket = open_bracket++;
	while (count) {
		++end_bracket;
    end_bracket = strpbrk(end_bracket, "{}");
		ch = *end_bracket;

		if (ch == '{')
			++count;
		if (ch == '}')
			--count;
	}

  // invalid.
  if (end_bracket > (chunk.start + chunk.size))
    return result;

  // valid result.
  result.start = open_bracket;
  result.size = (size_t)(end_bracket - result.start);
  return result;
}

static
chunk_t
read_sub_chunk(
  const chunk_t *outer, 
  const chunk_t *previous)
{
  assert(within(outer, previous->start));
  return read_chunk(outer, previous->start + previous->size + 1 - outer->start);
}

inline
uint32_t
is_valid(const chunk_t* chunk)
{
  return chunk->start != NULL;
}

inline
uint32_t
chunk_within(const chunk_t* chunk, const chunk_t* sub_chunk)
{
  return 
    (sub_chunk->start < (chunk->start + chunk->size) && 
      sub_chunk->start >= chunk->start) && 
    ((sub_chunk->start + sub_chunk->size) < (chunk->start + chunk->size) && 
      (sub_chunk->start + sub_chunk->size) >= chunk->start);
}

static
uint32_t
has_label(const chunk_t* chunk, const char* label)
{
  const char* start = strstr(chunk->start, label);
  return within(chunk, start);
}

chunk_t
find_chunk( 
  const chunk_t *content, 
  const char *label)
{
  chunk_t current = read_chunk(content, 0);
  while (is_valid(&current)) {
    if (has_label(&current, label))
      return current;

    current = read_chunk(
      content, (current.start + current.size + 1 - content->start));
  }

  return current;
}

static
void
free_brush(
  loader_map_brush_data_t* brush_data, 
  const allocator_t* allocator)
{
  allocator->mem_free(brush_data->faces);
}

static
void
read_brush(
  loader_map_brush_data_t* brush_data, 
  const chunk_t* brush, 
  const allocator_t* allocator)
{
  brush_data->face_count = 0;

  {   
    const char *start = brush->start;
    const char *end = strchr(start, '\n');
    while (within(brush, end)) {
      if (*start == '(')
        ++brush_data->face_count;
      
      start = end + 1;
      end = strchr(start, '\n');
    }

    brush_data->faces = 
      (brush_face_data_t*)allocator->mem_cont_alloc(
        brush_data->face_count, sizeof(brush_face_data_t));
    memset(
      brush_data->faces, 0, brush_data->face_count * sizeof(brush_face_data_t));

    {
      // populate the data used by each face.
      uint32_t i = 0;
      start = brush->start;
      end = strchr(start, '\n');
      while (within(brush, end)) {
        if (*start == '(') {
          brush_face_data_t* face = brush_data->faces + i++;
          // we need to read 3 vertices. ( x y z ) (...) (...)
          sscanf(
            start, 
            "( %i %i %i ) ( %i %i %i ) ( %i %i %i ) %s %i %i %i %f %f", 
            face->data + 0, face->data + 1, face->data + 2, 
            face->data + 3, face->data + 4, face->data + 5, 
            face->data + 6, face->data + 7, face->data + 8,
            face->texture, face->offset, face->offset + 1, &face->rotation,
            face->scale, face->scale + 1);
        }
        
        start = end + 1;
        end = strchr(start, '\n');
      }
    }
  }
}

static
void
free_world_data(
  loader_map_data_t* map_data, 
  const allocator_t* allocator)
{
  for (uint32_t i = 0, count = map_data->world.brush_count; i < count; ++i)
    free_brush(map_data->world.brushes + i, allocator);

  allocator->mem_free(map_data->world.brushes);
}

static
void
read_wad(
  loader_map_data_t* map_data, 
  const chunk_t* world, 
  const allocator_t* allocator)
{
  assert(has_label(world, "wad") != 0);

  {   
    const char *start = world->start;
    const char *end = strchr(start, '\n');
    while (within(world, end)) {
      int32_t read = sscanf(start, "\"wad\" \"%s\"", map_data->world.wad);
      if (read == 1)
        break;

      start = end + 1;
      end = strchr(start, '\n');
    }
  }
}

static
void
read_world_data(
  loader_map_data_t* map_data, 
  const chunk_t* world, 
  const allocator_t* allocator)
{
  chunk_t brush = { world->start, 0 };
  uint32_t brush_count = 0;
  brush = read_sub_chunk(world, &brush);
  while (chunk_within(world, &brush) && is_valid(&brush) && ++brush_count)
    brush = read_sub_chunk(world, &brush);

  read_wad(map_data, world, allocator);

  map_data->world.brush_count = brush_count;
  map_data->world.brushes = 
    (loader_map_brush_data_t *)allocator->mem_cont_alloc(
      brush_count, 
      sizeof(loader_map_brush_data_t));

  {
    // parse every brush, we need to know the number of planes the brush makes.
    uint32_t i = 0;
    chunk_t start = { world->start, 0 };
    brush = read_sub_chunk(world, &start);
    while (chunk_within(world, &brush) && is_valid(&brush)) {
      read_brush(map_data->world.brushes + i, &brush, allocator);
      brush = read_sub_chunk(world, &brush);
      i++;
    } 
  }
}

static
void
read_player_start(
  loader_map_data_t* map_data, 
  const chunk_t* player_start, 
  const allocator_t* allocator)
{
  map_data->player_angle = 0;
  map_data->player_start[0] = 
  map_data->player_start[1] = 
  map_data->player_start[2] = 0; 

  assert(has_label(player_start, "origin") != 0);
  assert(has_label(player_start, "angle") != 0);

  {   
    const char *start = player_start->start;
    const char *end = strchr(start, '\n');
    while (within(player_start, end)) {
      sscanf(
        start, "\"origin\" \"%i %i %i\"", 
        map_data->player_start + 0, 
        map_data->player_start + 1, 
        map_data->player_start + 2);
      sscanf(
        start, "\"angle\" \"%i\"", 
        &map_data->player_angle);

      start = end + 1;
      end = strchr(start, '\n');
    }
  }
}

loader_map_data_t*
read_map(const chunk_t *content, const allocator_t* allocator)
{
  loader_map_data_t* map_data = 
    (loader_map_data_t*)allocator->mem_alloc(sizeof(loader_map_data_t));
  memset(map_data, 0, sizeof(loader_map_data_t));

  {
    chunk_t chunk = find_chunk(content, "worldspawn");
    assert(is_valid(&chunk));

    read_world_data(map_data, &chunk, allocator);
  }

  {
    chunk_t chunk = find_chunk(content, "info_player_start");
    assert(is_valid(&chunk));

    read_player_start(map_data, &chunk, allocator);
  }

  return map_data;
}

// TODO: We need a loader unit test for map.
loader_map_data_t*
load_map(const char* path, const allocator_t *allocator)
{
  assert(path && allocator);

  {    
    size_t file_size = 0;
    const char* data = read_file_as_ascii(path, allocator, &file_size);
    chunk_t content = { data, file_size };
    loader_map_data_t* map_data = read_map(&content, allocator);
    allocator->mem_free((void*)data);
    return map_data;
  }
}

void
free_map(loader_map_data_t* data, const allocator_t *allocator)
{
  assert(data && allocator);

  free_world_data(data, allocator);
  allocator->mem_free(data);
}
