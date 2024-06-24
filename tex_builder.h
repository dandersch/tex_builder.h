#pragma once
/*
   IDEA: a texture generation api in C that mimicks the following javascript api:

   texture(0x468f).create(); // Sky
   texture(0x340f).grunge().create(); // Grass
   texture(0x236f).noise(0x1005, 4.5).noise(Ox100a, 2.5).create(); // Water

   inspiration:
   https://x.com/phoboslab/status/1432363394418491394/photo/1
*/

/*
  Limitations:
  - color (first argument) cannot be specified as an initializer containing commas, e.g. tex_build(texture((color_t){0.2,0.3,0.5}), ...)
  - requires GNU C-extension of statement expressions
  - ...
*/

/* Possible extensions:
 *
 * stencil/blit(t,x,y): draws tex t to the texture at (x,y)
 *
 * line:   draw a line segment between two points with a color
 * blend:  blend two textures together with a specified blending mode (e.g., alpha blending, additive blending)
 * resize: resize the texture to a new width and height using interpolation techniques (e.g., bilinear interpolation).
 * rotate: rotate the texture by a specified angle around a pivot point.
 * flip:   flip texture horizontally or vertically
 *
 * outline version of rect, circle
 *
 * generate normal maps along texture for operations that add depth (e.g. insets)
 *
 * cpp wrapper (avoids need of statement expressions)
 */

#include <stddef.h>

// NOTE RGBA vs BGRA layout could be set with a macro (?)
typedef struct color_t       { float r; float g; float b; float a;        } color_t;
typedef struct texture_t
{
    size_t width;
    size_t height;
    color_t* rgb;

    /* unused */
    size_t atlas_width;
    size_t atlas_height;
    int owns_buffer;

    /* shows where a subtexture starts OR where the texture atlas is free */
    size_t x_start; // show subtextures
    size_t y_start; // for subtextures
} texture_t;

/* wrapper for type safety */
typedef struct tex_builder_t {
    texture_t tex;

    size_t atlas_width;
    size_t atlas_height;

    /* bounds of where we are currently drawing */
    int x_start, y_start;
    int width, height;

    /* used in for loop macros */
    int i;
} tex_builder_t;

/*
 * api
 */
/* building api (allocating) */
tex_builder_t texture(int w, int h, color_t rgb);                     // use for creating texture atlas
tex_builder_t tex_copy(texture_t tex);                                // deep copy a texture

/* building api (non-allocating) */

#define subtexture(...) _subtexture(&temp.tex, __VA_ARGS__)
tex_builder_t _subtexture(texture_t* atlas,int w, int h, color_t rgba); // get a texture from an atlas

/* transformation api */
//#define noise(...) _noise(&temp, __VA_ARGS__) // TODO old
#define noise(...) __noise(temp, __VA_ARGS__)
void   _noise(tex_builder_t* tex, float intensity);
void  __noise(tex_builder_t  tex, float intensity);
#define grunge(...) _grunge(&temp, __VA_ARGS__)
void   _grunge(tex_builder_t* tex, float intensity); /* TODO */
#define smear(...) _smear(&temp, ##__VA_ARGS__)
void   _smear(tex_builder_t* tex, color_t rgb);      /* TODO */
#define rect(...) _rect(&temp, __VA_ARGS__)
void   _rect(tex_builder_t* tex, unsigned int x, unsigned int y, unsigned int width, unsigned int height, color_t color);

static tex_builder_t   __rect(tex_builder_t texer, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
    tex_builder_t temp = texer;
    temp.x_start = x;
    temp.y_start = y;
    temp.width   = width;
    temp.height  = height;
    return temp;
}

#define color(...) _color(temp, __VA_ARGS__)
static void   _color(tex_builder_t tex, color_t color)
{
    /* TODO check if we are out of bounds */

    /* color the subtexture */
    for (size_t y = tex.y_start; y < (tex.y_start + tex.height); y++) {
        for (size_t x = tex.x_start; x < (tex.x_start + tex.width); x++) {
            size_t index = (y * tex.atlas_width) + x;
            tex.tex.rgb[index] = color;
        }
    }
}

/* for debugging */
#define pixel(...) _pixel(temp, ##__VA_ARGS__)
static void   _pixel(tex_builder_t tex)
{
   size_t index = (tex.y_start * tex.atlas_width) + tex.x_start;
   tex.tex.rgb[index] = (color_t){1,1,1,1};
}

#define circle(...) _circle(&temp, __VA_ARGS__)
void   _circle(tex_builder_t* tex, unsigned int x, unsigned int y, unsigned int radius, color_t color);
#define flip(...) _flip(&temp, ##__VA_ARGS__)
void   _flip(tex_builder_t* tex);
#define mirror(...) _mirror(&temp, ##__VA_ARGS__)
void   _mirror(tex_builder_t* tex);

/* helper macros */
#define TOKEN_PASTE(a, b) a##b
#define CONCAT(a,b) TOKEN_PASTE(a,b)
#define UNIQUE_VAR(name) CONCAT(name, __LINE__)
#define min(a, b) ((a < b) ? a : b)

#define scope_tex_build(tex, builder) \
    for (tex_builder_t temp = builder; temp.i == 0; (temp.i+=1, atlas = _create(temp)))
#define scope_tex_rect(x,y,h,w) \
    for (int UNIQUE_VAR(old_x_start) = temp.x_start, \
             UNIQUE_VAR(old_y_start) = temp.y_start, \
             UNIQUE_VAR(old_width)   = temp.width,   \
             UNIQUE_VAR(old_height)  = temp.height,  \
             UNIQUE_VAR(j)           = (temp = __rect(temp,                        \
                                                      x + UNIQUE_VAR(old_x_start), \
                                                      y + UNIQUE_VAR(old_y_start), \
                                                      min(w,temp.width),           \
                                                      min(h,temp.height)),         \
                                        0);                                        \
         UNIQUE_VAR(j) == 0; \
         (UNIQUE_VAR(j)+=1,                       \
          temp.x_start = UNIQUE_VAR(old_x_start), \
          temp.y_start = UNIQUE_VAR(old_y_start), \
          temp.width   = UNIQUE_VAR(old_width),   \
          temp.height = UNIQUE_VAR(old_height)))




/* usage: tex_build(build(), transform(), transform(), ...) */
#define tex_build(tex, ...) ({tex_builder_t temp = tex; __VA_ARGS__; _create(temp); })

/* internal */
#ifdef TEX_BUILDER_IMPLEMENTATION
#include <stdlib.h> // for malloc & rand()
#include <math.h>   // for RAND_MAX, ...
#include <assert.h> // TODO take in assert macro from user
#include <time.h>   // for seeding srand()
void _noise(tex_builder_t* texer, float intensity)  {
    texture_t* tex = &(texer->tex);

    static unsigned int random = 0;
    srand(time(0) + random);
    random = rand();

    for (size_t i = 0; i < tex->atlas_width * tex->height; i++) {

        // Add noise to each color component based on intensity
        tex->rgb[i].r += intensity * ((float)rand() / (float) RAND_MAX - 0.5f);
        tex->rgb[i].g += intensity * ((float)rand() / (float) RAND_MAX - 0.5f);
        tex->rgb[i].b += intensity * ((float)rand() / (float) RAND_MAX - 0.5f);

        // Ensure color components are within [0, 1] range
        tex->rgb[i].r = fminf(fmaxf(tex->rgb[i].r, 0.0f), 1.0f);
        tex->rgb[i].g = fminf(fmaxf(tex->rgb[i].g, 0.0f), 1.0f);
        tex->rgb[i].b = fminf(fmaxf(tex->rgb[i].b, 0.0f), 1.0f);
    }
}

void __noise(tex_builder_t texer, float intensity)  {
    texture_t* tex = &(texer.tex);

    size_t index = (texer.y_start * texer.atlas_width) + texer.x_start;

    static unsigned int random = 0;
    srand(time(0) + random);
    random = rand();

    for (size_t y = texer.y_start; y < (texer.y_start + texer.height); y++) {
        for (size_t x = texer.x_start; x < (texer.x_start + texer.width); x++) {
            size_t idx = (y * texer.atlas_width) + x;
            // Add noise to each color component based on intensity
            tex->rgb[idx].r += intensity * ((float)rand() / (float) RAND_MAX - 0.5f);
            tex->rgb[idx].g += intensity * ((float)rand() / (float) RAND_MAX - 0.5f);
            tex->rgb[idx].b += intensity * ((float)rand() / (float) RAND_MAX - 0.5f);

            // Ensure color components are within [0, 1] range
            tex->rgb[idx].r = fminf(fmaxf(tex->rgb[idx].r, 0.0f), 1.0f);
            tex->rgb[idx].g = fminf(fmaxf(tex->rgb[idx].g, 0.0f), 1.0f);
            tex->rgb[idx].b = fminf(fmaxf(tex->rgb[idx].b, 0.0f), 1.0f);
        }
    }
}

void _rect(tex_builder_t* texer, unsigned int x, unsigned int y, unsigned int width, unsigned int height, color_t color)  {
    texture_t* tex = &(texer->tex);

    /* check if rectangle fits into the texture */
    if (x + width > tex->width || y + height > tex->height) { return; }

    /* fill rectangle region */
    for (unsigned int i = y; i < y + height; i++) {
        for (unsigned int j = x; j < x + width; j++) {
            size_t index = i * tex->width + j;
            tex->rgb[index] = color;
        }
    }
}

void _circle(tex_builder_t* texer, unsigned int x, unsigned int y, unsigned int radius, color_t color) {
    texture_t* tex = &(texer->tex);

    // filled circle using midpoint circle algorithm
    int cx = radius - 1;
    int cy = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    while (cx >= cy) {
        for (int i = x - cx; i <= x + cx; i++) {
            if (i >= 0 && i < tex->width && y + cy >= 0 && y + cy < tex->height) {
                size_t index = (y + cy) * tex->width + i;
                tex->rgb[index] = color;
            }
            if (i >= 0 && i < tex->width && y - cy >= 0 && y - cy < tex->height) {
                size_t index = (y - cy) * tex->width + i;
                tex->rgb[index] = color;
            }
        }

        for (int i = x - cy; i <= x + cy; i++) {
            if (i >= 0 && i < tex->width && y + cx >= 0 && y + cx < tex->height) {
                size_t index = (y + cx) * tex->width + i;
                tex->rgb[index] = color;
            }
            if (i >= 0 && i < tex->width && y - cx >= 0 && y - cx < tex->height) {
                size_t index = (y - cx) * tex->width + i;
                tex->rgb[index] = color;
            }
        }

        if (err <= 0) {
            cy++;
            err += dy;
            dy += 2;
        }

        if (err > 0) {
            cx--;
            dx += 2;
            err += dx - (radius << 1);
        }
    }
}

void _mirror(tex_builder_t* texer) {
    texture_t* tex = &(texer->tex);
    color_t* mirrored = (color_t*) malloc(tex->width * tex->height * sizeof(color_t));

    /* flip each row horizontally */
    for (size_t y = 0; y < tex->height; y++) {
        for (size_t x = 0; x < tex->width; x++) {
            mirrored[y * tex->width + x] = tex->rgb[y * tex->width + (tex->width - 1 - x)];
        }
    }

    for (size_t i = 0; i < tex->width * tex->height; i++) {
        tex->rgb[i] = mirrored[i];
    }

    free(mirrored);
}

void _flip(tex_builder_t* texer) {
    texture_t* tex = &(texer->tex);
    color_t* flipped = (color_t*)malloc(tex->width * tex->height * sizeof(color_t));

    /* flip each column vertically */
    for (size_t y = 0; y < tex->height; y++) {
        for (size_t x = 0; x < tex->width; x++) {
            flipped[y * tex->width + x] = tex->rgb[(tex->height - 1 - y) * tex->width + x];
        }
    }

    for (size_t i = 0; i < tex->width * tex->height; i++) {
        tex->rgb[i] = flipped[i];
    }

    free(flipped);
}

void _grunge(tex_builder_t* texer, float intensity) {
    texture_t* tex = &(texer->tex);
    tex->rgb[0].b += intensity;
}

void _smear(tex_builder_t* texer, color_t rgb) {
    texture_t* tex = &(texer->tex);
    tex->rgb[0] = rgb;
}

tex_builder_t texture(int w, int h, color_t rgba) {
    tex_builder_t texer;

    texer.atlas_width  = w;
    texer.atlas_height = h;
    texer.width        = w;
    texer.height       = h;
    texer.x_start      = 0;
    texer.y_start      = 0;
    texer.i            = 0;

    texer.tex.width        = w;
    texer.tex.height       = h;
    texer.tex.atlas_width  = w; // TODO old
    texer.tex.atlas_height = h; // TODO old
    texer.tex.x_start      = 0;
    texer.tex.y_start      = 0;
    texer.tex.owns_buffer  = 1;
    texer.tex.rgb         = malloc(w * h * sizeof(color_t));
    for (int i = 0; i < (w * h); i++)
    {
        texer.tex.rgb[i] = rgba;
    }
    return texer;
}

tex_builder_t _subtexture(texture_t* atlas, int w, int h, color_t rgba) {
    assert(atlas->atlas_width >= w && atlas->atlas_height >= h && "Subtexture is bigger than the atlas\n");

    tex_builder_t texer;
    texture_t* subtex = &texer.tex;

    size_t start_index = (atlas->width * atlas->y_start) + atlas->x_start;

    // Set the subtexture properties
    subtex->width = w;
    subtex->height = h;

    subtex->rgb          = &(atlas->rgb[start_index]); // set buffer pointer to start of subtexture
    subtex->atlas_width  = atlas->width;
    subtex->atlas_height = atlas->height;
    subtex->owns_buffer  = 0;                         // subtexture did not allocate buffer
    subtex->x_start      = atlas->x_start;
    subtex->y_start      = atlas->y_start;

    /* push onto the atlas */
    assert(!(atlas->x_start + subtex->width > atlas->atlas_width) && "Pushed beyond atlas width"); // TODO handle this case
    if (atlas->x_start + subtex->width == atlas->atlas_width) {
        atlas->x_start       = 0;
        assert(!(atlas->y_start + subtex->height > atlas->atlas_height) && "Pushed beyond atlas height"); // TODO handle this case
        atlas->y_start      += subtex->height;
    } else {
        atlas->x_start       = atlas->x_start + subtex->width;
    }

    /* color the subtexture */
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t index = (y * atlas->atlas_width) + x;
            if (x < atlas->atlas_width && y < atlas->atlas_height) {
                subtex->rgb[index] = rgba;
            }
        }
    }
    return texer;
}

tex_builder_t tex_copy(texture_t tex) {
    tex_builder_t texer;
    texer.tex.width       = tex.width;
    texer.tex.height      = tex.height;
    texer.tex.rgb         = malloc(tex.width * tex.height * sizeof(color_t));
    for (int i = 0; i < (tex.width * tex.height); i++)
    {
        texer.tex.rgb[i] = tex.rgb[i];
    }
    return texer;
}

texture_t _create(tex_builder_t texer) { return texer.tex; }
#endif
