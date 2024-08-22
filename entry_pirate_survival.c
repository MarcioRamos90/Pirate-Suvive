#define COLOR_BLACK_TRANSPARENT ((Vector4){0.5, 0.5, 0.5, 1})

Gfx_Font *font;

float sin_breathe(float time, float rate)
{
	return (sin(time * rate) + 1.0) / 2.0;
}

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

float32 v2_dist(Vector2 a, Vector2 b)
{
	return v2_length(v2_sub(a, b));
}

const int rock_health = 3;
const int tree_health = 3;

const int tile_width = 8;
const float entity_selection_radius = 100.0f;
const float player_pickup_radius = 20.0f;

int world_pos_to_tile_pos(float world_pos)
{
	return roundf(world_pos / (float)tile_width);
}

float tile_pos_to_world_pos(int tile_pos)
{
	return ((float)tile_pos * (float)tile_width);
}

Vector2 round_v2_to_tile(Vector2 world_pos)
{
	world_pos.x = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.x));
	world_pos.y = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.y));
	return world_pos;
}

typedef struct Sprite
{
	Gfx_Image *image;
} Sprite;
typedef enum SpriteID
{
	SPRITE_nil,
	SPRITE_player,
	SPRITE_tree0,
	SPRITE_rock0,
	SPRITE_rock1,
	SPRITE_item_pine_wood,
	SPRITE_item_rock0,
	SPRITE_item_rock1,
	SPRITE_MAX,
} SpriteID;
Sprite sprites[SPRITE_MAX];
Sprite *get_sprite(SpriteID id)
{
	if (id >= 0 && id < SPRITE_MAX)
	{
		return &sprites[id];
	}
	return &sprites[0];
}

Vector2 get_sprite_size(Sprite *sprite)
{
	return (Vector2){sprite->image->width, sprite->image->height};
}

typedef enum EntityArchetype
{
	arch_nil = 0,
	arch_rock0 = 1,
	arch_rock1 = 2,
	arch_tree = 3,
	arch_player = 4,
	arch_item_rock0 = 5,
	arch_item_rock1 = 6,
	arch_item_pine_wood = 7,
	ARCH_MAX,
} EntityArchetype;

SpriteID get_sprite_id_from_archetype(EntityArchetype arch)
{
	switch (arch)
	{
	case arch_item_pine_wood:
		return SPRITE_item_pine_wood;
		break;
	case arch_item_rock0:
		return SPRITE_item_rock0;
		break;
	case arch_item_rock1:
		return SPRITE_item_rock1;
		break;
	default:
		return 0;
	}
}

typedef struct Entity
{
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;
	Vector4 color;
	bool render_sprite;
	SpriteID sprite_id;
	int health;
	bool destroyable_world_item;
	bool is_item;

} Entity;
// :entity
#define MAX_ENTITY_COUNT 1024

typedef struct ItemData
{
	int amount;
} ItemData;

// :world
typedef struct World
{
	Entity entities[MAX_ENTITY_COUNT];
	ItemData inventory_items[ARCH_MAX];
} World;
World *world = 0;
typedef struct WorldFrame
{
	Entity *selected_entity;
} WorldFrame;
WorldFrame world_frame;

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

	en->health = tree_health;
	en->destroyable_world_item = true;
}

void setup_item_tree(Entity *en)
{
	en->arch = arch_item_pine_wood;
	en->sprite_id = SPRITE_item_pine_wood;
	en->is_item = true;
}

void setup_rock0(Entity *en)
{
	en->arch = arch_rock0;
	en->sprite_id = SPRITE_rock0;

	en->health = rock_health;
	en->destroyable_world_item = true;
}

void setup_rock1(Entity *en)
{
	en->arch = arch_rock1;
	en->sprite_id = SPRITE_rock1;

	en->health = rock_health;
	en->destroyable_world_item = true;
}

void setup_item_rock0(Entity *en)
{
	en->arch = arch_item_rock0;
	en->sprite_id = SPRITE_item_rock0;
	en->is_item = true;
}

void setup_item_rock1(Entity *en)
{
	en->arch = arch_item_rock1;
	en->sprite_id = SPRITE_item_rock1;
	en->is_item = true;
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

	window.title = STR("Pirate Game");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720;
	window.x = 200;
	window.y = 200;
	window.clear_color = hex_to_rgba(0x181818ff);

	world = alloc(get_heap_allocator(), sizeof(World));
	memset(world, 0, sizeof(World));

	sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(STR("assets/aseprite-simplified/player.png"), get_heap_allocator())};
	sprites[SPRITE_tree0] = (Sprite){.image = load_image_from_disk(STR("assets/aseprite-simplified/tree1.png"), get_heap_allocator())};
	sprites[SPRITE_rock0] = (Sprite){.image = load_image_from_disk(STR("assets/aseprite-simplified/rock0.png"), get_heap_allocator())};
	sprites[SPRITE_rock1] = (Sprite){.image = load_image_from_disk(STR("assets/aseprite-simplified/rock1.png"), get_heap_allocator())};
	sprites[SPRITE_item_pine_wood] = (Sprite){.image = load_image_from_disk(STR("assets/aseprite-simplified/item_tree0.png"), get_heap_allocator())};
	sprites[SPRITE_item_rock0] = (Sprite){.image = load_image_from_disk(STR("assets/aseprite-simplified/item_rock0.png"), get_heap_allocator())};
	sprites[SPRITE_item_rock1] = (Sprite){.image = load_image_from_disk(STR("assets/aseprite-simplified/item_rock1.png"), get_heap_allocator())};

	// Player entity
	Entity *player_en = entity_create();
	setup_player(player_en);

	int the_world_size = 500;

	// rock 0 entities
	for (int i = 1; i < 30; i++)
	{
		Entity *en = entity_create();
		setup_rock0(en);
		en->pos = v2(get_random_float32_in_range(-the_world_size, the_world_size), get_random_float32_in_range(-the_world_size, the_world_size));
	}

	// rock 1 entities
	for (int i = 1; i < 30; i++)
	{
		Entity *en = entity_create();
		setup_rock1(en);
		en->pos = v2(get_random_float32_in_range(-the_world_size, the_world_size), get_random_float32_in_range(-the_world_size, the_world_size));
	}

	// Tree entities
	for (int i = 1; i < 100; i++)
	{
		Entity *en = entity_create();
		setup_tree(en);
		en->pos = v2(get_random_float32_in_range(-the_world_size, the_world_size), get_random_float32_in_range(-the_world_size, the_world_size));
	}

	// vars to debug frames
	float64 seconds_counter = 0.0;
	s32 frame_count = 0;

	float zoom = 5.3;
	Vector2 camera_pos = v2(0, 0);

	float64 last_time = os_get_current_time_in_seconds();
	while (!window.should_close)
	{
		reset_temporary_storage();
		world_frame = (WorldFrame){0};

		draw_frame.projection = m4_make_orthographic_projection(window.pixel_width * -0.5, window.pixel_width * 0.5, window.pixel_height * -0.5, window.pixel_height * 0.5, -1, 10);
		float64 now = os_get_current_time_in_seconds();
		float64 delta_t = now - last_time;
		last_time = now;

		// :camera
		{
			Vector2 target_pos = player_en->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 30.0f);
			draw_frame.view = m4_make_scale(v3(1.0, 1.0, 1.0));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1.0)));
		}

		os_update();
		Vector2 mouse_pos_world = get_mouse_world_pos();
		float32 mouse_tile_x = mouse_pos_world.x;
		float32 mouse_tile_y = mouse_pos_world.y;

		// calculating enable entities
		for (int i = 0; i < MAX_ENTITY_COUNT; i++)
		{
			Entity *en = &world->entities[i];
			if (en->is_valid && en->destroyable_world_item)
			{
				Sprite *sprite = get_sprite(en->sprite_id);
				en->color = COLOR_BLACK_TRANSPARENT;
				// calculating enable entities
				{
					Vector2 en_player_pos = player_en->pos;
					Vector2 en_obj_pos = en->pos;
					f32 lenght_dist_pos_obj_and_player = v2_dist(en_player_pos, en_obj_pos);

					if (lenght_dist_pos_obj_and_player < entity_selection_radius)
					{
						en->color = COLOR_WHITE;

						// calculating cliackable entity
						{

							Vector2 size = v2(15, 15);
							f32 x1 = en->pos.x;
							f32 x2 = en->pos.x + sprite->image->width;
							f32 y1 = en->pos.y;
							f32 y2 = en->pos.y + sprite->image->height;
							f32 xCenter = (x1 + x2) * 0.5;
							f32 yCenter = (y1 + y2) * 0.5;

							Vector2 newpos = v2(xCenter - (sprite->image->width * 0.3), yCenter - (sprite->image->height * 0.47));
							Vector2 mousenewpos = v2_sub(v2(mouse_tile_x, mouse_tile_y), v2_divf(size, 2.0));

							// draw_circle(newpos, size, COLOR_WHITE);
							if (v2_dist(mousenewpos, newpos) < 7)
							{
								en->color = COLOR_RED;
								world_frame.selected_entity = en;
							}
						}
					}
				}
			}
			else if (en->is_valid && !en->destroyable_world_item)
			{
				en->color = COLOR_WHITE;
			}
		}

		// clicable thing
		{
			Entity *selected_en = world_frame.selected_entity;

			if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
			{
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);

				if (selected_en)
				{

					printf("selected %i\n", selected_en->health);
					selected_en->health -= 1;
					if (selected_en->health <= 0)
					{
						switch (selected_en->arch)
						{
						case arch_tree:
						{
							Entity *en = entity_create();
							setup_item_tree(en);
							en->pos = selected_en->pos;
						}
						break;
						case arch_rock0:
						{
							Entity *en = entity_create();
							setup_item_rock0(en);
							en->pos = selected_en->pos;
						}
						break;
						case arch_rock1:
						{
							Entity *en = entity_create();
							setup_item_rock1(en);
							en->pos = selected_en->pos;
						}
						break;
						default:
							break;
						}
						entity_destroy(selected_en);
					}
				}
			}
		}

		// :render
		for (int i = 0; i < MAX_ENTITY_COUNT; i++)
		{
			Entity *en = &world->entities[i];
			if (en->is_valid)
			{
				Sprite *sprite = get_sprite(en->sprite_id);
				switch (en->arch)
				{
				default:
				{
					Matrix4 xform = m4_scalar(1.0);
					if (en->is_item)
					{
						xform = m4_translate(xform, v3(0, 2.0 * sin_breathe(os_get_current_time_in_seconds(), 5.0), 0));
					}
					xform = m4_translate(xform, v3(0, tile_width * -0.5, 0));
					xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
					xform = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, 0.0, 0));

					draw_image_xform(sprite->image, xform, get_sprite_size(sprite), en->color);
					break;
				}
				}
			}
		}

		// pick item
		{
			for (int i = 0; i < MAX_ENTITY_COUNT; i++)
			{
				Entity *en = &world->entities[i];
				if (en->is_valid)
				{
					// pick item
					if (en->is_item)
					{
						if (fabsf(v2_dist(en->pos, player_en->pos)) < player_pickup_radius)
						{
							world->inventory_items[en->arch].amount += 1;
							entity_destroy(en);
						}
					}
				}
			}
		}

		// : UI rendering
		{
			float width = 240.0;
			float height = 135.0;
			draw_frame.view = m4_scalar(1.0);
			draw_frame.projection = m4_make_orthographic_projection(0.0, width, 0.0, height, -1, 10);

			float y_pos = 70.0;

			int item_count = 0;
			for (int i = 0; i < ARCH_MAX; i++)
			{
				ItemData *item = &world->inventory_items[i];
				if (item->amount > 0)
				{
					item_count += 1;
				}
			}

			const float icon_thing = 8.0;
			const float padding = 2.0;
			float icon_width = icon_thing + padding;

			float entire_thing_width_idk = item_count * icon_width;
			float x_start_pos = (width / 2.0) - (entire_thing_width_idk / 2.0) + (icon_width * 0.5);

			int slot_index = 0;
			for (int i = 0; i < ARCH_MAX; i++)
			{
				ItemData *item = &world->inventory_items[i];
				if (item->amount > 0)
				{

					float slot_index_offset = slot_index * icon_width;

					Matrix4 xform = m4_scalar(1.0);
					xform = m4_translate(xform, v3(x_start_pos + slot_index_offset, y_pos, 0.0));
					xform = m4_translate(xform, v3(-4, -4, 0.0));
					draw_rect_xform(xform, v2(8, 8), COLOR_BLACK);

					Sprite *sprite = get_sprite(get_sprite_id_from_archetype(i));

					draw_image_xform(sprite->image, xform, get_sprite_size(sprite), COLOR_WHITE);

					slot_index += 1;
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

		f32 velocity = 100.0 * delta_t;
		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, velocity));

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