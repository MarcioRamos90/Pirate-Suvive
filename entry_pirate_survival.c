const int tile_width = 8;

Gfx_Font *font;

bool almost_equals(float a, float b, float epsilon)
{
	return fabs(a - b) <= epsilon;
}

bool animate_f32_to_target(float *value, float target, float delat_t, float rate)
{
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delat_t));
	if (almost_equals(*value, target, 0.001f))
	{
		*value = target;
		return true;
	}
	return false;
}

void animate_v2_to_target(Vector2 *value, Vector2 target, float delat_t, float rate)
{
	animate_f32_to_target(&(value->x), target.x, delat_t, rate);
	animate_f32_to_target(&(value->y), target.y, delat_t, rate);
}

float32 world_pos_to_tile_pos(float world_pos)
{
	return world_pos / (float)tile_width;
}

float32 dist_lenght(Vector2 a, Vector2 b)
{
	return v2_length(v2_sub(a, b));
}

typedef struct Sprite
{
	Gfx_Image *image;
	Vector2 size;
} Sprite;
typedef enum SpriteID
{
	SPRITE_nil,
	SPRITE_player,
	SPRITE_tree0,
	// SPRITE_tree1,
	SPRITE_rock0,
	SPRITE_rock1,
	SPRITE_MAX,
} SpriteID;
// randy: maybe we make this an X macro?? https://chatgpt.com/share/260222eb-2738-4d1e-8b1d-4973a097814d
Sprite sprites[SPRITE_MAX];
Sprite *get_sprite(SpriteID id)
{
	if (id >= 0 && id < SPRITE_MAX)
	{
		return &sprites[id];
	}
	return &sprites[0];
}

typedef enum EntityArchetype
{
	arch_nil = 0,
	arch_rock = 1,
	arch_tree = 2,
	arch_player = 3,
} EntityArchetype;
typedef struct Entity
{
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;

	bool render_sprite;
	SpriteID sprite_id;
} Entity;
#define MAX_ENTITY_COUNT 1024

typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];
} World;
World *world = 0;

Entity *entity_create()
{
	Entity *entity_found = 0;
	for (int i = 0; i < MAX_ENTITY_COUNT; i++)
	{
		Entity *existing_entity = &world->entities[i];
		if (!existing_entity->is_valid)
		{
			entity_found = existing_entity;
			break;
		}
	}
	entity_found->is_valid = true;
	assert(entity_found, "No more free entities");
	return entity_found;
}

void entity_destroy(Entity *entity)
{
	memset(entity, 0, sizeof(Entity));
}

void setup_player(Entity *en)
{
	en->arch = arch_player;
	en->sprite_id = SPRITE_player;
}

void setup_tree(Entity *en)
{
	en->arch = arch_tree;
	en->sprite_id = SPRITE_tree0;
	// en->sprite_id = SPRITE_tree1;
}

void setup_rock(Entity *en)
{
	en->arch = arch_rock;
	en->sprite_id = SPRITE_rock0;
	// en->sprite_id = SPRITE_tree1;
}

void setup_rock1(Entity *en)
{
	en->arch = arch_rock;
	en->sprite_id = SPRITE_rock1;
	// en->sprite_id = SPRITE_tree1;
}

Vector2 screen_to_world(Vector2 screen)
{
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.view;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (screen.x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (screen.y / (window_h * 0.5f)) - 1.0f;

	// Transform to world coordinates
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);

	return world_pos.xy;
}

Vector2 get_mouse_world_pos()
{
	return screen_to_world(v2(input_frame.mouse_x, input_frame.mouse_y));
}

int entry(int argc, char **argv)
{
	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_temporary_allocator());
	assert(font, "Failed loading arial.ttf, %d", GetLastError());

	window.title = STR("Minimal Game Example OF UMOM");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720;
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0x181818ff);

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(STR("assets/player.png"), get_heap_allocator()), .size = v2(8.0, 10.0)};
	sprites[SPRITE_tree0] = (Sprite){.image = load_image_from_disk(STR("assets/tree0.png"), get_heap_allocator()), .size = v2(8.0, 12.0)};
	sprites[SPRITE_rock0] = (Sprite){.image = load_image_from_disk(STR("assets/rock0.png"), get_heap_allocator()), .size = v2(3.0, 6.0)};
	sprites[SPRITE_rock1] = (Sprite){.image = load_image_from_disk(STR("assets/rock1.png"), get_heap_allocator()), .size = v2(3.0, 6.0)};

	// Player entity
	Entity *player_en = entity_create();
	setup_player(player_en);

	// rock 0 entities
	for (int i = 1; i < 30; i++)
	{
		Entity *en = entity_create();
		setup_rock(en);
		en->pos = v2(get_random_float32_in_range(-50, 50), get_random_float32_in_range(-50, 50));
	}

	// rock 1 entities
	for (int i = 1; i < 30; i++)
	{
		Entity *en = entity_create();
		setup_rock1(en);
		en->pos = v2(get_random_float32_in_range(-50, 50), get_random_float32_in_range(-50, 50));
	}

	// Tree entities
	for (int i = 1; i < 100; i++)
	{
		Entity *en = entity_create();
		setup_tree(en);
		en->pos = v2(get_random_float32_in_range(-50, 50), get_random_float32_in_range(-50, 50));
	}

	// vars to debug frames
	float64 seconds_counter = 0.0;
	s32 frame_count = 0;

	float zoom = 15.3;
	Vector2 camera_pos = v2(0, 0);

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close)
	{
		reset_temporary_storage();

		draw_frame.projection = m4_make_orthographic_projection(window.pixel_width * -0.5, window.pixel_width * 0.5, window.pixel_height * -0.5, window.pixel_height * 0.5, -1, 10);
		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

		// :camera
		{
			Vector2 target_pos = player_en->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 5.0f);

			draw_frame.view = m4_make_scale(v3(1.0, 1.0, 1.0));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 1.0)));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0)));
		}

		os_update();
		Vector2 mouse_pos_world = get_mouse_world_pos();
		float32 mouse_tile_x = mouse_pos_world.x;
		float32 mouse_tile_y = mouse_pos_world.y;

		// render
		for (int i = 0; i < MAX_ENTITY_COUNT; i++)
		{
			Entity *en = &world->entities[i];
			if (en->is_valid)
			{
				Sprite *sprite = get_sprite(en->sprite_id);
				Vector4 color = COLOR_BLACK;
				{
					Vector2 en_player_pos = player_en->pos;
					Vector2 en_obj_pos = en->pos;
					f32 lenght_dist_pos_obj_and_player = dist_lenght(en_player_pos, en_obj_pos);

					if (lenght_dist_pos_obj_and_player < 20)
					{
						color = COLOR_WHITE;
						// {
						// 	string text = STR("x=%f, y=%f\n");
						// 	text = tprint(text, mouse_tile_x, mouse_tile_y);
						// 	Vector2 size = v2(2, 2);
						// 	Vector2 newpos = v2_sub(v2(mouse_tile_x, mouse_tile_y), v2_divf(size, 2.0));
						// 	draw_circle(newpos, size, COLOR_RED);
						// }
						{

							Vector2 size = v2(2, 2);
							f32 x1 = en->pos.x;
							f32 x2 = en->pos.x + sprite->size.x;
							f32 y1 = en->pos.y;
							f32 y2 = en->pos.y + sprite->size.y;
							f32 xCenter = (x1 + x2) * 0.5;
							f32 yCenter = (y1 + y2) * 0.5;

							Vector2 newpos = v2(xCenter - 1.25, yCenter - (sprite->size.y / 4.4));
							draw_circle(newpos, size, COLOR_WHITE);

							Vector2 mousenewpos = v2_sub(v2(mouse_tile_x, mouse_tile_y), v2_divf(size, 2.0));

							if (dist_lenght(mousenewpos, newpos) < 1)
							{
								color = COLOR_RED;
							}
						}
					}
				}
				switch (en->arch)
				{
				default:
				{

					Matrix4 xform = m4_scalar(2.0);
					// xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
					// xform = m4_translate(xform, v3(sprite->size.x * 0.2, sprite->size.y * 1, 0));
					draw_image(sprite->image, en->pos, sprite->size, color);
					break;
				}
				}
			}
		}

		if (is_key_just_pressed(KEY_ESCAPE))
		{
			window.should_close = true;
		}

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A'))
		{
			input_axis.x -= 1.0;
		}
		if (is_key_down('D'))
		{
			input_axis.x += 1.0;
		}
		if (is_key_down('S'))
		{
			input_axis.y -= 1.0;
		}
		if (is_key_down('W'))
		{
			input_axis.y += 1.0;
		}
		input_axis = v2_normalize(input_axis);

		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, 20.0 * delta_t));

		gfx_update();

		// Show frame count
		seconds_counter += delta_t;
		frame_count += 1;
		if (seconds_counter > 1.0)
		{
			log("fps: %i", frame_count);
			seconds_counter = 0.0;
			frame_count = 0;
		}
	}

	return 0;
}