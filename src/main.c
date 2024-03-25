#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

#include "tilemap.h"
#include "utils.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "rlImGui.h"


#define CAMERA_SPEED 30


typedef struct
{
	Camera2D camera;
	Tilemap tilemap;
	size_t current_texture;

	Rectangle viewport_bounds;
	RenderTexture2D viewport;
} CoreData;

int get_tile_index_collides_with_mouse(CoreData* data, const Layer* layer)
{
	if (!layer || !data)
		return -1;

	Vector2 mouse_pos = GetMousePosition();

	mouse_pos.x -= data->viewport_bounds.x;
	mouse_pos.y -= data->viewport_bounds.y;
	
	mouse_pos = GetScreenToWorld2D(mouse_pos, data->camera);

	for (size_t i = 0; i < layer->tiles.size; i++)
	{
		Rectangle tile_rect =
		{
			.x = layer->offset.x + layer->tiles.items[i].tilemap_index.x,
			.y = layer->offset.y + layer->tiles.items[i].tilemap_index.y,
			.width = 1.0f,
			.height = 1.0f,
		};

		if (CheckCollisionPointRec(mouse_pos, tile_rect))
			return i;
	}

	return -1;
}

void draw_viewport(CoreData* data)
{
	BeginTextureMode(data->viewport);
	ClearBackground(WHITE);

	BeginMode2D(data->camera);
	draw_tilemap(&data->tilemap);
	EndMode2D();

	if (data->current_texture >= 0 && data->current_texture < data->tilemap.textures.size)
	{
		Texture2D texture = data->tilemap.textures.items[data->current_texture];
    	Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
		Rectangle dest = {50.0f, 50.0f, 100.0f, 100.0f};
		DrawTexturePro(texture, source, dest, Vector2Zero(), 0.0f, (Color){255, 255, 255, 150});
	}

	EndTextureMode();
}

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Tilemap editor");
	SetTargetFPS(60);
	rlImGuiSetup(true);

	ImGuiIO* io = igGetIO();
	io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	CoreData data =
	{
		.camera.zoom = 100.0f,
		.viewport = LoadRenderTexture(800, 480),
	};

	add_tileset(&data.tilemap, "assets/Mossy - TileSet.png", 7, 7);

	while (!WindowShouldClose())
	{
		bool mouse_in_viewport = CheckCollisionPointRec(GetMousePosition(), data.viewport_bounds);

		float dt = GetFrameTime();

		Vector2 movement = {0};

		if (IsKeyDown(KEY_D))
			movement.x += 1.0f;
		if (IsKeyDown(KEY_A))
			movement.x -= 1.0f;

		if (IsKeyDown(KEY_S))
			movement.y += 1.0f;
		if (IsKeyDown(KEY_W))
			movement.y -= 1.0f;

		data.camera.target.x += movement.x * CAMERA_SPEED * dt;
		data.camera.target.y += movement.y * CAMERA_SPEED * dt;

		// TODO: set zoom position to mouse
		float mouse_wheel = GetMouseWheelMove();
		data.camera.zoom += mouse_wheel;
		if (data.camera.zoom <= 1.0f)
			data.camera.zoom = 1.0f;

		if (mouse_in_viewport && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && data.tilemap.textures.size > 0)
		{
			int collision_index = get_tile_index_collides_with_mouse(&data, &data.tilemap.main_layer);
			if (collision_index >= 0)
			{
				if (data.tilemap.main_layer.tiles.items[collision_index].texture_index != data.current_texture)
				{
					da_remove_at(data.tilemap.main_layer.tiles, collision_index);
					collision_index = -1;
				}
			}

			if (collision_index < 0)
			{
				Vector2 mouse_pos = GetMousePosition();
				mouse_pos.x -= data.viewport_bounds.x;
				mouse_pos.y -= data.viewport_bounds.y;
				mouse_pos = GetScreenToWorld2D(mouse_pos, data.camera);
				Tile tile =
				{
					.tilemap_index = { mouse_pos.x, mouse_pos.y },
					.texture_index = data.current_texture,
					.tint = WHITE,
				};

				if (mouse_pos.x < 0)
					tile.tilemap_index.x -= 1;
				if (mouse_pos.y < 0)
					tile.tilemap_index.y -= 1;

				da_append(data.tilemap.main_layer.tiles, tile);
			}
		}

		if (mouse_in_viewport && IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
		{
			int collision_index = get_tile_index_collides_with_mouse(&data, &data.tilemap.main_layer);
			if (collision_index >= 0)
				da_remove_at(data.tilemap.main_layer.tiles, collision_index);
		}

		if (IsKeyPressedRepeat(KEY_LEFT) || IsKeyPressed(KEY_LEFT))
		{
			if (data.current_texture == 0)
				data.current_texture = data.tilemap.textures.size - 1;
			else
				data.current_texture--;
		}
		if (IsKeyPressedRepeat(KEY_RIGHT) || IsKeyPressed(KEY_RIGHT))
			data.current_texture++;

		if (data.tilemap.textures.size > 0)
			data.current_texture %= data.tilemap.textures.size;
		else
			data.current_texture = 0;

		bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
		if (IsKeyPressed(KEY_S) && control)
			save_tilemap(&data.tilemap, "out.dat");
		if (IsKeyPressed(KEY_L) && control)
		{
			unload_tilemap(&data.tilemap);
			data.tilemap = load_tilemap("out.dat");
		}


		// Render viewport

		BeginDrawing();
		ClearBackground(WHITE);

		// start ImGui Conent
		rlImGuiBegin();

		igDockSpaceOverViewport(NULL, ImGuiDockNodeFlags_None, NULL);

		// show ImGui Content
		igShowDemoWindow(NULL);

		// Viewport
		igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0, 0});
		igBegin("Viewport", NULL, 0);

		// Yellow is content region min/max
		
		ImVec2 viewport_size_min;
		ImVec2 viewport_size_max;
		ImVec2 viewport_pos;
		igGetWindowContentRegionMin(&viewport_size_min);
		igGetWindowContentRegionMax(&viewport_size_max);
		igGetWindowPos(&viewport_pos);
		data.viewport_bounds.x = viewport_pos.x + viewport_size_min.x;
		data.viewport_bounds.y = viewport_pos.y + viewport_size_min.y;
		data.viewport_bounds.width  = viewport_size_max.x - viewport_size_min.x;
		data.viewport_bounds.height = viewport_size_max.y - viewport_size_min.y;

		if ((int)data.viewport_bounds.width != data.viewport.texture.width || (int)data.viewport_bounds.height != data.viewport.texture.height)
		{
			UnloadRenderTexture(data.viewport);
			data.viewport = LoadRenderTexture(data.viewport_bounds.width, data.viewport_bounds.height);
		}

		draw_viewport(&data);
		rlImGuiImageRenderTexture(&data.viewport);

		igEnd();
		igPopStyleVar(1);

		// end ImGui Content
		rlImGuiEnd();

		DrawRectangleLinesEx(data.viewport_bounds, 2.0f, RED);


		EndDrawing();
	}
	
	unload_tileset(&data.tilemap);
	UnloadRenderTexture(data.viewport);
	
	rlImGuiShutdown();

	CloseWindow();

	return 0;
}
