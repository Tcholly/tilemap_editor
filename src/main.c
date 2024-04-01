#include <stdio.h>
#include <raylib.h>
#include <raymath.h>

#include "tilemap.h"
#include "utils.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "external/cimgui.h"
#include "external/rlImGui.h"


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

	EndTextureMode();
}

void viewport_window(CoreData* data)
{
	igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0, 0});
	igBegin("Viewport", NULL, 0);

	ImVec2 viewport_size_min;
	ImVec2 viewport_size_max;
	ImVec2 viewport_pos;
	igGetWindowContentRegionMin(&viewport_size_min);
	igGetWindowContentRegionMax(&viewport_size_max);
	igGetWindowPos(&viewport_pos);
	data->viewport_bounds.x = viewport_pos.x + viewport_size_min.x;
	data->viewport_bounds.y = viewport_pos.y + viewport_size_min.y;
	data->viewport_bounds.width  = viewport_size_max.x - viewport_size_min.x;
	data->viewport_bounds.height = viewport_size_max.y - viewport_size_min.y;

	if ((int)data->viewport_bounds.width != data->viewport.texture.width || (int)data->viewport_bounds.height != data->viewport.texture.height)
	{
		UnloadRenderTexture(data->viewport);
		data->viewport = LoadRenderTexture(data->viewport_bounds.width, data->viewport_bounds.height);
	}

	// Render viewport
	draw_viewport(data);
	rlImGuiImageRenderTexture(&data->viewport);

	igEnd();
	igPopStyleVar(1);
}

void tile_selector_window(CoreData* data)
{
	igBegin("Tile select", NULL, ImGuiWindowFlags_None);

	ImVec2 window_size;
	igGetWindowSize(&window_size);

	ImVec2 item_size = {70.0f, 70.0f};
	float padding = 10.0f;
	ImVec2 spacing = igGetStyle()->ItemSpacing;

	int items_per_row = window_size.x / (item_size.x + spacing.x + 2 * igGetStyle()->CellPadding.x);
	if (items_per_row <= 0)
		items_per_row = 1;

	int to_remove = -1;
	for (size_t i = 0; i < data->tilemap.textures.size; i++)
	{
		int item_idx = i % items_per_row;
		if (item_idx != 0)
			igSameLine(0, -1);


		bool is_selected = i == data->current_texture;
		if (is_selected)
			igPushStyleColor_Vec4(ImGuiCol_Button, *igGetStyleColorVec4(ImGuiCol_ButtonActive));

		if (rlImGuiImageButtonSize(TextFormat("Tile %zu", i), &data->tilemap.textures.items[i], item_size))
			data->current_texture = i;

		// Context menu
		if (igBeginPopupContextItem(TextFormat("Tile %zu context", i), ImGuiPopupFlags_MouseButtonRight))
		{
			if (igMenuItem_Bool("Delete", NULL, false, true))
				to_remove = i;

			igEndPopup();
		}

		if (is_selected)
			igPopStyleColor(1);
	}

	igEnd(); // Tile selector

	if (to_remove >= 0 && to_remove < data->tilemap.textures.size)
		remove_texture(&data->tilemap, to_remove);
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
		if (mouse_in_viewport)
		{
			float mouse_wheel = GetMouseWheelMove();
			data.camera.zoom += mouse_wheel;
			if (data.camera.zoom <= 1.0f)
				data.camera.zoom = 1.0f;
		}

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


		BeginDrawing();
		ClearBackground(WHITE);

		// start ImGui Conent
		rlImGuiBegin();

		// Dockspace
		igDockSpaceOverViewport(NULL, ImGuiDockNodeFlags_None, NULL);

		// show ImGui Content
		// igShowDemoWindow(NULL);

		// Viewport
		viewport_window(&data);

		// Tile selector
		tile_selector_window(&data);

		// end ImGui Content
		rlImGuiEnd();

		EndDrawing();
	}
	
	unload_tileset(&data.tilemap);
	UnloadRenderTexture(data.viewport);
	
	rlImGuiShutdown();

	CloseWindow();

	return 0;
}
