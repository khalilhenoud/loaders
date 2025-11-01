/**
 * @file loader_map.h
 * @author khalilhenoud@gmail.com
 * @brief 
 * @version 0.1
 * @date 2024-01-29
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef LOADER_MAP_H
#define LOADER_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <loaders/internal/module.h>
#include <loaders/loader_map_data.h>


typedef struct allocator_t allocator_t;

LOADERS_API
loader_map_data_t*
load_map(const char* path, const allocator_t *allocator);

LOADERS_API
void
free_map(loader_map_data_t* data, const allocator_t *allocator);

#ifdef __cplusplus
}
#endif

#endif