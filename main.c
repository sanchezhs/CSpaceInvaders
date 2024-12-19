#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_CAPACITY 10
#define da_append(array, item)                                                 \
  do {                                                                         \
    if ((array)->count >= (array)->capacity) {                                 \
      (array)->capacity =                                                      \
          (array)->capacity == 0 ? INITIAL_CAPACITY : (array)->capacity * 2;   \
      (array)->items = realloc((array)->items,                                 \
                               (array)->capacity * sizeof(*(array)->items));   \
      assert((array)->items != NULL && "Buy more RAM lol");                    \
    }                                                                          \
    (array)->items[(array)->count++] = (item);                                 \
  } while (0)

// Window paremeters
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

typedef struct {
  Vector2 position;
  Vector2 velocity;
  Color color;
} Bullet;

typedef struct {
  Bullet *items;
  int capacity;
  int count;
} Bullets;

typedef struct {
  Vector2 position;
  Vector2 velocity;
  Bullets bullets;
  float last_shot_time;
  bool is_alive;
  Texture2D texture;
  float radius;
  Sound shot_sound;
  bool moves_vertically;
} Invader;

typedef struct {
  Invader *items;
  int capacity;
  int count;
  Color color;
  int width;
  int height;
  Texture2D shared_texture;
  float radius;
  Sound explosion_sound;
  Sound shot_sound;
} Invaders;

typedef struct {
  Vector2 position;
  Bullets bullets;
  int width;
  int height;
  Color color;
  Texture2D texture;
  Sound shot_sound;
} Player;

typedef struct {
  Player player;
  Invaders invaders;
  float player_shoot_coldown;
  float invaders_cooldown;
  float invaders_shoot_cooldown;
  int buller_width;
  int buller_height;
  float score;
  bool paused;
  int wave;
  float time_since_start;
  float next_wave_time;
  float next_wave_score;
} GameState;

float get_random_float(float min, float max) {
  return (float)rand() / (float)(RAND_MAX / (max - min)) + min;
}

void init_gamestate(GameState *gs) {
  // Player
  gs->player.width = 15;
  gs->player.height = 15;
  gs->player.position.x = (float)gs->player.width / 2 + gs->player.width * 2;
  gs->player.position.y =
      ((float)WINDOW_HEIGHT / 2) - (float)gs->player.height / 2;
  gs->player.color = BLACK;
  gs->player.texture = LoadTexture("./resources/PNG/playerShip1_blue.png");
  gs->player.shot_sound = LoadSound("./resources/Bonus/sfx_laser1.ogg");

  gs->player_shoot_coldown = 0.25f;
  gs->invaders_cooldown = 3.5f;
  gs->invaders_shoot_cooldown = 2.0f;

  // Bullets
  gs->buller_width = gs->player.width / 2;
  gs->buller_height = gs->player.height / 2;

  gs->paused = false;
  gs->wave = 0;

  // Invaders
  int invader_width = gs->player.width * 5.5;
  int invader_height = gs->player.height * 5.5;
  gs->invaders.width = invader_width;
  gs->invaders.height = invader_height;
  gs->invaders.color = RED;
  Image invaderImage = LoadImage("./resources/PNG/ufoRed.png");
  ImageResize(&invaderImage, invader_width, invader_height);
  gs->invaders.shared_texture = LoadTextureFromImage(invaderImage);
  UnloadImage(invaderImage);
  gs->invaders.radius = (invader_width * 0.9f) / 2.0f;
  gs->invaders.shot_sound = LoadSound("./resources/Bonus/sfx_laser2.ogg");
  gs->invaders.explosion_sound = LoadSound("./resources/8-bit-explosion.mp3");
}

void player_draw(Player *p) {
  DrawTextureEx(p->texture, (Vector2){p->position.x, p->position.y}, 90.0f,
                0.5f, WHITE);
}

void player_move(Player *p, Vector2 direction) {

  if (p->position.x <= 0 && direction.x < 0) {
    return;
  }

  if (p->position.x >= WINDOW_WIDTH - p->width && direction.x > 0) {
    return;
  }

  if (p->position.y <= 0 && direction.y < 0) {
    return;
  }

  if (p->position.y >= WINDOW_HEIGHT - p->height && direction.y > 0) {
    return;
  }

  p->position.x += direction.x * 4;
  p->position.y += direction.y * 4;
}

void player_shoot(GameState *gs, float *time_since_last_shoot) {
  if (*time_since_last_shoot < gs->player_shoot_coldown) {
    return;
  }
  float x = gs->player.position.x + gs->buller_width;
  float y = gs->player.position.y + (float)gs->player.height / 2 + 10;
  Vector2 bullet_pos = (Vector2){x, y};
  Vector2 bullet_vel = (Vector2){5, 0};
  Bullet bullet = (Bullet){bullet_pos, bullet_vel, WHITE};

  PlaySound(gs->player.shot_sound);
  da_append(&gs->player.bullets, bullet);

  *time_since_last_shoot = 0.0f;
}

void bullets_draw(GameState *gs) {
  // Player
  for (int i = 0; i < gs->player.bullets.count; i++) {
    Bullet b = gs->player.bullets.items[i];
    DrawRectangle(b.position.x, b.position.y, 5, 5, b.color);
  }

  // Invaders
  for (int i = 0; i < gs->invaders.count; i++) {
    Invader inv = gs->invaders.items[i];
    for (int j = 0; j < inv.bullets.count; j++) {
      Bullet b = inv.bullets.items[j];
      DrawRectangle(b.position.x, b.position.y, 5, 5, b.color);
    }
  }
}

void bullets_move(GameState *gs) {
  // Player
  for (int i = 0; i < gs->player.bullets.count; i++) {
    Bullet *b = &gs->player.bullets.items[i];
    b->position.x += b->velocity.x;
    b->position.y += b->velocity.y;
  }

  // Invaders
  for (int i = 0; i < gs->invaders.count; i++) {
    Invader *inv = &gs->invaders.items[i];
    for (int j = 0; j < inv->bullets.count; j++) {
      Bullet *b = &inv->bullets.items[j];
      b->position.x += b->velocity.x;
      b->position.y += b->velocity.y;
    }
  }
}

void bullets_clean(Bullets *bs) {
  int j = 0;
  for (int i = 0; i < bs->count; i++) {
    Bullet *b = &bs->items[i];
    if (b->position.x < WINDOW_WIDTH) {
      bs->items[j++] = *b;
    }
  }
  bs->count = j;
}

void invaders_draw(GameState *gs) {
  for (int i = 0; i < gs->invaders.count; i++) {
    Invader inv = gs->invaders.items[i];
    inv.radius = gs->invaders.radius;
    if (!inv.is_alive) {
      continue;
    }
    DrawTextureEx(gs->invaders.shared_texture, inv.position, 0.0f, 0.9, WHITE);
  }
}

void invaders_spawn(GameState *gs, float *time_since_last_invader) {
  if (*time_since_last_invader < gs->invaders_cooldown)
    return;

  int base_invaders = 5;
  int additional_invaders = gs->wave;
  int num_invaders =
      GetRandomValue(base_invaders, base_invaders + additional_invaders);
  for (int i = 0; i < num_invaders; i++) {
    int x, y;
    Vector2 inv_vel;

    int spawn_offset =
        GetRandomValue(gs->invaders.width, gs->invaders.width * 2);
    x = WINDOW_WIDTH + spawn_offset;
    y = GetRandomValue(0, WINDOW_HEIGHT - gs->invaders.height);
    inv_vel = (Vector2){get_random_float(-5.0f, -2.0f), GetRandomValue(-2, 2)};
    Vector2 inv_pos = (Vector2){x, y};
    Invader inv = (Invader){inv_pos,
                            inv_vel,
                            {0},
                            get_random_float(0.0f, 1.0f),
                            true,
                            gs->invaders.shared_texture,
                            gs->invaders.radius,
                            gs->invaders.shot_sound,
                            get_random_float(0, 1) <= 0.1};
    da_append(&gs->invaders, inv);
  }

  *time_since_last_invader = 0.0f;
}

void invaders_move(Invaders *invs) {
  for (int i = 0; i < invs->count; i++) {
    Invader *inv = &invs->items[i];
    inv->position.x += inv->velocity.x;
    if (inv->moves_vertically) {
      int amp = GetRandomValue(5, 10);
      inv->position.y += sinf(inv->position.x * 0.03f) * amp;
    }
  }
}

void invaders_clean(Invaders *invs) {
  int j = 0;
  for (int i = 0; i < invs->count; i++) {
    Invader *inv = &invs->items[i];
    if (inv->position.x > 0) {
      invs->items[j++] = *inv;
    }
  }
  invs->count = j;
}

void invaders_shoot(GameState *gs) {
  for (int i = 0; i < gs->invaders.count; i++) {
    float x = get_random_float(0.0, 1.0);
    Invader *inv = &gs->invaders.items[i];
    inv->last_shot_time += GetFrameTime();

    if (inv->last_shot_time < gs->invaders_shoot_cooldown || !inv->is_alive) {
      continue;
    }

    if (x >= 0.7) {
      float x = inv->position.x - gs->buller_width;
      float y = inv->position.y + (float)gs->invaders.height / 2;
      Vector2 bullet_pos = (Vector2){x, y};
      Vector2 bullet_vel = (Vector2){inv->velocity.x * 2.5, 0};
      Bullet bullet = (Bullet){bullet_pos, bullet_vel, RED};
      PlaySound(inv->shot_sound);
      da_append(&gs->invaders.items[i].bullets, bullet);
      inv->last_shot_time = 0.0f;
      if (gs->wave >= 10) {
        float z = get_random_float(0, 1);
        if (z < 0.05) {
          Vector2 up_bullet_vel = (Vector2){inv->velocity.x * 2.5, 5};
          Bullet up_bullet = (Bullet){bullet_pos, up_bullet_vel, RED};
          Vector2 down_bullet_vel = (Vector2){inv->velocity.x * 2.5, -5};
          Bullet down_bullet = (Bullet){bullet_pos, down_bullet_vel, RED};
          da_append(&gs->invaders.items[i].bullets, up_bullet);
          da_append(&gs->invaders.items[i].bullets, down_bullet);
        }
      }
    }
  }
}

bool check_collision(Player *p, Invaders *invs, float *score) {

  // Player vs Invaders
  for (int i = 0; i < invs->count; i++) {
    Invader inv = invs->items[i];
    if (!inv.is_alive) {
      continue;
    }
    if (CheckCollisionRecs(
            (Rectangle){p->position.x, p->position.y, p->width, p->height},
            (Rectangle){inv.position.x, inv.position.y, invs->width,
                        invs->height})) {
      return true;
    }
  }

  // Player bullets vs Invaders
  for (int i = 0; i < p->bullets.count; i++) {
    Bullet b = p->bullets.items[i];
    for (int j = 0; j < invs->count; j++) {
      Invader *inv = &invs->items[j];
      if (!inv->is_alive) {
        continue;
      }
      if (CheckCollisionCircleRec(
              (Vector2){inv->position.x + inv->radius,
                        inv->position.y + inv->radius},
              inv->radius, (Rectangle){b.position.x, b.position.y, 5, 5})) {
        inv->is_alive = false;
        PlaySound(invs->explosion_sound);
        p->bullets.items[i] = p->bullets.items[p->bullets.count - 1];
        p->bullets.count--;
        i--;
        *score += 10;
        break;
      }
    }
  }

  // Invaders bullets vs Player
  for (int i = 0; i < invs->count; i++) {
    Invader inv = invs->items[i];
    for (int j = 0; j < inv.bullets.count; j++) {
      Bullet b = inv.bullets.items[j];
      int w = p->texture.width;
      int h = p->texture.height;
      Vector2 pos =
          (Vector2){p->position.x - p->texture.width / 2, p->position.y};
      Rectangle rec =
          (Rectangle){pos.x, pos.y, w - 2 * p->width, h - p->height};

      if (CheckCollisionRecs((Rectangle){b.position.x, b.position.y, 5, 5},
                             rec)) {
        return true;
      }
    }
  }

  return false;
}

void unload_textures(GameState *gs) {
  UnloadTexture(gs->player.texture);
  for (int i = 0; i < gs->invaders.count; i++) {
    UnloadTexture(gs->invaders.items[i].texture);
  }
}

void unload_sounds(GameState *gs) {
  UnloadSound(gs->player.shot_sound);
  UnloadSound(gs->invaders.shot_sound);
  CloseAudioDevice();
}

void reset_game(GameState *gs, bool *game_over) {
  gs->player.bullets.count = 0;
  gs->invaders.count = 0;
  *game_over = false;
  init_gamestate(gs);
}

void adjust_difficulty(GameState *gs) {
  if (gs->score >= gs->next_wave_score * 10) {
    gs->wave += 1;

    gs->invaders_cooldown -= 0.05f;
    gs->invaders_cooldown = fmaxf(1.5f, gs->invaders_cooldown);

    gs->invaders_shoot_cooldown -= 0.05f;
    gs->invaders_shoot_cooldown = fmaxf(1.0f, gs->invaders_shoot_cooldown);

    gs->next_wave_score += 15.0f;
  }

  if (gs->time_since_start >= gs->next_wave_time) {
    gs->wave += 1;

    gs->invaders_cooldown -= 0.05f;
    gs->invaders_cooldown = fmaxf(1.5f, gs->invaders_cooldown);

    gs->invaders_shoot_cooldown -= 0.05f;
    gs->invaders_shoot_cooldown = fmaxf(1.0f, gs->invaders_shoot_cooldown);

    gs->next_wave_time += 60.0f;
  }

  if (gs->wave > 20) {
    gs->wave = 20;
  }
}

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Space Invaders");
  InitAudioDevice();
  SetTargetFPS(60);

  GameState gs = {0};
  init_gamestate(&gs);

  Texture2D background = LoadTexture("./resources/Backgrounds/black.png");

  Vector2 direction = {.x = 0, .y = 0};
  float new_width = WINDOW_WIDTH;
  float new_height = WINDOW_HEIGHT;
  static float time_since_last_shoot = 0.0f;
  static float time_since_last_invader = 0.0f;

  bool show_menu = true;
  bool paused = false;
  bool game_over = false;

  gs.wave = 1;
  gs.time_since_start = 0.0f;
  gs.next_wave_time = 60.0f;
  gs.next_wave_score = 10.0f;

  while (!WindowShouldClose()) {
    float scale_x = (float)new_width / background.width;
    float scale_y = (float)new_height / background.height;
    float final_scale = scale_x < scale_y ? scale_x : scale_y;

    ClearBackground(RAYWHITE);
    BeginDrawing();
    DrawTextureEx(background, (Vector2){0, 0}, 0, final_scale * 2, WHITE);

    if (show_menu) {
      const char *title = "SPACE INVADERS";
      const char *instructions1 = "Controls:";
      const char *instructions2 = "W/A/S/D: Move player";
      const char *instructions3 = "SPACE: Shoot";
      const char *instructions4 = "P: Pause/Unpause game";
      const char *start = "Press Enter to start";

      int titleWidth = MeasureText(title, 40);
      DrawText(title, WINDOW_WIDTH / 2 - titleWidth / 2, WINDOW_HEIGHT / 4, 40,
               WHITE);

      int instWidth = MeasureText(instructions1, 20);
      DrawText(instructions1, WINDOW_WIDTH / 2 - instWidth / 2,
               WINDOW_HEIGHT / 2 - 40, 20, DARKGRAY);

      instWidth = MeasureText(instructions2, 20);
      DrawText(instructions2, WINDOW_WIDTH / 2 - instWidth / 2,
               WINDOW_HEIGHT / 2, 20, DARKGRAY);

      instWidth = MeasureText(instructions3, 20);
      DrawText(instructions3, WINDOW_WIDTH / 2 - instWidth / 2,
               WINDOW_HEIGHT / 2 + 40, 20, DARKGRAY);

      instWidth = MeasureText(instructions4, 20);
      DrawText(instructions4, WINDOW_WIDTH / 2 - instWidth / 2,
               WINDOW_HEIGHT / 2 + 80, 20, DARKGRAY);

      int startWidth = MeasureText(start, 20);
      DrawText(start, WINDOW_WIDTH / 2 - startWidth / 2, WINDOW_HEIGHT - 100,
               20, RED);

      if (IsKeyPressed(KEY_ENTER)) {
        show_menu = false;
      }

    } else if (game_over) {
      game_over = true;
      const char *game_over_txt = "Game Over";
      const char *reset_txt = "Press R to reset";
      int game_over_w = MeasureText(game_over_txt, 40);
      int reset_w = MeasureText(reset_txt, 40);
      DrawText(game_over_txt, WINDOW_WIDTH / 2 - game_over_w / 2,
               WINDOW_HEIGHT / 4, 40, RED);
      DrawText(reset_txt, WINDOW_WIDTH / 2 - reset_w / 2, WINDOW_HEIGHT / 3, 40,
               RED);
      if (IsKeyPressed(KEY_R)) {
        reset_game(&gs, &game_over);
      }
    } else {
      if (!paused && !game_over) {
        gs.time_since_start += GetFrameTime();
        time_since_last_shoot += GetFrameTime();
        time_since_last_invader += GetFrameTime();
        adjust_difficulty(&gs);
        // Movement
        direction.x = 0;
        direction.y = 0;
        if (IsKeyDown(KEY_D) && IsKeyDown(KEY_S)) {
          direction.x = 1;
          direction.y = 1;
          player_move(&gs.player, direction);
        } else if (IsKeyDown(KEY_A) && IsKeyDown(KEY_S)) {
          direction.x = -1;
          direction.y = 1;
          player_move(&gs.player, direction);
        } else if (IsKeyDown(KEY_A) && IsKeyDown(KEY_W)) {
          direction.x = -1;
          direction.y = -1;
          player_move(&gs.player, direction);
        } else if (IsKeyDown(KEY_D) && IsKeyDown(KEY_W)) {
          direction.x = 1;
          direction.y = -1;
          player_move(&gs.player, direction);
        } else if (IsKeyDown(KEY_D)) {
          direction.x = 1;
          player_move(&gs.player, direction);
        } else if (IsKeyDown(KEY_A)) {
          direction.x = -1;
          player_move(&gs.player, direction);
        } else if (IsKeyDown(KEY_W)) {
          direction.y = -1;
          player_move(&gs.player, direction);
        } else if (IsKeyDown(KEY_S)) {
          direction.y = 1;
          player_move(&gs.player, direction);
        } else if (IsKeyDown(KEY_P)) {
          gs.paused = !gs.paused;
        }

        player_draw(&gs.player);

        invaders_spawn(&gs, &time_since_last_invader);
        invaders_draw(&gs);
        invaders_move(&gs.invaders);
        invaders_shoot(&gs);

        bullets_draw(&gs);
        bullets_move(&gs);

        game_over = check_collision(&gs.player, &gs.invaders, &gs.score);

        bullets_clean(&gs.player.bullets);
        invaders_clean(&gs.invaders);
      }
      if (IsKeyDown(KEY_SPACE)) {
        player_shoot(&gs, &time_since_last_shoot);
      }
      DrawText(TextFormat("Score: %.0f", gs.score), 10, 10, 20, WHITE);
      DrawText(TextFormat("Round: %d", gs.wave), WINDOW_WIDTH - 150, 10, 20,
               WHITE);

      if (IsKeyPressed(KEY_P)) {
        paused = !paused;
      }
      if (paused) {
        DrawText("Game Paused",
                 WINDOW_WIDTH / 2 - MeasureText("Game Paused", 20) / 2,
                 WINDOW_HEIGHT / 2, 20, RED);
      }
    }
    EndDrawing();
  }
  UnloadTexture(background);
  unload_sounds(&gs);
  unload_textures(&gs);
  CloseWindow();

  return 0;
}
