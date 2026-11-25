/******************************************************************************
 * Null Relay - Atmospheric pixel-art survival horror
 * Pure C + Raylib
 * Compiles to a single standalone Windows .exe
 *****************************************************************************/
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#define NATIVE_W 320
#define NATIVE_H 180
#define SCALE    4
#define TILE     16
#define MAP_W    40
#define MAP_H    20

// ---------- Colors ----------
#define COL_BLACK   (Color){8,6,12,255}
#define COL_DARK    (Color){18,14,22,255}
#define COL_MID     (Color){40,32,48,255}
#define COL_LIGHT   (Color){90,78,110,255}
#define COL_WHITE   (Color){220,210,230,255}
#define COL_BLOOD   (Color){140,20,30,255}
#define COL_GREEN   (Color){40,160,90,255}
#define COL_CYAN    (Color){60,180,200,255}
#define COL_PURPLE  (Color){120,60,160,255}
#define COL_FLICKER (Color){255,240,180,255}

// ---------- Map (1 = wall, 0 = floor, 4 = console) ----------
static const char *mapData[MAP_H] = {
    "1111111111111111111111111111111111111111",
    "1000000000000000000000000000000000000001",
    "1000000000000000000000000000000000000001",
    "1000000040000000000000000000000000000001",
    "1000000000000000000000000000000000000001",
    "1000000000000000000000000000000000000001",
    "1000000000000000000000000000000000000001",
    "1111111100000000000000000000011111111111",
    "1000000100000000000000000000010000000001",
    "1000000100000000000000000000010000000001",
    "1000000100000000000000000000010000000001",
    "1000000100000000000000000000010000000001",
    "1000000100000000000000000000010000000001",
    "1000000111111111111111111111110000000001",
    "1000000000000000000000000000000000000001",
    "1000000000000000000000000000000000000001",
    "1000000000000000000000000000000000000001",
    "1000000000000000000000000000000000000001",
    "1000000000000000000000000000000000000001",
    "1111111111111111111111111111111111111111",
};

// Notes
typedef struct { int tx, ty; const char *text; bool collected; } Note;
static Note notes[] = {
    {8, 3,  "LOG 01 - Dr. Voss: Amplification test scheduled. Voices in the static. Ignore.", false},
    {25,9,  "LOG 04 - Tech R.: Power fluctuations in Containment. Something is drawing current.", false},
    {12,15, "PERSONAL - I hear it breathing when the lights flicker. It knows the flashlight.", false},
    {30,5,  "FINAL - We opened a door that should stay closed. The signal is not ours.", false},
};
#define NOTE_COUNT (sizeof(notes)/sizeof(notes[0]))

// ---------- Player ----------
typedef struct {
    float x, y;
    float stamina, battery, sanity;
    bool lightOn;
    int facing;          // 1 or -1
    float animTimer;
    int animFrame;
    const char *notesRead[16];
    int noteCount;
    float interactCD;
} Player;

// ---------- Entity ----------
typedef enum { PATROL, STALK, CHASE, SEARCH, COOLDOWN } EntityState;

typedef struct {
    float x, y;
    float targetX, targetY;
    EntityState state;
    float awareness;
    float cooldown;
    float attackTimer;
    float animTimer;
    int animFrame;
    Vector2 patrol[4];
    int patrolIdx;
} Entity;

// ---------- Game ----------
typedef struct {
    Player player;
    Entity entity;
    int generatorsFixed;
    bool exitOpen;
    float camX, camY;
    float shake;
    float timeAlive;
    const char *message;
    float messageTimer;
    bool journalOpen;
    bool paused;
    const char *ending;      // NULL, "escape", "truth", "trapped"
    float endingTimer;
    RenderTexture2D target;  // for pixel scaling
} Game;

static Game g;

// ---------- Helpers ----------
static bool collides(float x, float y, float w, float h) {
    float pts[5][2] = {
        {x, y}, {x+w, y}, {x, y+h}, {x+w, y+h}, {x+w/2, y+h/2}
    };
    for (int i = 0; i < 5; i++) {
        int tx = (int)(pts[i][0] / TILE);
        int ty = (int)(pts[i][1] / TILE);
        if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) return true;
        if (mapData[ty][tx] == '1') return true;
    }
    return false;
}

static void showMessage(const char *msg, float duration) {
    g.message = msg;
    g.messageTimer = duration;
}

// ---------- Init ----------
static void InitGame(void) {
    memset(&g, 0, sizeof(g));

    g.player.x = 48;
    g.player.y = 48;
    g.player.stamina = 100;
    g.player.battery = 100;
    g.player.sanity = 100;
    g.player.lightOn = true;
    g.player.facing = 1;

    g.entity.x = 280;
    g.entity.y = 140;
    g.entity.state = PATROL;
    g.entity.patrol[0] = (Vector2){200, 80};
    g.entity.patrol[1] = (Vector2){300, 100};
    g.entity.patrol[2] = (Vector2){250, 160};
    g.entity.patrol[3] = (Vector2){180, 120};
    g.entity.targetX = g.entity.patrol[0].x;
    g.entity.targetY = g.entity.patrol[0].y;

    g.target = LoadRenderTexture(NATIVE_W, NATIVE_H);
    SetTextureFilter(g.target.texture, TEXTURE_FILTER_POINT); // crisp pixels
}

// ---------- Update ----------
static void UpdatePlayer(float dt) {
    Player *p = &g.player;
    p->interactCD -= dt;
    if (p->interactCD < 0) p->interactCD = 0;

    float dx = 0, dy = 0;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  { dx -= 1; p->facing = -1; }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) { dx += 1; p->facing = 1; }
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dy -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  dy += 1;

    bool sprint = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && p->stamina > 5;
    float speed = 55.0f * (sprint ? 1.7f : 1.0f);

    if (dx || dy) {
        float len = sqrtf(dx*dx + dy*dy);
        dx /= len; dy /= len;
        float nx = p->x + dx * speed * dt;
        float ny = p->y + dy * speed * dt;
        if (!collides(nx, p->y, 10, 14)) p->x = nx;
        if (!collides(p->x, ny, 10, 14)) p->y = ny;

        if (sprint) p->stamina -= 28 * dt;
        else        p->stamina += 7 * dt;
    } else {
        p->stamina += 18 * dt;
    }
    if (p->stamina > 100) p->stamina = 100;
    if (p->stamina < 0)   p->stamina = 0;

    // Light
    if (p->lightOn && p->battery > 0) {
        p->battery -= 4.5f * dt;
        if (p->battery <= 0) { p->battery = 0; p->lightOn = false; }
    } else if (!p->lightOn) {
        p->battery += 1.2f * dt;
        if (p->battery > 100) p->battery = 100;
    }

    // Sanity
    if (!p->lightOn || p->battery < 15)
        p->sanity -= 6.0f * dt;
    else
        p->sanity += 3.5f * dt;
    if (p->sanity > 100) p->sanity = 100;
    if (p->sanity < 0)   p->sanity = 0;

    p->animTimer += dt;
    if (p->animTimer > 0.12f) {
        p->animTimer = 0;
        p->animFrame = (p->animFrame + 1) % 4;
    }

    // Interact
    if ((IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE)) && p->interactCD <= 0) {
        int tx = (int)((p->x + 5) / TILE);
        int ty = (int)((p->y + 7) / TILE);

        // Notes
        for (int i = 0; i < NOTE_COUNT; i++) {
            if (!notes[i].collected && notes[i].tx == tx && notes[i].ty == ty) {
                notes[i].collected = true;
                p->notesRead[p->noteCount++] = notes[i].text;
                showMessage(notes[i].text, 5.0f);
                p->interactCD = 0.6f;
                return;
            }
        }

        // Generator
        if (mapData[ty][tx] == '4') {
            g.generatorsFixed++;
            if (g.generatorsFixed >= 2) g.exitOpen = true;
            showMessage("GENERATOR NODE STABILIZED. Power routed to surface lock.", 4.0f);
            p->interactCD = 0.6f;

            if (g.exitOpen) {
                g.ending = "escape";
                g.endingTimer = 5.0f;
            }
        }
    }
}

static void UpdateEntity(float dt) {
    Entity *e = &g.entity;
    Player *p = &g.player;

    e->cooldown -= dt;
    e->attackTimer -= dt;
    if (e->cooldown < 0) e->cooldown = 0;
    if (e->attackTimer < 0) e->attackTimer = 0;

    float px = p->x + 5, py = p->y + 7;
    float dx = px - (e->x + 8);
    float dy = py - (e->y + 12);
    float dist = sqrtf(dx*dx + dy*dy);

    // Very simple vision
    bool sees = dist < (p->lightOn ? 110 : 70);

    if (e->state == COOLDOWN) {
        if (e->cooldown <= 0) e->state = PATROL;
        return;
    }

    if (sees) {
        e->awareness += 55 * dt;
        if (e->awareness > 100) e->awareness = 100;
        if (e->awareness > 65 || dist < 40) e->state = CHASE;
        else e->state = STALK;
    } else {
        e->awareness -= 12 * dt;
        if (e->awareness < 0) e->awareness = 0;
        if ((e->state == CHASE || e->state == STALK) && e->awareness < 15)
            e->state = SEARCH;
    }

    float speed = 38.0f;
    if (e->state == CHASE) {
        speed = 78.0f;
        e->targetX = px;
        e->targetY = py;
    } else if (e->state == STALK) {
        float ang = atan2f(dy, dx) + 0.5f;
        e->targetX = px - cosf(ang) * 55;
        e->targetY = py - sinf(ang) * 55;
    } else if (e->state == PATROL) {
        Vector2 t = e->patrol[e->patrolIdx];
        if (fabsf(e->x - t.x) < 10 && fabsf(e->y - t.y) < 10)
            e->patrolIdx = (e->patrolIdx + 1) % 4;
        e->targetX = e->patrol[e->patrolIdx].x;
        e->targetY = e->patrol[e->patrolIdx].y;
    }

    // Move
    float tdx = e->targetX - e->x;
    float tdy = e->targetY - e->y;
    float tdist = sqrtf(tdx*tdx + tdy*tdy);
    if (tdist > 2) {
        tdx /= tdist; tdy /= tdist;
        float nx = e->x + tdx * speed * dt;
        float ny = e->y + tdy * speed * dt;
        if (!collides(nx, e->y, 16, 24)) e->x = nx;
        if (!collides(e->x, ny, 16, 24)) e->y = ny;
    }

    // Attack
    if (dist < 22 && e->state == CHASE && e->attackTimer <= 0) {
        p->sanity -= 28;
        p->stamina -= 20;
        e->attackTimer = 1.4f;
        e->state = COOLDOWN;
        e->cooldown = 2.8f;
    }

    e->animTimer += dt;
    if (e->animTimer > 0.1f) {
        e->animTimer = 0;
        e->animFrame = (e->animFrame + 1) % 6;
    }
}

static void UpdateGame(float dt) {
    if (g.ending) {
        g.endingTimer -= dt;
        if (g.endingTimer <= 0) {
            // stay on ending screen until Esc
        }
        return;
    }
    if (g.paused || g.journalOpen) return;

    g.timeAlive += dt;
    UpdatePlayer(dt);
    UpdateEntity(dt);

    // Camera
    g.camX = g.player.x - NATIVE_W/2 + 5;
    g.camY = g.player.y - NATIVE_H/2 + 7;
    if (g.camX < 0) g.camX = 0;
    if (g.camY < 0) g.camY = 0;
    if (g.camX > MAP_W*TILE - NATIVE_W) g.camX = MAP_W*TILE - NATIVE_W;
    if (g.camY > MAP_H*TILE - NATIVE_H) g.camY = MAP_H*TILE - NATIVE_H;

    // Tension
    if (g.player.sanity < 40)
        g.shake = (40 - g.player.sanity) * 0.08f;
    else
        g.shake *= 0.9f;

    if (g.player.sanity <= 0) {
        g.ending = "trapped";
        g.endingTimer = 4.0f;
    }

    // Truth ending
    if (g.player.x > 520 && g.player.noteCount >= 3) {
        g.ending = "truth";
        g.endingTimer = 5.0f;
    }

    if (g.messageTimer > 0) g.messageTimer -= dt;
}

// ---------- Draw ----------
static void DrawMap(void) {
    int startTX = (int)(g.camX / TILE) - 1;
    int endTX   = (int)((g.camX + NATIVE_W) / TILE) + 2;
    int startTY = (int)(g.camY / TILE) - 1;
    int endTY   = (int)((g.camY + NATIVE_H) / TILE) + 2;

    if (startTX < 0) startTX = 0;
    if (startTY < 0) startTY = 0;
    if (endTX > MAP_W) endTX = MAP_W;
    if (endTY > MAP_H) endTY = MAP_H;

    for (int ty = startTY; ty < endTY; ty++) {
        for (int tx = startTX; tx < endTX; tx++) {
            float sx = tx * TILE - g.camX;
            float sy = ty * TILE - g.camY;
            char t = mapData[ty][tx];

            if (t == '1') {
                DrawRectangle(sx, sy, TILE, TILE, COL_BLACK);
                DrawRectangle(sx, sy, TILE, 3, COL_MID);
                DrawRectangle(sx, sy+13, TILE, 3, COL_MID);
            } else {
                DrawRectangle(sx, sy, TILE, TILE, COL_DARK);
                // subtle floor pattern
                if ((tx + ty) % 2 == 0)
                    DrawRectangle(sx+2, sy+2, 2, 2, (Color){30,24,36,80});
            }

            if (t == '4') {
                DrawRectangle(sx+2, sy+4, 12, 10, COL_MID);
                DrawRectangle(sx+4, sy+6, 3, 2, COL_GREEN);
                DrawRectangle(sx+9, sy+6, 3, 2, (Color){200,80,40,255});
            }
        }
    }

    // Notes
    for (int i = 0; i < NOTE_COUNT; i++) {
        if (!notes[i].collected) {
            float sx = notes[i].tx * TILE - g.camX;
            float sy = notes[i].ty * TILE - g.camY;
            DrawRectangle(sx+3, sy+3, 10, 12, COL_WHITE);
            DrawRectangle(sx+4, sy+5, 8, 1, COL_DARK);
            DrawRectangle(sx+4, sy+8, 6, 1, COL_DARK);
        }
    }

    // Blood trails
    int blood[][2] = {{15,8},{18,9},{22,10}};
    for (int i = 0; i < 3; i++) {
        float sx = blood[i][0] * TILE - g.camX + 4;
        float sy = blood[i][1] * TILE - g.camY + 6;
        DrawRectangle(sx, sy, 3, 2, COL_BLOOD);
        DrawRectangle(sx+2, sy+2, 2, 1, COL_BLOOD);
    }
}

static void DrawPlayer(void) {
    float sx = g.player.x - g.camX;
    float sy = g.player.y - g.camY;

    // Simple pixel character
    DrawRectangle(sx+3, sy+1, 6, 4, COL_WHITE);          // head
    DrawRectangle(sx+4, sy+4, 4, 6, COL_LIGHT);           // body
    int legOff = (g.player.animFrame % 2);
    DrawRectangle(sx+3 + legOff, sy+10, 2, 5, COL_MID);   // legs
    DrawRectangle(sx+7 - legOff, sy+10, 2, 5, COL_MID);

    // Flashlight
    if (g.player.lightOn)
        DrawRectangle(sx + (g.player.facing > 0 ? 8 : -1), sy+6, 3, 2, COL_FLICKER);
}

static void DrawEntity(void) {
    float sx = g.entity.x - g.camX;
    float sy = g.entity.y - g.camY;

    Color body = (g.entity.state == COOLDOWN) ? COL_BLOOD : COL_PURPLE;
    DrawRectangle(sx+6, sy+4, 8, 16, body);
    DrawRectangle(sx+5, sy, 10, 6, COL_BLACK);
    DrawRectangle(sx+7, sy+1, 6, 3, COL_CYAN);

    // Tendrils
    for (int t = 0; t < 3; t++) {
        int ox = (g.entity.animFrame + t) % 3;
        DrawRectangle(sx+4 + t*4 + ox, sy+18, 2, 5 + (g.entity.animFrame%3), body);
    }
}

static void DrawHUD(void) {
    // Stamina
    DrawRectangle(6, 6, 50, 5, COL_DARK);
    DrawRectangle(7, 7, (int)(48 * g.player.stamina / 100), 3, COL_GREEN);

    // Battery
    DrawRectangle(6, 13, 50, 5, COL_DARK);
    DrawRectangle(7, 14, (int)(48 * g.player.battery / 100), 3,
                  g.player.lightOn ? COL_CYAN : COL_MID);

    // Sanity
    DrawRectangle(6, 20, 50, 5, COL_DARK);
    Color sanCol = g.player.sanity > 40 ? COL_PURPLE : COL_BLOOD;
    DrawRectangle(7, 21, (int)(48 * g.player.sanity / 100), 3, sanCol);

    // Message
    if (g.messageTimer > 0 && g.message) {
        DrawText(g.message, 8, NATIVE_H - 20, 8, COL_WHITE);
    }
}

static void DrawEnding(void) {
    ClearBackground(COL_BLACK);
    const char *lines[5] = {0};
    if (strcmp(g.ending, "escape") == 0) {
        lines[0] = "You restored surface power.";
        lines[1] = "The lock cycles open.";
        lines[2] = "You climb into the cold night air.";
        lines[3] = "Behind you the station goes silent.";
        lines[4] = "ENDING: ESCAPE";
    } else if (strcmp(g.ending, "truth") == 0) {
        lines[0] = "You reach the core.";
        lines[1] = "The signal is not a message.";
        lines[2] = "It is a mouth.";
        lines[3] = "You understand why the crew never left.";
        lines[4] = "ENDING: TRUTH";
    } else {
        lines[0] = "The light dies.";
        lines[1] = "Something finds you in the dark.";
        lines[2] = "Your last thought is static.";
        lines[3] = "ENDING: TRAPPED";
    }

    int y = 50;
    for (int i = 0; i < 5 && lines[i]; i++) {
        int w = MeasureText(lines[i], 12);
        DrawText(lines[i], NATIVE_W/2 - w/2, y, 12, COL_WHITE);
        y += 16;
    }
    DrawText("[ESC] Quit", 10, NATIVE_H - 16, 8, COL_LIGHT);
}

static void DrawFrame(void) {
    BeginTextureMode(g.target);
        ClearBackground(COL_BLACK);

        if (g.ending) {
            DrawEnding();
        } else {
            DrawMap();
            DrawEntity();
            DrawPlayer();

            // Darkness overlay
            DrawRectangle(0, 0, NATIVE_W, NATIVE_H, (Color){0,0,0,180});

            // Flashlight circle (simple)
            if (g.player.lightOn && g.player.battery > 0) {
                float lx = g.player.x - g.camX + 5 + g.player.facing * 8;
                float ly = g.player.y - g.camY + 7;
                DrawCircleGradient(lx, ly, 70, (Color){255,240,200,90}, (Color){0,0,0,0});
            }

            // Vignette-ish
            DrawRectangle(0, 0, NATIVE_W, 20, (Color){0,0,0,100});
            DrawRectangle(0, NATIVE_H-20, NATIVE_W, 20, (Color){0,0,0,100});

            // Sanity pulse
            if (g.player.sanity < 50) {
                float pulse = fabsf(sinf(g.timeAlive * 3.0f)) * (50 - g.player.sanity) * 1.2f;
                DrawRectangle(0, 0, NATIVE_W, NATIVE_H, (Color){80,0,20, (unsigned char)pulse});
            }

            DrawHUD();

            if (g.journalOpen) {
                DrawRectangle(0, 0, NATIVE_W, NATIVE_H, (Color){0,0,0,200});
                DrawText("FIELD JOURNAL", 20, 12, 12, COL_CYAN);
                int y = 30;
                for (int i = 0; i < g.player.noteCount; i++) {
                    DrawText(g.player.notesRead[i], 18, y, 8, COL_WHITE);
                    y += 12;
                }
                DrawText("[TAB] close", 20, NATIVE_H - 14, 8, COL_LIGHT);
            }
        }
    EndTextureMode();

    // Scale to screen with nearest neighbor
    BeginDrawing();
        ClearBackground(BLACK);
        float ox = 0, oy = 0;
        if (g.shake > 0.5f) {
            ox = (float)GetRandomValue(- (int)g.shake, (int)g.shake);
            oy = (float)GetRandomValue(- (int)g.shake, (int)g.shake);
        }
        DrawTexturePro(
            g.target.texture,
            (Rectangle){0, 0, (float)NATIVE_W, -(float)NATIVE_H}, // flip Y
            (Rectangle){ox, oy, NATIVE_W*SCALE, NATIVE_H*SCALE},
            (Vector2){0,0}, 0, WHITE
        );
    EndDrawing();
}

// ---------- Main ----------
int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(NATIVE_W * SCALE, NATIVE_H * SCALE, "Null Relay");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // we handle Esc ourselves

    InitGame();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Input
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (g.ending) break;
            g.paused = !g.paused;
        }
        if (IsKeyPressed(KEY_TAB)) g.journalOpen = !g.journalOpen;
        if (IsKeyPressed(KEY_F) && g.player.battery > 2)
            g.player.lightOn = !g.player.lightOn;

        UpdateGame(dt);
        DrawFrame();
    }

    UnloadRenderTexture(g.target);
    CloseWindow();
    return 0;
}
