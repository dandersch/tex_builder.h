#pragma once
/*
   IDEA: an immediate-mode texture generation api in C that is nice to use

   inspiration:
   https://x.com/phoboslab/status/1432363394418491394/photo/1

   Limitations:
   - ...
*/

/* Possible extensions:
 *
 * combining textures:
 *   stencil/blit(t,x,y): draws tex t to the texture at (x,y)
 *   blend:  blend two textures together with a specified blending mode (e.g., alpha blending, additive blending)
 *
 * drawing operations:
 *   line:   draw a line segment between two points with a color
 *   resize: resize the texture to a new width and height using interpolation techniques (e.g., bilinear interpolation).
 *   rotate: rotate the texture by a specified angle around a pivot point.
 *   text:   draw string with font at position with size or fit the rect
 *
 * scopes:
 *   scope_centered() : draw everything at the center
 *   scope_rect_cut_{top,left,right,bottom}(): rect_cut api scopes
 *   scope_...
 *
 * general:
 *   make possible to regenerate without reallocating
 *   outline version of rect, circle
 *   generate normal maps along texture for operations that add depth (e.g. insets)
 *   make work in glsl shader code
 *   check if it works with C++ and MSVC
 */

#include <stddef.h>

// NOTE RGBA vs BGRA layout could be set with a macro
typedef struct color_t       { float r; float g; float b; float a;        } color_t;
typedef struct texture_t     { size_t width; size_t height; color_t* rgb; } texture_t;

/* used internally */
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
tex_builder_t texture(int w, int h, color_t rgb);
tex_builder_t tex_copy(texture_t tex); // TODO unused


/* transformation api */
#define        color(...) temp = _color(temp, __VA_ARGS__)
tex_builder_t _color(tex_builder_t tex, color_t color);

#define        noise(...) temp = _noise(temp, __VA_ARGS__)
tex_builder_t _noise(tex_builder_t  tex, float intensity);

#define        flip(...)    temp = _flip(temp, ##__VA_ARGS__)
tex_builder_t _flip(tex_builder_t texer);
#define        mirror(...)  temp = _mirror(temp, ##__VA_ARGS__)
tex_builder_t _mirror(tex_builder_t tex);

/* TODO reimplement these */
#define rect(...) _rect(&temp, __VA_ARGS__)
void   _rect(tex_builder_t* tex, unsigned int x, unsigned int y, unsigned int width, unsigned int height, color_t color);
#define circle(...) _circle(&temp, __VA_ARGS__)
void   _circle(tex_builder_t* tex, unsigned int x, unsigned int y, unsigned int radius, color_t color);

/* for debugging */
#define      pixel(...) temp = _pixel(temp, ##__VA_ARGS__) // places a pixel at the current {x,y} start
static tex_builder_t _pixel(tex_builder_t tex) {
   size_t index = (tex.y_start * tex.atlas_width) + tex.x_start;
   tex.tex.rgb[index] = (color_t){1,1,1,1};
   return tex;
}

/* called by internally by macros */
tex_builder_t   __rect(tex_builder_t texer, unsigned int x, unsigned int y, unsigned int width, unsigned int height);


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
                                                      min(h,temp.height)), 0);     \
         UNIQUE_VAR(j) == 0; \
         (UNIQUE_VAR(j)+=1,                       \
          temp.x_start = UNIQUE_VAR(old_x_start), \
          temp.y_start = UNIQUE_VAR(old_y_start), \
          temp.width   = UNIQUE_VAR(old_width),   \
          temp.height = UNIQUE_VAR(old_height)))

/* internal */
#ifdef TEX_BUILDER_IMPLEMENTATION
#include <stdlib.h> // for malloc & rand()
#include <math.h>   // for RAND_MAX, ...
#include <assert.h> // TODO take in assert macro from user
#include <time.h>   // for seeding srand()
tex_builder_t _color(tex_builder_t tex, color_t color) {
    /* color the subtexture */
    for (size_t y = tex.y_start; y < (tex.y_start + tex.height); y++) {
        for (size_t x = tex.x_start; x < (tex.x_start + tex.width); x++) {
            size_t index = (y * tex.atlas_width) + x;
            tex.tex.rgb[index] = color;
        }
    }

    return tex;
}

tex_builder_t _noise(tex_builder_t texer, float intensity)  {
    texture_t* tex = &(texer.tex);

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

    return texer;
}

tex_builder_t _mirror(tex_builder_t texer) {
    /* mirror texture column by column */
    for (size_t y = 0; y < texer.height; y++) {
        for (size_t x = 0; x < texer.width / 2; x++) {
            size_t idx_left = (texer.y_start + y) * texer.atlas_width + (texer.x_start + x);
            size_t idx_right = (texer.y_start + y) * texer.atlas_width + (texer.x_start + texer.width - 1 - x);
            color_t temp = texer.tex.rgb[idx_left];
            texer.tex.rgb[idx_left] = texer.tex.rgb[idx_right];
            texer.tex.rgb[idx_right] = temp;
        }
    }

    return texer;
}

tex_builder_t _flip(tex_builder_t texer) {
    /* flip the texture upside down row by row */
    for (size_t y = 0; y < texer.height / 2; y++) {
        for (size_t x = 0; x < texer.width; x++) {
            size_t idx_top = (texer.y_start + y) * texer.atlas_width + (texer.x_start + x);
            size_t idx_bottom = (texer.y_start + texer.height - 1 - y) * texer.atlas_width + (texer.x_start + x);
            color_t temp = texer.tex.rgb[idx_top];
            texer.tex.rgb[idx_top] = texer.tex.rgb[idx_bottom];
            texer.tex.rgb[idx_bottom] = temp;
        }
    }

    return texer;
}

tex_builder_t texture(int w, int h, color_t rgba) {
    tex_builder_t texer;

    /* init builder */
    texer.atlas_width  = w;
    texer.atlas_height = h;
    texer.width        = w;
    texer.height       = h;
    texer.x_start      = 0;
    texer.y_start      = 0;
    texer.i            = 0;

    /* init texture */
    texer.tex.width    = w;
    texer.tex.height   = h;
    texer.tex.rgb      = malloc(w * h * sizeof(color_t));
    for (int i = 0; i < (w * h); i++)
    {
        texer.tex.rgb[i] = rgba;
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

tex_builder_t __rect(tex_builder_t texer, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    tex_builder_t temp = texer;
    temp.x_start = x;
    temp.y_start = y;
    temp.width   = width;
    temp.height  = height;
    return temp;
}

texture_t _create(tex_builder_t texer) {
    return texer.tex;
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
#endif
