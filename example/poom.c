/* POOM-F: original float-only SIXEL FPS.

MIT License

Copyright (c) 2026 POOM-F contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#define _POSIX_C_SOURCE 200809L

/* ==================== mathf.h ==================== */
#ifndef POOM_MATHF_H
#define POOM_MATHF_H
#include <float.h>
#include <stdint.h>
_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24,
               "IEEE binary32 float required");
_Static_assert(FLT_EVAL_METHOD == 0, "Native float expression evaluation required");
#define PM_PI 3.14159265358979323846f
#define PM_TAU 6.28318530717958647692f
float pm_sinf(float x); /* finite |x| <= 4096; not a general libm replacement */
float pm_cosf(float x);
float pm_sqrtf(float x);
static inline float pm_absf(float x) { return x < 0.0f ? -x : x; }
static inline float pm_minf(float a, float b) { return a < b ? a : b; }
static inline float pm_maxf(float a, float b) { return a > b ? a : b; }
static inline float pm_clampf(float x, float lo, float hi) {
    return pm_minf(pm_maxf(x, lo), hi);
}
/* The renderer calls these only with bounded, finite coordinates. */
static inline int pm_floori(float x) { int i = (int)x; return i - (x < (float)i); }
static inline int pm_ceili(float x) { int i = (int)x; return i + (x > (float)i); }
#endif

/* ==================== poom.h ==================== */
#ifndef POOM_FLOAT_H
#define POOM_FLOAT_H
#include <stddef.h>
#include <stdint.h>
#define SCREEN_W 128
#define SCREEN_H 128
#define VIEW_H 110
#define TEX_SIZE 32
#define NUM_TEX 8
#define MAX_VERTS 16
#define MAX_SECTORS 12
#define MAX_ACTORS 32
#define SPR_W 24
#define SPR_H 32
#define NUM_SPRITES 7
#define TRANSPARENT 255
#define PLAYER_RADIUS 0.22f
#define PLAYER_HEIGHT 1.55f
#define EYE_HEIGHT 1.12f
#define NEAR_Z 0.07f

typedef struct { float x, y; } Vec2;
typedef struct {
    int count;
    Vec2 v[MAX_VERTS];
    int neighbor[MAX_VERTS], door[MAX_VERTS];
    float floor, ceiling, light;
    uint8_t wall_tex, floor_tex, ceiling_tex;
} Sector;
typedef struct { float open; int target; } Door;
enum { ENEMY, MEDKIT, AMMO, KEY, EXIT_SIGN };
typedef struct {
    float x, y, z, cooldown, hurt, age;
    int kind, hp, active, sector, variant;
} Actor;
typedef struct {
    float x, y, z, angle, shot, hurt, walk_phase;
    int sector, hp, ammo, key, kills, won;
} Player;
typedef struct {
    Sector sectors[MAX_SECTORS];
    int num_sectors, num_actors, enemy_count;
    Door doors[1];
    Actor actors[MAX_ACTORS];
    Player p;
    uint32_t now, hold[6];
    unsigned held;
    int show_map, paused, quit;
    float message_time;
    const char *message;
    uint8_t textures[NUM_TEX][TEX_SIZE * TEX_SIZE];
    uint8_t sprites[NUM_SPRITES][SPR_W * SPR_H];
    uint8_t shade[8][16];
    uint8_t *pixels;
    float *depth;
    unsigned triangles, fragments;
} Game;
extern const uint8_t poom_palette[16][3];
Game *game_create(void);
void game_destroy(Game *g);
void game_reset(Game *g);
void game_key(Game *g, int ch);
void game_step(Game *g, unsigned ms);
void game_fire(Game *g);
void game_use(Game *g);
int world_sector(const Game *g, float x, float y);
int world_can_stand(const Game *g, int old_sector, float x, float y, float radius);
int world_line_clear(const Game *g, float x0, float y0, float z0,
                     float x1, float y1, float z1);
void world_move(Game *g, float dx, float dy);
void assets_init(Game *g);
void render_frame(Game *g);
uint32_t frame_hash(const Game *g);
void draw_text(Game *g, int x, int y, const char *s, uint8_t color);
void draw_number(Game *g, int x, int y, unsigned n, uint8_t color);
void draw_rect(Game *g, int x0, int y0, int x1, int y1, uint8_t color);
#endif

/* ==================== sixel.h ==================== */
#ifndef POOM_SIXEL_H
#define POOM_SIXEL_H
#include <stddef.h>
#include <stdint.h>
/* Callback returns 0 on complete success, -1 on an output failure. */
typedef int (*SixelWrite)(void *context, const void *data, unsigned size);
int sixel_encode(const uint8_t *pixels, int width, int height, int scale,
                 SixelWrite write, void *context, size_t *bytes);
#endif

/* ==================== platform.h ==================== */
#ifndef POOM_PLATFORM_H
#define POOM_PLATFORM_H
#include <stdint.h>
extern volatile uint32_t ticks; /* one tick = one millisecond */
void systick(void);
int getchar_timeout(unsigned ms); /* byte 0..255, timeout/EOF/error: -1 */
int console_write(const void *data, unsigned size); /* 0 success, -1 failure */
int platform_init(int interactive);
void platform_shutdown(void);
uint32_t platform_millis(void);
int platform_stopped(void);
int platform_input_closed(void);
#endif

/* ==================== mathf.c ==================== */
#include <string.h>
static float from_bits(uint32_t u) { float x; memcpy(&x, &u, sizeof x); return x; }
float pm_sinf(float x) {
    if (!(x >= -4096.0f && x <= 4096.0f)) return from_bits(0x7fc00000u);
    while (x > PM_PI) x -= PM_TAU;
    while (x < -PM_PI) x += PM_TAU;
    if (x > PM_PI * 0.5f) x = PM_PI - x;
    if (x < -PM_PI * 0.5f) x = -PM_PI - x;
    float z = x * x;
    return x + x * z * (-1.6666667163e-1f + z * (8.3333337680e-3f +
           z * (-1.9841270114e-4f + z * (2.7557314297e-6f +
           z * (-2.5050759689e-8f + z * 1.5896910177e-10f)))));
}
float pm_cosf(float x) { return pm_sinf(x + PM_PI * 0.5f); }
float pm_sqrtf(float x) {
    uint32_t u; memcpy(&u, &x, sizeof u);
    if ((u & 0x7fffffffu) == 0) return x; /* includes negative zero */
    if (u & 0x80000000u) return from_bits(0x7fc00000u);
    if (u >= 0x7f800000u) return x; /* positive infinity / NaN */
    float scale = 1.0f;
    if (u < 0x00800000u) {
        x *= 0x1p24f; scale = 0x1p-12f;
        memcpy(&u, &x, sizeof u);
    }
    float y = from_bits((u >> 1) + 0x1fc00000u);
    for (int i = 0; i < 5; ++i) y = 0.5f * (y + x / y);
    return y * scale;
}

/* ==================== assets.c ==================== */
#include <stdlib.h>
#include <string.h>
const uint8_t poom_palette[16][3] = {
    {8, 9, 12}, {21, 27, 40}, {39, 44, 51}, {67, 73, 76},
    {106, 112, 112}, {167, 174, 162}, {237, 221, 177}, {252, 249, 217},
    {66, 31, 29}, {143, 40, 32}, {215, 89, 40}, {244, 184, 59},
    {30, 78, 52}, {92, 180, 65}, {48, 131, 194}, {133, 221, 230}
};
static void sp_rect(uint8_t *p, int x0, int y0, int x1, int y1, uint8_t c) {
    for (int y=y0;y<=y1;++y) for (int x=x0;x<=x1;++x)
        if (x>=0 && y>=0 && x<SPR_W && y<SPR_H) p[y*SPR_W+x]=c;
}
static void sp_ellipse(uint8_t *p, int cx, int cy, int rx, int ry, uint8_t c) {
    for (int y=-ry;y<=ry;++y) for (int x=-rx;x<=rx;++x)
        if (x*x*ry*ry+y*y*rx*rx <= rx*rx*ry*ry)
            sp_rect(p, cx+x, cy+y, cx+x, cy+y, c);
}
void assets_init(Game *g) {
    srand(7331u);
    for (int y=0;y<TEX_SIZE;++y) for (int x=0;x<TEX_SIZE;++x) {
        int i=y*TEX_SIZE+x, noise=rand()%8;
        uint8_t steel=(uint8_t)(noise==0 ? 4 : 3);
        if (x%16==0 || y==0 || y==31) steel=1;
        if (x%16==1 || y==1) steel=4;
        if ((x%16==3 || x%16==13) && (y==3 || y==28)) steel=5;
        if (y>12 && y<20 && x>5 && x<11) steel=(uint8_t)(x==6 ? 4 : 2);
        g->textures[0][i]=steel;
        int bx=(x+((y/8)&1)*8)%16;
        g->textures[1][i]=(uint8_t)(y%8==0 || bx==0 ? 8 : noise==0 ? 10 : 9);
        if (y%8==1 && bx>0) g->textures[1][i]=4;
        uint8_t tech=(uint8_t)(x%16==0 || y%16==0 ? 1 : 2);
        if (x>3 && x<12 && y>4 && y<19) tech=3;
        if (x>5 && x<10 && y>6 && y<16) tech=(uint8_t)(y%3 ? 12 : 13);
        if (x>22 && x<26) tech=(uint8_t)(x==23 ? 5 : 4);
        if (y>26) tech=(uint8_t)(((x+y)/4)%2 ? 11 : 0);
        g->textures[2][i]=tech;
        uint8_t floor=(uint8_t)(noise==0 ? 4 : 3);
        if (x%16==0 || y%16==0) floor=1;
        if (x%16==1 || y%16==1) floor=4;
        g->textures[3][i]=floor;
        g->textures[4][i]=(uint8_t)(y%4==0 ? 1 : 2);
        if (x>=12 && x<20) g->textures[4][i]=(uint8_t)(x==12 || x==19 ? 4 : 6);
        uint8_t door=(uint8_t)(x%8==0 ? 1 : 3);
        if (x==1 || x==30) door=14;
        if (x>10 && x<21) door=(uint8_t)(y%8==0 ? 15 : 14);
        if (y<4 || y>27) door=(uint8_t)(((x+y)/4)%2 ? 11 : 0);
        g->textures[5][i]=door;
        g->textures[6][i]=(uint8_t)(x%4==0 || y%4==0 ? 2 : noise<2 ? 13 : 12);
        g->textures[7][i]=(uint8_t)(y<8 ? ((x+y)/4)%2 ? 11 : 1 : (x+y)%4==0 ? 4 : 3);
    }
    for (int level=0;level<8;++level) for (int c=0;c<16;++c) {
        int best=0, dist=0x7fffffff;
        for (int k=0;k<16;++k) {
            int d=0;
            for (int j=0;j<3;++j) {
                int v=(int)poom_palette[c][j]*(level+2)/9-(int)poom_palette[k][j];
                d+=v*v;
            }
            if (d<dist) { dist=d; best=k; }
        }
        g->shade[level][c]=(uint8_t)best;
    }
    memset(g->sprites, TRANSPARENT, sizeof g->sprites);
    for (int f=0;f<2;++f) {
        uint8_t *p=g->sprites[f];
        sp_ellipse(p, 12, 30, 9, 1, 1);
        sp_rect(p, 7, 20, 10, 28-f, 8); sp_rect(p, 14, 20, 17, 27+f, 8);
        sp_rect(p, 6, 28-f, 10, 29-f, 2); sp_rect(p, 14, 27+f, 18, 28+f, 2);
        sp_ellipse(p, 12, 15, 8, 8, 8); sp_ellipse(p, 12, 15, 6, 7, 9);
        sp_rect(p, 9, 12, 14, 14, 10); sp_rect(p, 10, 16, 14, 19, 8);
        sp_ellipse(p, 4, 15, 3, 5, 2); sp_ellipse(p, 19, 15, 3, 5, 2);
        sp_rect(p, 3, 14, 5, 18, 9); sp_rect(p, 18, 14, 20, 18, 9);
        sp_rect(p, 3, 19, 5, 22, 6); sp_rect(p, 18, 19, 20, 22, 6);
        sp_ellipse(p, 12, 7, 5, 6, 8); sp_ellipse(p, 12, 7, 4, 5, 6);
        sp_rect(p, 7, 2, 8, 5, 5); sp_rect(p, 16, 2, 17, 5, 5);
        sp_rect(p, 8, 6, 11, 7, 0); sp_rect(p, 13, 6, 16, 7, 0);
        sp_rect(p, 9, 6, 10, 6, 10); sp_rect(p, 14, 6, 15, 6, 10);
        sp_rect(p, 11, 9, 13, 10, 2); sp_rect(p, 10, 11, 14, 11, 5);
        sp_rect(p, 6, 12, 8, 14, 4); sp_rect(p, 16, 12, 18, 14, 4);
        sp_rect(p, 5, 20, 10, 22, 2); sp_rect(p, 8, 19, 10, 20, 4);
    }
    uint8_t *p=g->sprites[2];
    sp_ellipse(p, 12, 29, 11, 2, 8); sp_ellipse(p, 9, 28, 6, 2, 9);
    sp_ellipse(p, 17, 27, 3, 3, 6); sp_rect(p, 18, 27, 19, 27, 0);
    p=g->sprites[3];
    sp_rect(p, 3, 17, 20, 29, 1); sp_rect(p, 4, 17, 19, 27, 5);
    sp_rect(p, 5, 18, 18, 26, 6); sp_rect(p, 9, 19, 13, 25, 9); sp_rect(p, 7, 21, 15, 23, 9);
    sp_rect(p, 8, 14, 15, 16, 4); sp_rect(p, 10, 14, 13, 15, 1);
    p=g->sprites[4];
    sp_rect(p, 4, 19, 19, 29, 1); sp_rect(p, 5, 18, 18, 26, 8);
    for (int x=7;x<=16;x+=3) { sp_rect(p, x, 17, x+1, 24, 11); sp_rect(p, x, 17, x, 18, 6); }
    sp_rect(p, 5, 25, 18, 28, 4); sp_rect(p, 7, 26, 16, 27, 2);
    p=g->sprites[5];
    sp_ellipse(p, 9, 20, 6, 6, 14); sp_ellipse(p, 9, 20, 3, 3, TRANSPARENT);
    sp_rect(p, 13, 20, 21, 22, 14); sp_rect(p, 18, 22, 19, 25, 14);
    sp_rect(p, 21, 22, 22, 24, 14); sp_rect(p, 5, 16, 10, 16, 15);
    p=g->sprites[6];
    sp_rect(p, 1, 5, 22, 29, 1); sp_rect(p, 2, 6, 21, 28, 12);
    sp_rect(p, 4, 8, 19, 26, 13); sp_rect(p, 10, 10, 13, 24, 7);
    sp_rect(p, 7, 12, 16, 14, 7); sp_rect(p, 5, 14, 18, 16, 7);
    sp_rect(p, 4, 25, 19, 26, 12);
}

/* ==================== world.c ==================== */
#include <stdlib.h>
#include <string.h>
static const Sector level[] = {
    {.count=8, .v={{0, 0}, {6, 0}, {6, 1}, {6, 4}, {6, 5}, {0, 5}, {0, 4}, {0, 1}},
     .floor=0.0f, .ceiling=3.2f, .light=1.0f, .wall_tex=0, .floor_tex=3, .ceiling_tex=4},
    {.count=4, .v={{6, 1}, {10, 1}, {10, 4}, {6, 4}},
     .floor=0.0f, .ceiling=2.5f, .light=0.82f, .wall_tex=1, .floor_tex=3, .ceiling_tex=4},
    {.count=12, .v={{10, -1}, {16, -1}, {18, 1}, {18, 2}, {18, 6}, {18, 7}, {16, 9},
                   {12, 9}, {10, 9}, {10, 4}, {10, 1}, {10, 0}},
     .floor=0.0f, .ceiling=4.5f, .light=0.96f, .wall_tex=1, .floor_tex=3, .ceiling_tex=4},
    {.count=4, .v={{12, 9}, {16, 9}, {16, 12}, {12, 12}},
     .floor=0.30f, .ceiling=3.0f, .light=0.82f, .wall_tex=0, .floor_tex=7, .ceiling_tex=4},
    {.count=10, .v={{10, 12}, {12, 12}, {16, 12}, {18, 12}, {20, 14}, {20, 18},
                   {18, 20}, {10, 20}, {8, 18}, {8, 14}},
     .floor=0.60f, .ceiling=4.2f, .light=1.0f, .wall_tex=2, .floor_tex=3, .ceiling_tex=4},
    {.count=4, .v={{18, 2}, {22, 2}, {22, 6}, {18, 6}},
     .floor=0.0f, .ceiling=3.2f, .light=0.88f, .wall_tex=0, .floor_tex=7, .ceiling_tex=4},
    {.count=10, .v={{22, 0}, {30, 0}, {32, 2}, {32, 6}, {30, 8}, {28, 8},
                   {26, 8}, {22, 8}, {22, 6}, {22, 2}},
     .floor=0.0f, .ceiling=5.0f, .light=1.0f, .wall_tex=2, .floor_tex=6, .ceiling_tex=4},
    {.count=4, .v={{26, 8}, {28, 8}, {28, 11}, {26, 11}},
     .floor=0.30f, .ceiling=3.0f, .light=0.86f, .wall_tex=1, .floor_tex=7, .ceiling_tex=4},
    {.count=6, .v={{24, 11}, {26, 11}, {28, 11}, {30, 11}, {30, 15}, {24, 15}},
     .floor=0.60f, .ceiling=3.5f, .light=1.0f, .wall_tex=2, .floor_tex=3, .ceiling_tex=4}
};
static float cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
static Vec2 sub(Vec2 a, Vec2 b) { return (Vec2){a.x-b.x, a.y-b.y}; }
static int same(Vec2 a, Vec2 b) { return a.x == b.x && a.y == b.y; }
static float segment_dist2(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab = sub(b, a), ap = sub(p, a);
    float d = ab.x*ab.x + ab.y*ab.y;
    float t = d > 0.0f ? pm_clampf((ap.x*ab.x+ap.y*ab.y)/d, 0.0f, 1.0f) : 0.0f;
    float x = ap.x-t*ab.x, y = ap.y-t*ab.y;
    return x*x+y*y;
}
int world_sector(const Game *g, float x, float y) {
    Vec2 p = {x, y};
    for (int s = 0; s < g->num_sectors; ++s) {
        const Sector *sec = &g->sectors[s];
        int inside = 1;
        for (int i = 0; i < sec->count; ++i) {
            Vec2 a = sec->v[i], b = sec->v[(i+1)%sec->count];
            if (cross(sub(b, a), sub(p, a)) < -0.00001f) { inside = 0; break; }
        }
        if (inside) return s;
    }
    return -1;
}
static int walkable(const Game *g, int s, int e, float old_floor) {
    const Sector *a = &g->sectors[s];
    int n = a->neighbor[e];
    if (n < 0) return 0;
    const Sector *b = &g->sectors[n];
    float lo = pm_maxf(a->floor, b->floor), hi = pm_minf(a->ceiling, b->ceiling);
    if (lo > old_floor + 0.36f) return 0;
    if (a->door[e] >= 0) hi = lo + (hi-lo)*g->doors[a->door[e]].open;
    return hi-lo >= PLAYER_HEIGHT+0.03f;
}
int world_can_stand(const Game *g, int old_sector, float x, float y, float radius) {
    int s = world_sector(g, x, y);
    if (s < 0 || old_sector < 0 || old_sector >= g->num_sectors) return 0;
    float old_floor = g->sectors[old_sector].floor;
    if (g->sectors[s].floor > old_floor+0.36f) return 0;
    if (s != old_sector) {
        int adjacent = 0;
        const Sector *old = &g->sectors[old_sector];
        for (int e = 0; e < old->count; ++e)
            if (old->neighbor[e] == s && walkable(g, old_sector, e, old_floor)) adjacent = 1;
        if (!adjacent) return 0;
    }
    Vec2 p = {x, y};
    /* Test wall capsules globally. This also protects the ends of portals. */
    for (int t = 0; t < g->num_sectors; ++t) {
        const Sector *sec = &g->sectors[t];
        for (int e = 0; e < sec->count; ++e) {
            if (walkable(g, t, e, old_floor)) continue;
            if (segment_dist2(p, sec->v[e], sec->v[(e+1)%sec->count]) < radius*radius) return 0;
        }
    }
    return 1;
}
void world_move(Game *g, float dx, float dy) {
    Player *p = &g->p;
    float length = pm_sqrtf(dx*dx+dy*dy);
    int steps = 1 + (int)(length / 0.08f);
    dx /= (float)steps; dy /= (float)steps;
    for (int i = 0; i < steps; ++i) {
        if (world_can_stand(g, p->sector, p->x+dx, p->y, PLAYER_RADIUS)) {
            p->x += dx; p->sector = world_sector(g, p->x, p->y);
        }
        if (world_can_stand(g, p->sector, p->x, p->y+dy, PLAYER_RADIUS)) {
            p->y += dy; p->sector = world_sector(g, p->x, p->y);
        }
    }
}
int world_line_clear(const Game *g, float x0, float y0, float z0,
                     float x1, float y1, float z1) {
    Vec2 p={x0, y0}, d={x1-x0, y1-y0};
    for (int s = 0; s < g->num_sectors; ++s) {
        const Sector *a = &g->sectors[s];
        for (int e = 0; e < a->count; ++e) {
            Vec2 v=a->v[e], edge=sub(a->v[(e+1)%a->count], v);
            float den=cross(d, edge);
            if (pm_absf(den) < 0.000001f) continue;
            float t=cross(sub(v, p), edge)/den, u=cross(sub(v, p), d)/den;
            if (t <= 0.0001f || t >= 0.9999f || u < 0.0f || u > 1.0f) continue;
            int n=a->neighbor[e];
            if (n < 0) return 0;
            const Sector *b=&g->sectors[n];
            float lo=pm_maxf(a->floor, b->floor), hi=pm_minf(a->ceiling, b->ceiling);
            if (a->door[e] >= 0) hi=lo+(hi-lo)*g->doors[a->door[e]].open;
            float z=z0+(z1-z0)*t;
            if (z <= lo+0.001f || z >= hi-0.001f) return 0;
        }
    }
    return 1;
}
static void add_actor(Game *g, int kind, float x, float y, int variant) {
    Actor *a=&g->actors[g->num_actors++];
    *a=(Actor){.x=x, .y=y, .kind=kind, .hp=50, .active=1, .variant=variant};
    a->sector=world_sector(g, x, y); a->z=g->sectors[a->sector].floor;
    a->cooldown=0.8f+(float)(rand()%100)*0.01f;
    if (kind == ENEMY) ++g->enemy_count;
}
void game_reset(Game *g) {
    memset(g->sectors, 0, sizeof g->sectors);
    memcpy(g->sectors, level, sizeof level);
    g->num_sectors=(int)(sizeof level/sizeof level[0]);
    for (int s=0;s<g->num_sectors;++s) for (int e=0;e<g->sectors[s].count;++e) {
        g->sectors[s].neighbor[e]=-1; g->sectors[s].door[e]=-1;
    }
    for (int s=0;s<g->num_sectors;++s) for (int e=0;e<g->sectors[s].count;++e) {
        Sector *a=&g->sectors[s];
        for (int t=s+1;t<g->num_sectors;++t) for (int f=0;f<g->sectors[t].count;++f) {
            Sector *b=&g->sectors[t];
            if (same(a->v[e], b->v[(f+1)%b->count]) && same(a->v[(e+1)%a->count], b->v[f])) {
                a->neighbor[e]=t; b->neighbor[f]=s;
                if (s==2 && t==5) { a->door[e]=0; b->door[f]=0; }
            }
        }
    }
    memset(g->actors, 0, sizeof g->actors); memset(g->doors, 0, sizeof g->doors);
    memset(g->hold, 0, sizeof g->hold);
    g->held=0; g->num_actors=0; g->enemy_count=0; g->now=0;
    g->paused=0; g->quit=0; g->show_map=0;
    g->p=(Player){.x=2.2f, .y=2.5f, .z=EYE_HEIGHT, .angle=0.0f, .sector=0, .hp=100, .ammo=48};
    g->message="FIND THE BLUE KEY"; g->message_time=4.0f;
    srand(12345u);
    add_actor(g, ENEMY, 13.0f, 3.0f, 0);
    add_actor(g, ENEMY, 16.0f, 6.5f, 1);
    add_actor(g, ENEMY, 10.5f, 16.0f, 1);
    add_actor(g, ENEMY, 17.0f, 17.0f, 0);
    add_actor(g, ENEMY, 25.0f, 3.0f, 0);
    add_actor(g, ENEMY, 29.0f, 5.8f, 1);
    add_actor(g, ENEMY, 27.0f, 12.5f, 0);
    add_actor(g, MEDKIT, 3.0f, 4.0f, 0);
    add_actor(g, AMMO, 4.5f, 1.0f, 0);
    add_actor(g, AMMO, 11.0f, 8.0f, 0);
    add_actor(g, MEDKIT, 18.0f, 18.0f, 0);
    add_actor(g, KEY, 14.0f, 18.0f, 0);
    add_actor(g, AMMO, 23.0f, 6.5f, 0);
    add_actor(g, MEDKIT, 29.0f, 1.5f, 0);
    add_actor(g, EXIT_SIGN, 27.0f, 14.0f, 0);
}
Game *game_create(void) {
    Game *g=malloc(sizeof *g);
    if (!g) return NULL;
    memset(g, 0, sizeof *g);
    g->pixels=malloc(SCREEN_W*SCREEN_H);
    g->depth=malloc((size_t)SCREEN_W*VIEW_H*sizeof *g->depth);
    if (!g->pixels || !g->depth) { game_destroy(g); return NULL; }
    assets_init(g); game_reset(g); return g;
}
void game_destroy(Game *g) {
    if (!g) return;
    free(g->pixels); free(g->depth); free(g);
}

/* ==================== game.c ==================== */
#include <stdlib.h>
static float decay(float x, float dt) { return pm_maxf(0.0f, x-dt); }
static void message(Game *g, const char *s) { g->message=s; g->message_time=2.0f; }
void game_fire(Game *g) {
    Player *p=&g->p;
    if (p->hp<=0 || p->won || g->paused || p->shot>0.0f) return;
    if (p->ammo<=0) { message(g,"OUT OF AMMO"); return; }
    --p->ammo; p->shot=0.24f;
    float fx=pm_cosf(p->angle), fy=pm_sinf(p->angle), best=1000.0f;
    Actor *hit=NULL;
    for (int i=0;i<g->num_actors;++i) {
        Actor *a=&g->actors[i];
        if (!a->active || a->kind!=ENEMY || a->hp<=0) continue;
        float dx=a->x-p->x, dy=a->y-p->y;
        float along=dx*fx+dy*fy, side=pm_absf(dx*fy-dy*fx);
        /* Small classic-FPS autoaim cone, not a vertical mouse aim. */
        if (along<=0.0f || along>=best || side>0.32f+along*0.025f) continue;
        if (!world_line_clear(g, p->x, p->y, p->z, a->x, a->y, a->z+0.85f)) continue;
        best=along; hit=a;
    }
    if (hit) {
        hit->hp-=26+rand()%12; hit->hurt=0.18f;
        if (hit->hp<=0) { ++p->kills; message(g,"TARGET DOWN"); }
    }
}
void game_use(Game *g) {
    if (g->p.hp<=0 || g->p.won || g->paused) return;
    float dx=g->p.x-18.0f, dy=g->p.y-4.0f;
    if (dx*dx+dy*dy < 10.0f) {
        if (!g->p.key) message(g,"BLUE KEY REQUIRED");
        else { g->doors[0].target=1; message(g,"ACCESS GRANTED"); }
    } else message(g,"FIND KEY THEN EXIT");
}
void game_key(Game *g, int ch) {
    if (ch>='A' && ch<='Z') ch+=32;
    const char keys[]="wsadqe";
    for (unsigned i=0;i<6;++i) if (ch==keys[i]) {
        /* Legacy terminals have no key-up events: each repeat leases motion. */
        g->hold[i]=g->now; g->held|=1u<<i; return;
    }
    if (ch==' ' || ch=='j') game_fire(g);
    else if (ch=='f' || ch=='\r' || ch=='\n') game_use(g);
    else if (ch=='m' || ch=='\t') g->show_map=!g->show_map;
    else if (ch=='p') g->paused=!g->paused;
    else if (ch=='r') game_reset(g);
    else if (ch=='x' || ch==27 || ch==3) g->quit=1;
}
static void step_actor(Game *g, Actor *a, float dt) {
    if (!a->active) return;
    Player *p=&g->p;
    a->age+=dt;
    if (a->age>PM_TAU) a->age-=PM_TAU;
    a->hurt=decay(a->hurt, dt); a->cooldown=decay(a->cooldown, dt);
    float dx=p->x-a->x, dy=p->y-a->y, d2=dx*dx+dy*dy;
    if (a->kind!=ENEMY) {
        if (d2>0.62f*0.62f || pm_absf(g->sectors[p->sector].floor-a->z)>0.7f) return;
        if (!world_line_clear(g, p->x, p->y, p->z, a->x, a->y, a->z+0.4f)) return;
        if (a->kind==MEDKIT && p->hp<100) {
            p->hp+=35; if (p->hp>100) p->hp=100;
            a->active=0; message(g,"HEALTH +35");
        } else if (a->kind==AMMO && p->ammo<150) {
            p->ammo+=24; if (p->ammo>150) p->ammo=150;
            a->active=0; message(g,"AMMO +24");
        } else if (a->kind==KEY) {
            p->key=1; a->active=0; message(g,"BLUE KEY ACQUIRED");
        } else if (a->kind==EXIT_SIGN && p->key) {
            p->won=1; message(g,"SECTOR CLEAR");
        }
        return;
    }
    if (a->hp<=0 || d2>18.0f*18.0f) return;
    int visible=world_line_clear(g, a->x, a->y, a->z+0.9f, p->x, p->y, p->z);
    if (!visible) return;
    if (d2>0.9f*0.9f) {
        float speed=0.8f*dt/pm_sqrtf(d2);
        if (world_can_stand(g, a->sector, a->x+dx*speed, a->y, 0.20f)) a->x+=dx*speed;
        a->sector=world_sector(g, a->x, a->y);
        if (world_can_stand(g, a->sector, a->x, a->y+dy*speed, 0.20f)) a->y+=dy*speed;
        a->sector=world_sector(g, a->x, a->y); a->z=g->sectors[a->sector].floor;
    }
    if (d2<11.0f*11.0f && a->cooldown<=0.0f) {
        a->cooldown=1.35f+(float)(rand()%70)*0.01f;
        /* Hitscan enemy fire. The brief bright sprite is the firing tell. */
        a->hurt=0.10f;
        if (rand()%100<55 || d2<1.5f) {
            p->hp-=3+rand()%5; if (p->hp<0) p->hp=0;
            p->hurt=0.20f;
        }
    }
}
void game_step(Game *g, unsigned ms) {
    if (ms>100) ms=100;
    g->now+=ms;
    for (unsigned i=0;i<6;++i)
        if ((uint32_t)(g->now-g->hold[i])>=160u) g->held&=~(1u<<i);
    if (g->paused || g->p.won || g->p.hp<=0) return;
    float dt=(float)ms*0.001f;
    Player *p=&g->p;
    p->shot=decay(p->shot, dt); p->hurt=decay(p->hurt, dt);
    g->message_time=decay(g->message_time, dt);
    int fwd=((g->held&1u)!=0)-((g->held&2u)!=0);
    int turn=((g->held&8u)!=0)-((g->held&4u)!=0);
    int strafe=((g->held&32u)!=0)-((g->held&16u)!=0);
    p->angle+=(float)turn*2.1f*dt;
    if (p->angle<0.0f) p->angle+=PM_TAU;
    if (p->angle>=PM_TAU) p->angle-=PM_TAU;
    float c=pm_cosf(p->angle), s=pm_sinf(p->angle);
    float speed=3.0f*dt*(fwd && strafe ? 0.70710678f : 1.0f);
    world_move(g, ((float)fwd*c-(float)strafe*s)*speed,
                 ((float)fwd*s+(float)strafe*c)*speed);
    if (fwd || strafe) p->walk_phase+=dt*9.0f;
    if (p->walk_phase>PM_TAU) p->walk_phase-=PM_TAU;
    float target=g->sectors[p->sector].floor+EYE_HEIGHT;
    p->z+=(target-p->z)*pm_minf(1.0f, dt*12.0f);
    if (g->doors[0].target) g->doors[0].open=pm_minf(1.0f, g->doors[0].open+dt*0.85f);
    for (int i=0;i<g->num_actors && p->hp>0 && !p->won;++i) step_actor(g, &g->actors[i], dt);
}
uint32_t frame_hash(const Game *g) {
    uint32_t hash=2166136261u;
    for (int i=0;i<SCREEN_W*SCREEN_H;++i) hash=(hash^g->pixels[i])*16777619u;
    return hash;
}

/* ==================== ui.c ==================== */
/* Original compact 3x5 bitmap alphabet; row bits are packed top to bottom. */
#define G(a, b, c, d, e) ((a)<<12|(b)<<9|(c)<<6|(d)<<3|(e))
static const uint16_t font[36]={
    G(2, 5, 7, 5, 5), G(6, 5, 6, 5, 6), G(3, 4, 4, 4, 3), G(6, 5, 5, 5, 6),
    G(7, 4, 6, 4, 7), G(7, 4, 6, 4, 4), G(3, 4, 5, 5, 3), G(5, 5, 7, 5, 5),
    G(7, 2, 2, 2, 7), G(1, 1, 1, 5, 2), G(5, 5, 6, 5, 5), G(4, 4, 4, 4, 7),
    G(5, 7, 7, 5, 5), G(5, 7, 7, 7, 5), G(2, 5, 5, 5, 2), G(6, 5, 6, 4, 4),
    G(2, 5, 5, 7, 3), G(6, 5, 6, 5, 5), G(3, 4, 2, 1, 6), G(7, 2, 2, 2, 2),
    G(5, 5, 5, 5, 7), G(5, 5, 5, 5, 2), G(5, 5, 7, 7, 5), G(5, 5, 2, 5, 5),
    G(5, 5, 2, 2, 2), G(7, 1, 2, 4, 7),
    G(7, 5, 5, 5, 7), G(2, 6, 2, 2, 7), G(6, 1, 7, 4, 7), G(6, 1, 3, 1, 6),
    G(5, 5, 7, 1, 1), G(7, 4, 6, 1, 6), G(3, 4, 7, 5, 7), G(7, 1, 2, 2, 2),
    G(7, 5, 7, 5, 7), G(7, 5, 7, 1, 6)
};
void draw_rect(Game *g, int x0, int y0, int x1, int y1, uint8_t color) {
    if (x0<0) x0=0;
    if (y0<0) y0=0;
    if (x1>=SCREEN_W) x1=SCREEN_W-1;
    if (y1>=SCREEN_H) y1=SCREEN_H-1;
    for (int y=y0;y<=y1;++y) for (int x=x0;x<=x1;++x) g->pixels[y*SCREEN_W+x]=color;
}
void draw_text(Game *g, int x, int y, const char *s, uint8_t color) {
    for (;*s;++s, x+=4) {
        int c=*s;
        if (c>='a' && c<='z') c-=32;
        uint16_t bits=0;
        if (c>='A' && c<='Z') bits=font[c-'A'];
        else if (c>='0' && c<='9') bits=font[c-'0'+26];
        else if (c=='+') bits=G(0, 2, 7, 2, 0);
        else if (c=='-') bits=G(0, 0, 7, 0, 0);
        else if (c==':') bits=G(0, 2, 0, 2, 0);
        else if (c=='/') bits=G(1, 1, 2, 4, 4);
        for (int row=0;row<5;++row) for (int col=0;col<3;++col)
            if (bits&(1u<<(14-row*3-col))) draw_rect(g, x+col, y+row, x+col, y+row, color);
    }
}
void draw_number(Game *g, int x, int y, unsigned n, uint8_t color) {
    char s[11]; unsigned len=0;
    do { s[len++]=(char)('0'+n%10u); n/=10u; } while (n && len<10);
    for (unsigned i=0;i<len/2;++i) { char c=s[i]; s[i]=s[len-1-i]; s[len-1-i]=c; }
    s[len]=0; draw_text(g, x, y, s, color);
}

/* ==================== render.c ==================== */
#include <stdlib.h>
#include <string.h>
static int iabs(int x) { return x < 0 ? -x : x; }
#define FOCAL 78.0f
#define CX 64.0f
#define CY 53.0f
typedef struct { float x, y, z, u, v; } Vertex;
typedef struct { float x, y, iz, uz, vz; } Projected;
typedef struct { int index; float z; } SortItem;
typedef struct { Game *g; float c, s, eye; } Renderer;
static Vertex camera(const Renderer *r, float x, float y, float z, float u, float v) {
    float dx=x-r->g->p.x, dy=y-r->g->p.y;
    return (Vertex){dy*r->c-dx*r->s, z-r->eye, dx*r->c+dy*r->s, u, v};
}
static Vertex lerp(Vertex a, Vertex b, float t) {
    return (Vertex){a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t, a.z+(b.z-a.z)*t,
                    a.u+(b.u-a.u)*t, a.v+(b.v-a.v)*t};
}
static float edge(float ax, float ay, float bx, float by, float x, float y) {
    return (bx-ax)*(y-ay)-(by-ay)*(x-ax);
}
static void triangle(Game *g, Projected a, Projected b, Projected c, int tex, float light) {
    float area=edge(a.x, a.y, b.x, b.y, c.x, c.y);
    if (pm_absf(area)<0.0001f) return;
    if (area<0.0f) { Projected t=b;b=c;c=t;area=-area; }
    float minx=pm_minf(a.x, pm_minf(b.x, c.x)), maxx=pm_maxf(a.x, pm_maxf(b.x, c.x));
    float miny=pm_minf(a.y, pm_minf(b.y, c.y)), maxy=pm_maxf(a.y, pm_maxf(b.y, c.y));
    if (maxx<0.0f || maxy<0.0f || minx>SCREEN_W || miny>VIEW_H) return;
    int x0=pm_floori(pm_clampf(minx, 0.0f, (float)(SCREEN_W-1)));
    int x1=pm_ceili(pm_clampf(maxx, 0.0f, (float)(SCREEN_W-1)));
    int y0=pm_floori(pm_clampf(miny, 0.0f, (float)(VIEW_H-1)));
    int y1=pm_ceili(pm_clampf(maxy, 0.0f, (float)(VIEW_H-1)));
    float inv=1.0f/area;
    float da=-(c.y-b.y)*inv, db=-(a.y-c.y)*inv, dc=-(b.y-a.y)*inv;
    ++g->triangles;
    for (int y=y0;y<=y1;++y) {
        float px=(float)x0+0.5f, py=(float)y+0.5f;
        float wa=edge(b.x, b.y, c.x, c.y, px, py)*inv;
        float wb=edge(c.x, c.y, a.x, a.y, px, py)*inv;
        float wc=edge(a.x, a.y, b.x, b.y, px, py)*inv;
        for (int x=x0;x<=x1;++x, wa+=da, wb+=db, wc+=dc) {
            if (wa< -0.000001f || wb< -0.000001f || wc< -0.000001f) continue;
            int p=y*SCREEN_W+x;
            float iz=wa*a.iz+wb*b.iz+wc*c.iz;
            if (iz<=g->depth[p]) continue;
            float z=1.0f/iz;
            float u=(wa*a.uz+wb*b.uz+wc*c.uz)*z;
            float v=(wa*a.vz+wb*b.vz+wc*c.vz)*z;
            unsigned tx=(unsigned)pm_floori(u*(float)TEX_SIZE)&(TEX_SIZE-1u);
            unsigned ty=(unsigned)pm_floori(v*(float)TEX_SIZE)&(TEX_SIZE-1u);
            uint8_t color=g->textures[tex][ty*TEX_SIZE+tx];
            /* Ordered distance shading in the 16-color palette. */
            float shade=light*7.0f/(1.0f+z*0.025f)+(((x^y)&1) ? 0.35f : -0.10f);
            int level=(int)pm_clampf(shade, 0.0f, 7.0f);
            g->pixels[p]=g->shade[level][color];g->depth[p]=iz;++g->fragments;
        }
    }
}
static void polygon(Renderer *r, const Vertex *input, int count, int tex, float light) {
    Vertex clipped[MAX_VERTS+2];int n=0;
    Vertex a=input[count-1];int ain=a.z>=NEAR_Z;
    for (int i=0;i<count;++i) {
        Vertex b=input[i];int bin=b.z>=NEAR_Z;
        if (ain!=bin) clipped[n++]=lerp(a, b, (NEAR_Z-a.z)/(b.z-a.z));
        if (bin) clipped[n++]=b;
        a=b;ain=bin;
    }
    if (n<3) return;
    Projected p[MAX_VERTS+2];
    for (int i=0;i<n;++i) {
        float iz=1.0f/clipped[i].z;
        p[i]=(Projected){CX+clipped[i].x*FOCAL*iz, CY-clipped[i].y*FOCAL*iz,
                         iz, clipped[i].u*iz, clipped[i].v*iz};
    }
    for (int i=1;i<n-1;++i) triangle(r->g, p[0], p[i], p[i+1], tex, light);
}
static void wall(Renderer *r, Vec2 a, Vec2 b, float bottom, float top, int tex, float light, float offset) {
    if (top<=bottom+0.0001f) return;
    float dx=b.x-a.x, dy=b.y-a.y, u=pm_sqrtf(dx*dx+dy*dy)*0.625f;
    float v0=(bottom-offset)*0.625f, v1=(top-offset)*0.625f;
    Vertex q[]={camera(r, a.x, a.y, bottom, 0.0f, v0), camera(r, b.x, b.y, bottom, u, v0),
                camera(r, b.x, b.y, top, u, v1), camera(r, a.x, a.y, top, 0.0f, v1)};
    polygon(r, q, 4, tex, light);
}
static void geometry(Renderer *r) {
    Game *g=r->g;
    /* Small embedded level: brute-force geometry + Z buffer, not POOM's BSP/PVS. */
    for (int s=0;s<g->num_sectors;++s) {
        Sector *sec=&g->sectors[s];
        for (int ceil=0;ceil<2;++ceil) {
            float z=ceil ? sec->ceiling : sec->floor;
            Vertex q[MAX_VERTS];
            for (int i=0;i<sec->count;++i)
                q[i]=camera(r, sec->v[i].x, sec->v[i].y, z, sec->v[i].x*0.625f, sec->v[i].y*0.625f);
            polygon(r, q, sec->count, ceil ? sec->ceiling_tex : sec->floor_tex,
                    sec->light*(ceil ? 0.78f : 0.88f));
        }
        for (int e=0;e<sec->count;++e) {
            Vec2 a=sec->v[e], b=sec->v[(e+1)%sec->count];int n=sec->neighbor[e];
            float light=sec->light*(pm_absf(a.x-b.x)>pm_absf(a.y-b.y) ? 0.88f : 1.0f);
            if (n<0) wall(r, a, b, sec->floor, sec->ceiling, sec->wall_tex, light, 0.0f);
            else {
                Sector *next=&g->sectors[n];
                wall(r, a, b, sec->floor, next->floor, 7, light, 0.0f);
                wall(r, a, b, next->ceiling, sec->ceiling, sec->wall_tex, light, 0.0f);
                if (sec->door[e]>=0) {
                    float lo=pm_maxf(sec->floor, next->floor), hi=pm_minf(sec->ceiling, next->ceiling);
                    float offset=(hi-lo)*g->doors[sec->door[e]].open;
                    wall(r, a, b, lo+offset, hi, 5, 1.0f, offset);
                }
            }
        }
    }
}
static int compare_actor(const void *a, const void *b) {
    float x=((const SortItem *)a)->z, y=((const SortItem *)b)->z;
    return (x<y)-(x>y); /* No float-to-int subtraction; far to near. */
}
static void actors(Renderer *r) {
    Game *g=r->g;SortItem order[MAX_ACTORS];int count=0;
    for (int i=0;i<g->num_actors;++i) if (g->actors[i].active) {
        Actor *a=&g->actors[i];Vertex c=camera(r, a->x, a->y, a->z, 0.0f, 0.0f);
        if (c.z>NEAR_Z) order[count++]=(SortItem){i, c.z};
    }
    qsort(order, (size_t)count, sizeof *order, compare_actor);
    for (int i=0;i<count;++i) {
        Actor *a=&g->actors[order[i].index];
        float bob=a->kind==KEY ? pm_sinf(a->age)*0.08f : 0.0f;
        Vertex c=camera(r, a->x, a->y, a->z+bob, 0.0f, 0.0f);
        float iz=1.0f/c.z, scale=FOCAL*iz;
        float height=a->kind==ENEMY ? 1.65f : a->kind==EXIT_SIGN ? 1.4f : 0.9f;
        float width=height*(float)SPR_W/(float)SPR_H;
        float left=CX+c.x*scale-width*scale*0.5f;
        float top=CY-(c.y+height)*scale;
        float sw=width*scale, sh=height*scale;
        if (left+sw<=0.0f || left>=SCREEN_W || top+sh<=0.0f || top>=VIEW_H) continue;
        int x0=pm_floori(pm_clampf(left, 0.0f, (float)(SCREEN_W-1)));
        int x1=pm_ceili(pm_clampf(left+sw, 0.0f, (float)(SCREEN_W-1)));
        int y0=pm_floori(pm_clampf(top, 0.0f, (float)(VIEW_H-1)));
        int y1=pm_ceili(pm_clampf(top+sh, 0.0f, (float)(VIEW_H-1)));
        int frame=a->kind==ENEMY ? a->hp<=0 ? 2 : ((int)(a->age*5.0f)&1) : a->kind+2;
        int level=(int)pm_clampf(7.0f/(1.0f+c.z*0.022f), 0.0f, 7.0f);
        for (int y=y0;y<=y1;++y) for (int x=x0;x<=x1;++x) {
            int u=(int)(((float)x+0.5f-left)/sw*(float)SPR_W);
            int v=(int)(((float)y+0.5f-top)/sh*(float)SPR_H);
            if (u<0 || u>=SPR_W || v<0 || v>=SPR_H) continue;
            int p=y*SCREEN_W+x;
            if (iz<g->depth[p]) continue;
            uint8_t color=g->sprites[frame][v*SPR_W+u];
            if (color==TRANSPARENT) continue;
            if (a->kind==ENEMY && a->variant) {
                if (color==9) color=12;
                else if (color==10) color=13;
            }
            if (a->kind==ENEMY && a->hurt>0.0f && (color==9 || color==12)) color=11;
            g->pixels[p]=g->shade[level][color];g->depth[p]=iz;
        }
    }
}
static void line(Game *g, int x0, int y0, int x1, int y1, uint8_t c) {
    int dx=iabs(x1-x0), sx=x0<x1 ? 1 : -1, dy=-iabs(y1-y0), sy=y0<y1 ? 1 : -1, e=dx+dy;
    for (;;) {
        draw_rect(g, x0, y0, x0, y0, c);if (x0==x1 && y0==y1) break;
        int e2=2*e;if (e2>=dy) {e+=dy;x0+=sx;}if (e2<=dx) {e+=dx;y0+=sy;}
    }
}
static int mapx(float x) {return (int)(10.0f+x*3.3f);}
static int mapy(float y) {return (int)(94.0f-y*3.3f);}
static void automap(Game *g) {
    draw_rect(g, 2, 10, 125, 106, 0);
    for (int s=0;s<g->num_sectors;++s) {
        Sector *a=&g->sectors[s];
        for (int e=0;e<a->count;++e) {
            Vec2 p=a->v[e], q=a->v[(e+1)%a->count];
            uint8_t c=(uint8_t)(a->door[e]>=0 ? 14 : a->neighbor[e]>=0 ? 2 : 5);
            line(g, mapx(p.x), mapy(p.y), mapx(q.x), mapy(q.y), c);
        }
    }
    for (int i=0;i<g->num_actors;++i) {
        Actor *a=&g->actors[i];if (!a->active || (a->kind==ENEMY && a->hp<=0)) continue;
        uint8_t c=(uint8_t)(a->kind==ENEMY ? 9 : a->kind==KEY ? 14 : 13);
        int x=mapx(a->x), y=mapy(a->y);draw_rect(g, x-1, y-1, x+1, y+1, c);
    }
    int x=mapx(g->p.x), y=mapy(g->p.y);
    draw_rect(g, x-1, y-1, x+1, y+1, 11);
    line(g, x, y, x+(int)(pm_cosf(g->p.angle)*5.0f), y-(int)(pm_sinf(g->p.angle)*5.0f), 7);
    draw_text(g, 8, 15,"MAP  BLUE KEY - GREEN EXIT", 5);
}
static void weapon(Game *g) {
    int bob=(int)(pm_sinf(g->p.walk_phase)*1.5f), y=78+bob;
    if (g->p.shot>0.16f) y+=3;
    for (int k=0;k<32;++k) {
        int w=6+k/3;draw_rect(g, 64-w, y+k, 64+w, y+k, 1);
        if (k>1) draw_rect(g, 65-w, y+k, 63+w, y+k, (uint8_t)(k>18 ? 2 : 3));
    }
    draw_rect(g, 59, y+2, 69, y+19, 4);draw_rect(g, 61, y+1, 66, y+18, 5);
    draw_rect(g, 62, y+1, 65, y+4, 1);draw_rect(g, 60, y+13, 68, y+15, 2);
    draw_rect(g, 56, y+22, 62, y+31, 8);draw_rect(g, 51, y+28, 57, y+31, 6);
    draw_rect(g, 68, y+23, 74, y+31, 8);draw_rect(g, 73, y+28, 77, y+31, 6);
    if (g->p.shot>0.16f) {
        for (int k=-8;k<=8;++k) {
            int w=8-iabs(k);if ((k&3)==0) w+=2;
            draw_rect(g, 64-w, y-5+k, 64+w, y-5+k, 10);
        }
        draw_rect(g, 62, y-10, 66, y, 11);draw_rect(g, 59, y-7, 69, y-3, 11);
        draw_rect(g, 63, y-7, 65, y-3, 7);
    }
}
void render_frame(Game *g) {
    memset(g->pixels, 0, SCREEN_W*SCREEN_H);
    memset(g->depth, 0, (size_t)SCREEN_W*VIEW_H*sizeof *g->depth);
    g->triangles=0;g->fragments=0;
    Renderer r={g, pm_cosf(g->p.angle), pm_sinf(g->p.angle), g->p.z};
    geometry(&r);actors(&r);
    weapon(g);
    line(g, 60, 53, 62, 53, 5);line(g, 66, 53, 68, 53, 5);
    line(g, 64, 49, 64, 51, 5);line(g, 64, 55, 64, 57, 5);
    if (g->p.hurt>0.0f) {
        draw_rect(g, 0, 0, 127, 1, 9);draw_rect(g, 0, 0, 1, 109, 9);
        draw_rect(g, 126, 0, 127, 109, 9);draw_rect(g, 0, 108, 127, 109, 9);
    }
    if (g->show_map) automap(g);
    draw_rect(g, 0, VIEW_H, 127, 127, 1);draw_rect(g, 0, VIEW_H, 127, VIEW_H, 4);
    draw_text(g, 4, 113,"HP", 4);draw_number(g, 4, 120, (unsigned)g->p.hp, g->p.hp>25 ? 13 : 10);
    draw_text(g, 31, 113,"AMMO", 4);draw_number(g, 31, 120, (unsigned)g->p.ammo, 11);
    draw_text(g, 63, 113,"KILLS", 4);draw_number(g, 63, 120, (unsigned)g->p.kills, 6);
    draw_text(g, 71, 120,"/7", 4);draw_text(g, 101, 113,"KEY", 4);
    draw_text(g, 101, 120, g->p.key ? "YES" : "NO", g->p.key ? 14 : 3);
    if (g->message_time>0.0f) { draw_rect(g, 1, 1, 126, 8, 0);draw_text(g, 4, 3, g->message, 6); }
    if (g->paused || g->p.hp<=0 || g->p.won) {
        draw_rect(g, 13, 40, 114, 68, 1);draw_rect(g, 14, 41, 113, 42, g->p.won ? 13 : 10);
        draw_text(g, 25, 48, g->p.won ? "SECTOR CLEAR" : g->p.hp<=0 ? "YOU DIED" : "PAUSED", 7);
        draw_text(g, 25, 59, g->paused ? "P TO RESUME" : "R TO RESTART", 5);
    }
}

/* ==================== sixel.c ==================== */
#include <string.h>
#define OUT_SIZE 4096
#define MAX_WIDTH (SCREEN_W*4)
typedef struct {
    SixelWrite write;void *context;
    char buffer[OUT_SIZE];unsigned used;size_t bytes;int error;
} Output;
static void flush(Output *o) {
    if (o->error || !o->used) return;
    if (o->write(o->context, o->buffer, o->used)<0) o->error=1;
    else o->bytes+=o->used;
    o->used=0;
}
static void put(Output *o, int c) {
    if (o->error) return;
    o->buffer[o->used++]=(char)c;if (o->used==OUT_SIZE) flush(o);
}
static void text(Output *o, const char *s) {while (*s) put(o, *s++);}
static void decimal(Output *o, unsigned n) {
    char s[10];unsigned k=0;
    do {s[k++]=(char)('0'+n%10u);n/=10u;} while (n);
    while (k) put(o, s[--k]);
}
static void run(Output *o, int code, unsigned n) {
    if (n>=4u) {put(o,'!');decimal(o, n);put(o, code);}
    else while (n--) put(o, code);
}
int sixel_encode(const uint8_t *pixels, int width, int height, int scale,
                 SixelWrite write, void *context, size_t *bytes) {
    if (bytes) *bytes=0;
    if (!pixels || !write || width<1 || width>SCREEN_W || height<1 || height>SCREEN_H || scale<1 || scale>4) return -1;
    for (int i=0;i<width*height;++i) if (pixels[i]>15) return -1;
    Output out={.write=write, .context=context};
    int w=width*scale, h=height*scale;
    /* Full repaint, transparent background: every actual pixel is still set. */
    text(&out,"\033[H\033P0;1;0q\"1;1;");decimal(&out, (unsigned)w);put(&out,';');decimal(&out, (unsigned)h);
    for (unsigned c=0;c<16;++c) {
        put(&out,'#');decimal(&out, c);text(&out,";2");
        for (unsigned j=0;j<3;++j) {
            put(&out,';');decimal(&out, ((unsigned)poom_palette[c][j]*100u+127u)/255u);
        }
    }
    uint8_t masks[16][MAX_WIDTH];
    for (int y=0;y<h && !out.error;y+=6) {
        memset(masks, 0, sizeof masks);unsigned used=0;
        for (int k=0;k<6 && y+k<h;++k) for (int x=0;x<w;++x) {
            unsigned color=pixels[((y+k)/scale)*width+x/scale];
            masks[color][x]|=(uint8_t)(1u<<k);used|=1u<<color;
        }
        int first=1;
        for (unsigned c=0;c<16;++c) if (used&(1u<<c)) {
            if (!first) put(&out,'$');
            first=0;
            put(&out,'#');decimal(&out, c);
            int end=w;while (end>0 && masks[c][end-1]==0) --end;
            for (int x=0;x<end;) {
                int next=x+1;while (next<end && masks[c][next]==masks[c][x]) ++next;
                run(&out, 63+masks[c][x], (unsigned)(next-x));x=next;
            }
        }
        if (y+6<h) put(&out,'-');
    }
    text(&out,"\033\\");flush(&out);
    if (bytes) *bytes=out.bytes;
    return out.error ? -1 : 0;
}


/* ==================== platform_pico2.c ==================== */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "timer.h"

volatile uint32_t ticks;
static int terminal_active;

uint32_t platform_millis(void){return gettick();}
int platform_stopped(void){return 0;}
int platform_input_closed(void){return 0;}

int platform_init(int interactive)
{
    if(!interactive)return 0;
    static const char enter[]="\033[?1049h\033[?25l\033[2J\033[H";
    terminal_active=1;
    return console_write(enter, sizeof enter-1);
}

void platform_shutdown(void)
{
    if(!terminal_active)return;
    static const char leave[]="\033\\\033[0m\033[?25h\033[?1049l";
    console_write(leave, sizeof leave-1);
    terminal_active=0;
}

/* ==================== main.c ==================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int output(void *context, const void *data, unsigned size) {
    (void)context;return console_write(data, size);
}
static int discard(void *context, const void *data, unsigned size) {
    (void)context;(void)data;(void)size;return 0;
}
static int parse_positive(const char *s, int maximum) {
    if (!*s) return -1;
    unsigned n=0;
    for (;*s;++s) {
        if (*s<'0' || *s>'9' || n>(unsigned)maximum/10u) return -1;
        n=n*10u+(unsigned)(*s-'0');
        if (n>(unsigned)maximum) return -1;
    }
    return n ? (int)n : -1;
}
static void select_scene(Game *g, int scene) {
    static const float views[][3]={{2.2f, 2.5f, 0.0f}, {12.0f, 2.8f, 0.55f},
        {14.0f, 13.0f, 1.570796327f}, {24.0f, 2.0f, 0.75f}, {27.0f, 11.8f, 1.570796327f}};
    g->p.x=views[scene][0];g->p.y=views[scene][1];g->p.angle=views[scene][2];
    g->p.sector=world_sector(g, g->p.x, g->p.y);
    g->p.z=g->sectors[g->p.sector].floor+EYE_HEIGHT;
    g->message_time=0.0f;
}
static void demo(Game *g, unsigned frame) {
    int scene=(int)((frame/160u)%5u);select_scene(g, scene);
    float phase=(float)(frame%160u)*0.039269908f;
    g->p.angle+=pm_sinf(phase)*0.45f;
    g->p.walk_phase=phase;g->p.shot=frame%80u<8u ? 0.22f : 0.0f;
    g->doors[0].open=0.5f+pm_sinf(phase)*0.5f;
    for (int i=0;i<g->num_actors;++i) g->actors[i].age=phase;
}
typedef struct {int state;uint32_t when;} Escape;
static void input(Game *g, Escape *e, int c) {
    if (e->state==1) {
        if (c=='[' || c=='O') {e->state=2;return;}
        e->state=0;game_key(g, 27);return;
    }
    if (e->state==2) {
        if (c>='0' && c<='9') return;
        if (c==';') return;
        e->state=0;
        if (c=='A') game_key(g,'w');else if (c=='B') game_key(g,'s');
        else if (c=='C') game_key(g,'d');else if (c=='D') game_key(g,'a');
        return;
    }
    if (c==27) {e->state=1;e->when=platform_millis();return;}
    game_key(g, c);
}
int main(void) {
    int argc = 1;
    char *argv[] = {"./poom", NULL};
    int scale=1, fps=4, frames=0, autodemo=0, dump=0, bench=0, scene=0;
    for (int i=1;i<argc;++i) {
        if (!strcmp(argv[i],"--help")) {
            puts("POOM-F: original float-only, silent 2.5D Sixel FPS\n"
                 "Usage: ./poom [--scale 1..4] [--fps 1..120] [--frames N]\n"
                 "              [--demo] [--dump] [--scene 0..4] [--benchmark N]\n"
                 "W/S move, A/D turn, Q/E strafe, arrows move/turn\n"
                 "Space/J fire, F/Enter use door, M/Tab map, P pause, R restart, X/Esc quit\n"
                 "Find the blue key, open the blue door, reach the green exit.\n"
                 "--dump writes one Sixel frame to stdout; --demo is a camera tour.\n"
                 "No game asset files, no audio, no libm or graphics library required.");return 0;
        } else if (!strcmp(argv[i],"--demo")) autodemo=1;
        else if (!strcmp(argv[i],"--dump")) dump=1;
        else if (i+1<argc && !strcmp(argv[i],"--scale")) {scale=parse_positive(argv[++i], 4);if (scale<0) goto badarg;}
        else if (i+1<argc && !strcmp(argv[i],"--fps")) {fps=parse_positive(argv[++i], 120);if (fps<0) goto badarg;}
        else if (i+1<argc && !strcmp(argv[i],"--frames")) {frames=parse_positive(argv[++i], 1000000);if (frames<0) goto badarg;}
        else if (i+1<argc && !strcmp(argv[i],"--benchmark")) {bench=parse_positive(argv[++i], 1000000);if (bench<0) goto badarg;}
        else if (i+1<argc && !strcmp(argv[i],"--scene")) {
            const char *s=argv[++i];if (s[0]<'0' || s[0]>'4' || s[1]) goto badarg;scene=s[0]-'0';
        } else goto badarg;
    }
    Game *g=game_create();
    if (!g) {fputs("Memory allocation failed.\n", stderr);return 1;}
    if (scene) select_scene(g, scene);
    if (platform_init(!dump && !bench)<0) {
        fputs("Interactive mode requires a terminal on stdin and stdout. Use --dump or --benchmark for headless operation.\n", stderr);
        game_destroy(g);return 1;
    }
    int status=0;
    if (dump) {
        render_frame(g);status=sixel_encode(g->pixels, SCREEN_W, SCREEN_H, scale, output, NULL, NULL)<0;
    } else if (bench) {
        uint32_t start=platform_millis();size_t total=0;unsigned completed=0;
        for (int f=0;f<bench && !platform_stopped();++f) {
            size_t bytes;demo(g, (unsigned)f);render_frame(g);
            if (sixel_encode(g->pixels, SCREEN_W, SCREEN_H, scale, discard, NULL, &bytes)<0) {status=1;break;}
            total+=bytes;++completed;
        }
        uint32_t ms=platform_millis()-start;
        printf("frames=%u elapsed_ms=%u fps_x1000=%llu avg_sixel_bytes=%zu\n", completed, ms,
               ms ? (unsigned long long)completed*1000000u/ms : 0ull, completed ? total/completed : 0u);
        printf("game_bytes=%zu framebuffer_bytes=%d depth_bytes=%zu encoder_stack_approx=%d hash=%08x\n",
               sizeof *g, SCREEN_W*SCREEN_H, (size_t)SCREEN_W*VIEW_H*sizeof(float), 16*SCREEN_W*4+4096, frame_hash(g));
    } else {
        Escape escape={0};uint32_t last=platform_millis(), deadline=last;unsigned frame=0;
        unsigned interval=1000u/(unsigned)fps, remainder=0;
        while (!g->quit && !platform_stopped() && !platform_input_closed() && (!frames || frame<(unsigned)frames)) {
            uint32_t now=platform_millis();unsigned elapsed=now-last;last=now;
            if (autodemo) demo(g, frame);else game_step(g, elapsed);
            render_frame(g);
            if (sixel_encode(g->pixels, SCREEN_W, SCREEN_H, scale, output, NULL, NULL)<0) {status=1;break;}
            ++frame;deadline+=interval;remainder+=1000u%(unsigned)fps;
            if (remainder>=(unsigned)fps) {++deadline;remainder-=(unsigned)fps;}
            if ((int32_t)(platform_millis()-deadline)>100) deadline=platform_millis();
            /* Poll even after a slow frame, so input is never starved by output. */
            for (unsigned reads=0;reads<256 && !g->quit && !platform_stopped();++reads) {
                now=platform_millis();int32_t left=(int32_t)(deadline-now);
                unsigned wait=left>0 ? (unsigned)left : 0u;
                if (escape.state) {
                    unsigned age=(uint32_t)(now-escape.when), limit=age<40u ? 40u-age : 0u;
                    if (wait>limit) wait=limit;
                }
                int c=getchar_timeout(wait);
                if (c>=0) input(g, &escape, c);
                if (escape.state && (uint32_t)(platform_millis()-escape.when)>=40u) {
                    int lone=escape.state==1;escape.state=0;if (lone) game_key(g, 27);
                }
                if (c<0 && (int32_t)(platform_millis()-deadline)>=0) break;
                if (platform_input_closed()) break;
            }
        }
    }
    platform_shutdown();game_destroy(g);return status;
badarg:
    fputs("Invalid argument. Run ./poom --help.\n", stderr);return 2;
}
