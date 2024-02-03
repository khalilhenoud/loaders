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

static
chunk_t
read_chunk(const chunk_t *outer, uintptr_t offset)
{
  chunk_t result = { NULL, 0};
  chunk_t chunk = { outer->start + offset, outer->size };
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

inline
uint32_t
is_valid(const chunk_t* chunk)
{
  return chunk->start != NULL;
}

inline
uint32_t
within(const chunk_t* chunk, const char* ptr)
{
  return ptr < (chunk->start + chunk->size) && ptr >= chunk->start;
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

    current = read_chunk(content, (uintptr_t)current.start + current.size);
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

    {
      // populate the data used by each face.
      uint32_t i = 0;
      start = brush->start;
      end = strchr(start, '\n');
      while (within(brush, end)) {
        if (*start == '(') {
          int32_t* data = brush_data->faces[i++].data;
          // we need to read 3 vertices. ( x y z ) (...) (...)
          sscanf(
            start, 
            "( %i %i %i ) ( %i %i %i ) ( %i %i %i )", 
            data + 0, data + 1, data + 2, 
            data + 3, data + 4, data + 5, 
            data + 6, data + 7, data + 8);
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
read_world_data(
  loader_map_data_t* map_data, 
  const chunk_t* world, 
  const allocator_t* allocator)
{
  chunk_t brush;
  uint32_t brush_count = 0;
  brush = read_chunk(world, 1);
  while (chunk_within(world, &brush) && is_valid(&brush) && ++brush_count)
    brush = read_chunk(
      world, (uintptr_t)brush.start + brush.size - (uintptr_t)world->start);

  map_data->world.brush_count = brush_count;
  map_data->world.brushes = 
    (loader_map_brush_data_t *)allocator->mem_cont_alloc(
      brush_count, 
      sizeof(loader_map_brush_data_t));

  {
    // parse every brush, we need to know the number of planes the brush makes.
    uint32_t i = 0;
    brush = read_chunk(world, 1);
    while (chunk_within(world, &brush) && is_valid(&brush)) {
      read_brush(map_data->world.brushes + i, &brush, allocator);
      brush = read_chunk(
        world, (uintptr_t)brush.start + brush.size - (uintptr_t)world->start);
      i++;
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

  return map_data;
}

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