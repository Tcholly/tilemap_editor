#include "tilemap.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <raylib.h>

#include "utils.h"

static Rectangle get_tile_rect(const Tilemap* tilemap, const Layer* layer, Tile tile, bool is_static)
{
	if (is_static)
	{
		Rectangle result = tile.bounds;
		result.x += tilemap->offset.x + layer->offset.x;
		result.y += tilemap->offset.y + layer->offset.y;

		return result;
	}

	Rectangle result =
	{
		.x = tilemap->offset.x + layer->offset.x + tile.tilemap_index.x,
		.y = tilemap->offset.y + layer->offset.y + tile.tilemap_index.y,
		.width = 1.0f,
		.height = 1.0f,
	};

	return result;
}

static void draw_layer(const Tilemap* tilemap, const Layer* layer)
{
	for (size_t i = 0; i < layer->tiles.size; i++)
	{
		Tile tile = layer->tiles.items[i];
		Texture2D texture = tilemap->textures.items[tile.texture_index];
    	Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = get_tile_rect(tilemap, layer, tile, false);
		DrawTexturePro(texture, source, dest, (Vector2){0.0f, 0.0f}, 0.0f, tile.tint);
	}

	for (size_t i = 0; i < layer->static_tiles.size; i++)
	{
		Tile tile = layer->static_tiles.items[i];
		Texture2D texture = tilemap->textures.items[tile.texture_index];
    	Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = get_tile_rect(tilemap, layer, tile, true);
		DrawTexturePro(texture, source, dest, (Vector2){0.0f, 0.0f}, 0.0f, tile.tint);
	}
}


void draw_tilemap(const Tilemap* tilemap)
{
	for (size_t i = 0; i < tilemap->layers.size; i++)
		draw_layer(tilemap, &tilemap->layers.items[i]);

	draw_layer(tilemap, &tilemap->main_layer);
}

void add_tileset(Tilemap* tilemap, const char* filepath, int width, int height)
{
	Image tileset = LoadImage(filepath);
	
	if (tileset.width % width != 0 || tileset.height % height != 0)
	{
		fprintf(stderr, "ERROR: tileset [%s] is not divisible in %dx%d tiles", filepath, width, height);
		goto defer_return;
	}

	int tile_width = tileset.width / width;
	int tile_height = tileset.height / height;

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			Rectangle tile_rect =
			{
				.x = i * tile_width,
				.y = j * tile_height,
				.width = tile_width,
				.height = tile_height,
			};

			Image tile_image = ImageFromImage(tileset, tile_rect);
			Texture tile_texture = LoadTextureFromImage(tile_image);

			da_append(tilemap->textures, tile_texture);

			UnloadImage(tile_image);
		}
	}
	
defer_return:
	UnloadImage(tileset);
}

static void remove_texture_from_layer(Layer* layer, size_t texture_index)
{
	if (!layer)
		return;

	// Normal tiles
	for (size_t i = 0; i < layer->tiles.size; i++)
	{
		Tile* tile = &layer->tiles.items[i];
		if (tile->texture_index == texture_index)
		{
			da_remove_at(layer->tiles, i);
			i--;
		}
		else if (tile->texture_index > texture_index)
			tile->texture_index--;
	}

	// Static tiles
	for (size_t i = 0; i < layer->static_tiles.size; i++)
	{
		Tile* tile = &layer->static_tiles.items[i];
		if (tile->texture_index == texture_index)
		{
			da_remove_at(layer->static_tiles, i);
			i--;
		}
		else if (tile->texture_index > texture_index)
			tile->texture_index--;
	}
}

void remove_texture(Tilemap* tilemap, size_t texture_index)
{
	if (!tilemap)
		return;

	if (texture_index >= tilemap->textures.size)
		return;

	remove_texture_from_layer(&tilemap->main_layer, texture_index);

	for (size_t i = 0; i < tilemap->layers.size; i++)
		remove_texture_from_layer(&tilemap->layers.items[i], texture_index);

	UnloadTexture(tilemap->textures.items[texture_index]);
	da_remove_at_keep_order(tilemap->textures, texture_index);
}

void unload_tileset(Tilemap* tilemap)
{
	for (size_t i = 0; i < tilemap->textures.size; i++)
		UnloadTexture(tilemap->textures.items[i]);

	tilemap->textures.size = 0;
}

void unload_layer(Layer* layer)
{
	// Vector2 offset;
	layer->offset = (Vector2){0.0f, 0.0f};

	// Tiles tiles;
	free(layer->tiles.items);
	layer->tiles.size = 0;
	layer->tiles.capacity = 0;

	// Tiles static_tiles;
	free(layer->static_tiles.items);
	layer->static_tiles.size = 0;
	layer->static_tiles.capacity = 0;
}

void unload_tilemap(Tilemap* tilemap)
{
	// Offset
	tilemap->offset = (Vector2){0.0f, 0.0f};

	// main_layer;
	unload_layer(&tilemap->main_layer);
	
	// layers;
	for (size_t i = 0; i < tilemap->layers.size; i++)
		unload_layer(&tilemap->layers.items[i]);
	free(tilemap->layers.items);
	tilemap->layers.capacity = 0;
	tilemap->layers.size = 0;

	// Textures
	unload_tileset(tilemap);
	free(tilemap->textures.items);
	tilemap->textures.capacity = 0;
}



// TODO: make save and load system architecture independent
static const char* MAGIC = "MIAU";
static void write_vector2(FILE* file, const Vector2 value)
{
	if (!file)
		return;

	fwrite(&value.x, sizeof(value.x), 1, file);
	fwrite(&value.y, sizeof(value.y), 1, file);
}

static void write_vector2i(FILE* file, const Vec2i value)
{
	if (!file)
		return;

	fwrite(&value.x, sizeof(value.x), 1, file);
	fwrite(&value.y, sizeof(value.y), 1, file);
}

static void write_rectangle(FILE* file, const Rectangle rect)
{
	if (!file)
		return;

	fwrite(&rect.x, sizeof(rect.x), 1, file);
	fwrite(&rect.y, sizeof(rect.y), 1, file);
	fwrite(&rect.width, sizeof(rect.width), 1, file);
	fwrite(&rect.height, sizeof(rect.height), 1, file);
}

static void write_color(FILE* file, const Color color)
{
	if (!file)
		return;

	fwrite(&color.r, sizeof(color.r), 1, file);
	fwrite(&color.g, sizeof(color.g), 1, file);
	fwrite(&color.b, sizeof(color.b), 1, file);
	fwrite(&color.a, sizeof(color.a), 1, file);
}


static void write_tile(FILE* file, const Tile tile, bool is_static)
{
	if (!file)
		return;

	
	// tilemap_index;
	// bounds;
	if (is_static)
		write_rectangle(file, tile.bounds);
	else
		write_vector2i(file, tile.tilemap_index);
	
	// size_t texture_index;
	
	fwrite(&tile.texture_index, sizeof(tile.texture_index), 1, file);

	// Color tint;
	write_color(file, tile.tint);
}

static void write_layer(FILE* file, const Layer layer)
{
	if (!file)
		return;

	// Offset;
	write_vector2(file, layer.offset);

	// Tiles;
	fwrite(&layer.tiles.size, sizeof(layer.tiles.size), 1, file);
	for (size_t i = 0; i < layer.tiles.size; i++)
		write_tile(file, layer.tiles.items[i], false);


	// Static tiles;
	fwrite(&layer.static_tiles.size, sizeof(layer.static_tiles.size), 1, file);
	for (size_t i = 0; i < layer.static_tiles.size; i++)
		write_tile(file, layer.static_tiles.items[i], true);

}

static void write_texture(FILE* file, Texture2D texture)
{
	if (!file)
		return;

	Image image = LoadImageFromTexture(texture);

	fwrite(&image.width, sizeof(image.width), 1, file);
	fwrite(&image.height, sizeof(image.height), 1, file);
	fwrite(&image.format, sizeof(image.format), 1, file);

	size_t size = GetPixelDataSize(image.width, image.height, image.format);
	fwrite(image.data, size, 1, file);

	UnloadImage(image);
}



bool save_tilemap(const Tilemap* tilemap, const char* filepath)
{
	if (!tilemap)
		return false;

	FILE* output = fopen(filepath, "w");
	if (!output)
	{
		fprintf(stderr, "Could not open %s: %s\n", filepath, strerror(errno)); 
		return false;
	}

	// TODO: check all return value
	fwrite(MAGIC, 1, strlen(MAGIC), output);

	// Offset
	write_vector2(output, tilemap->offset);

	// Main layer
	write_layer(output, tilemap->main_layer);

	// Layers
	fwrite(&tilemap->layers.size, sizeof(tilemap->layers.size), 1, output);
	for (size_t i = 0; i < tilemap->layers.size; i++)
		write_layer(output, tilemap->layers.items[i]);

	// Textures
	fwrite(&tilemap->textures.size, sizeof(tilemap->textures.size), 1, output);
	for (size_t i = 0; i < tilemap->textures.size; i++)
		write_texture(output, tilemap->textures.items[i]);

	fclose(output);

	return true;
}

static Vector2 read_vector2(FILE* file)
{
	Vector2 result = {0};
	if (!file)
		return result;

	fread(&result.x, sizeof(result.x), 1, file);
	fread(&result.y, sizeof(result.y), 1, file);

	return result;
}

static Vec2i read_vector2i(FILE* file)
{
	Vec2i result = {0};
	if (!file)
		return result;

	fread(&result.x, sizeof(result.x), 1, file);
	fread(&result.y, sizeof(result.y), 1, file);

	return result;
}

static Rectangle read_rectangle(FILE* file)
{
	Rectangle result = {0};
	if (!file)
		return result;

	fread(&result.x, sizeof(result.x), 1, file);
	fread(&result.y, sizeof(result.y), 1, file);
	fread(&result.width, sizeof(result.width), 1, file);
	fread(&result.height, sizeof(result.height), 1, file);

	return result;
}

static Color read_color(FILE* file)
{
	Color result = {0};
	if (!file)
		return result;

	fread(&result.r, sizeof(result.r), 1, file);
	fread(&result.g, sizeof(result.g), 1, file);
	fread(&result.b, sizeof(result.b), 1, file);
	fread(&result.a, sizeof(result.a), 1, file);

	return result;
}

static Tile read_tile(FILE* file, bool is_static)
{
	Tile result = {0};
	if (!file)
		return result;

	// tilemap_index;
	// bounds;
	if (is_static)
		result.bounds = read_rectangle(file);
	else
		result.tilemap_index = read_vector2i(file);
	
	// size_t texture_index;
	fread(&result.texture_index, sizeof(result.texture_index), 1, file);

	// Color tint;
	result.tint = read_color(file);

	return result;
}

static Layer read_layer(FILE* file)
{
	Layer result = {0};
	if (!file)
		return result;

	// Offset;
	result.offset = read_vector2(file);

	size_t amount;
	// Tiles;
	fread(&amount, sizeof(amount), 1, file);
	for (size_t i = 0; i < amount; i++)
	{
		Tile tile = read_tile(file, false);
		da_append(result.tiles, tile);
	}


	// Static tiles;
	fread(&amount, sizeof(amount), 1, file);
	for (size_t i = 0; i < amount; i++)
	{
		Tile tile = read_tile(file, true);
		da_append(result.static_tiles, tile);
	}

	return result;
}

static Texture2D read_texture(FILE* file)
{
	Texture2D result = {0};
	if (!file)
		return result;

	Image image = { .mipmaps = 1 };
	fread(&image.width, sizeof(image.width), 1, file);
	fread(&image.height, sizeof(image.height), 1, file);
	fread(&image.format, sizeof(image.format), 1, file);

	size_t size = GetPixelDataSize(image.width, image.height, image.format);
	image.data = malloc(size);
	if (!image.data)
		fprintf(stderr, "ERROR: Could not allocate enough space\n");
	fread(image.data, size, 1, file);

	result = LoadTextureFromImage(image);

	free(image.data);

	return result;
}

Tilemap load_tilemap(const char* filepath)
{
	Tilemap result = {0};

	FILE* input = fopen(filepath, "r");
	if (!input)
	{
		fprintf(stderr, "ERROR: Could not open %s: %s\n", filepath, strerror(errno)); 
		return result;
	}

	char magic[5] = {0};
	fread(magic, 1, strlen(MAGIC), input);
	if (strcmp(magic, MAGIC) != 0)
	{
		fprintf(stderr, "ERROR: The format of the file is not correct: expected magic: \"%s\", got \"%s\"\n", MAGIC, magic);
		goto return_defer;
	}

	// Offset
	result.offset = read_vector2(input);

	// Main layer
	result.main_layer = read_layer(input);

	size_t amount;
	// Layers
	fread(&amount, sizeof(amount), 1, input);
	for (size_t i = 0; i < amount; i++)
	{
		Layer layer = read_layer(input);
		da_append(result.layers, layer);
	}

	// Textures
	fread(&amount, sizeof(amount), 1, input);
	for (size_t i = 0; i < amount; i++)
	{
		Texture2D texture = read_texture(input);
		da_append(result.textures, texture);
	}
	

return_defer:
	fclose(input);
	return result;
}
