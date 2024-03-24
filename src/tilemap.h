#pragma once

#include <raylib.h>
#include <stddef.h>

typedef struct
{
	int x, y;
} Vec2i;

typedef struct
{
	Texture2D* items;
	size_t size;
	size_t capacity;
} Textures2D;


typedef struct
{
	union
	{
		Vec2i tilemap_index;
		Rectangle bounds;
	};
	size_t texture_index;
	Color tint;
} Tile;

// TODO: Make into a hash_map<Vec2i, Tile>
typedef struct
{
	Tile* items;
	size_t size;
	size_t capacity;
} Tiles;

typedef struct
{
	Vector2 offset;
	Tiles tiles;
	Tiles static_tiles;
} Layer;

typedef struct
{
	Layer* items;
	size_t size;
	size_t capacity;
} Layers;

typedef struct
{
	Vector2 offset;
	Layer main_layer;
	Layers layers;

	Textures2D textures;
} Tilemap;

void add_tileset(Tilemap* tilemap, const char* filepath, int width, int height);

void draw_tilemap(const Tilemap* tilemap);

void unload_tileset(Tilemap* tilemap);
void unload_layer(Layer* layer);
void unload_tilemap(Tilemap* tilemap);

bool save_tilemap(const Tilemap* tilemap, const char* filepath);
Tilemap load_tilemap(const char* filepath);
