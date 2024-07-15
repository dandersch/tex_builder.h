#pragma once
/*
   IDEA: an immediate-mode texture generation api in C that is nice to use

   inspiration:
   https://x.com/phoboslab/status/1432363394418491394/photo/1

   Limitations:
   - scope_... macros cannot be on the same line
*/

/* Possible extensions:
 *
 * combining textures:
 *   stencil/blit(t,x,y): draws tex t to the texture at (x,y)
 *   blend:  blend two textures together with a specified blending mode (e.g., alpha blending, additive blending)
 *
 * drawing operations:
 *   line:     draw a line segment between two points with a color
 *   voronoi:  draw a voronoi pattern with an outline color
 *   rotate:   rotate the texture by a specified angle around a pivot point.
 *   text:     draw string with font at position with size or fit the rect
 *   resize:   resize the texture to a new width and height using interpolation (e.g., bilinear interpolation).
 *   gradient: gradient version of color()
 *
 * scopes:
 *   scope_centered(): draw everything at the center
 *   scope_tex_push_rect(w,h): push a texture of size h,w onto the current texture without
 *                             caring where it ends up (useful for building atlases).
 *                             Maybe make it so that a rect_t can be passed in that gets filled with x,y,h,w.
 *
 * general:
 *   outline version of rect, circle.
 *   NOTE: outline could be implemented in terms of a scope_rect if we had masks
 *   scope_rect(0+thickness, 0+thickness, w-thickness, h-thickness) { color(COLOR); }
 *
 *   all drawing operations should have versions that take float (i.e. percentages) instead of pixels
 *   all drawing operations should take in alpha values into account
 *
 *   if we have consider alpha values, we can have masks inside texer_t to implement e.g. scope_tex_circle() {}
 *
 *   push/pop pragma warning about shadowing variables when nesting scope_tex_build's
 *
 *   make work in glsl shader code
 *   check if it works with C++ and MSVC
 *   generate normal maps along texture for operations that add depth (e.g. insets)
 *
 *   flags(MIRROR|BLEND_ALPHA);
 */


/* GLSL restrictions:
 * - no pointers
 * - no function forward declarations
 * - no '(color_t){1,1,1,1}' initialization, only 'color_t(1,1,1,1)' -> define a ctor() macro
*/

#include <stddef.h>

#ifndef RUN_ON_COMPUTE_SHADER
/* TODO use these */
typedef unsigned int uint;
typedef struct color_t color_t;
typedef struct texture_t texture_t;
#endif

// NOTE RGBA vs BGRA layout could be set with a macro
struct color_t       { float r; float g; float b; float a;        };
struct texture_t     { uint width; uint height; color_t* rgb; };

/* used internally */
enum {
      CLIPPING_SDF_NONE,
      CLIPPING_SDF_CIRCLE,
      CLIPPING_SDF_BOX,
      CLIPPING_SDF_COUNT,
};
typedef struct clipping_sdf_t {
    uint type;
    int x,y,r,w,h;
} clipping_sdf_t;
typedef struct texer_t {
    texture_t tex;

    /* size of the whole texture NOTE: same as tex.{width,height} */
    uint atlas_width;
    uint atlas_height;

    // int flags; // TODO bitfield containing e.g. TEX_BUILD_{FLIP,MIRROR,BLEND_ALPHA}

    uint seed; /* random seed that will be used by noise, voronoi, etc. */

    struct {
        int x, y;
        int w, h;
    } mask; // TODO rename to clipping_region

    clipping_sdf_t sdf;

    /* used in for-loop macros */
    int i;
} texer_t;

/* NOTE: turn into flags that can be set by user */
#define TEXER_FLAG_FLIP 1

/*
 * api
 */
/* building api (allocating) */
texer_t texture(int w, int h);

/* scope api */
#define texer(tex, builder)                     _texer_threaded(tex,builder,0,1)
#define texer_threaded(tex, builder, id, count) _texer_threaded(tex,builder,id,count)
#define texer_rect(x,y,h,w)                     _texer_rect(x,y,h,w)

#define texer_rectcut_top(cut)                  _texer_rectcut_top(cut)
#define texer_rectcut_left(cut)                 _texer_rectcut_left(cut)
#define texer_rectcut_right(cut)                _texer_rectcut_right(cut)
#define texer_rectcut_bottom(cut)               _texer_rectcut_bottom(cut)

/* drawing api */
#define        color(...) temp = _color(temp, pixel_x, pixel_y, __VA_ARGS__)
texer_t _color(texer_t tex, int pixel_x, int pixel_y, color_t color);
#define        seed(nr)   temp.seed = nr
#define        noise(...) temp = _noise(temp, pixel_x, pixel_y, __VA_ARGS__)
texer_t _noise(texer_t  tex, int pixel_x, int pixel_y, float intensity); /* TODO should take a color value */
#define        outline(color,thick) temp = _outline(temp, pixel_x, pixel_y, color, thick); _texer_rect(thick,thick,temp.mask.h-(thick*2),temp.mask.w-(thick*2)) /* TODO why do we need (thick*2) here? */
texer_t _outline(texer_t tex, int pixel_x, int pixel_y, color_t color, uint thickness);
#define        voronoi(...) temp = _voronoi(temp, pixel_x, pixel_y, ##__VA_ARGS__)
texer_t _voronoi(texer_t tex, int pixel_x, int pixel_y, uint seed_points); /* TODO should take a color value */
/* for debugging */
#define        pixel(...) temp = _pixel(temp, ##__VA_ARGS__) // places a pixel at the current {x,y} start
texer_t _pixel(texer_t tex);

/* called by internally by macros */
texer_t _set_mask(texer_t* builder, uint x, uint y, uint width, uint height);

/* helper macros */
#define TOKEN_PASTE(a, b) a##b
#define CONCAT(a,b) TOKEN_PASTE(a,b)
#define UNIQUE_VAR(name) CONCAT(name, __LINE__)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

/* helper functions */
static inline float step(float edge, float x) { return x >= edge ? 1.0f : 0.0f; }
/* TODO convert to function */
#define clip_to_region(mask, px, py) (step(mask.x, px) * step(px, mask.x + mask.w-1) * step(mask.y, py) * step(py, mask.y + mask.h-1))
static inline color_t alpha_blend(color_t src, color_t dst) {
    color_t blend;
    blend.r = CLAMP(src.r * src.a + dst.r * (1.0f - src.a), 0.0f, 1.0f);
    blend.g = CLAMP(src.g * src.a + dst.g * (1.0f - src.a), 0.0f, 1.0f);
    blend.b = CLAMP(src.b * src.a + dst.b * (1.0f - src.a), 0.0f, 1.0f);
    blend.a = 0.0f; // TODO for some reason we need to set it to 0 not see glClearColor...
    return blend;
}
static inline int squared_distance(int x1, int y1, int x2, int y2) { return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2); }
const int  I32_MAX      = 0x7FFFFFFF;
const uint U32_MAX      = 4294967295;
static inline uint _rand(uint index) { index = (index << 13) ^ index; return ((index * (index * index * 15731 + 789221) + 1376312589) & 0x7fffffff); };
static inline uint get_index(texer_t texer, uint pixel_x, uint pixel_y) {
    /* NOTE: a shearing effect can be implemented by doing atlas_width-{1,2,3,...} */
    #ifdef TEXER_FLAG_FLIP
    return (texer.atlas_height - pixel_y - 1) * (texer.atlas_width) + pixel_x;
    #else
    return (pixel_y * texer.atlas_width) + pixel_x;
    #endif
}

static inline float sdf(clipping_sdf_t sdf) {
    float in = 0;
    switch (sdf.type) {
        case CLIPPING_SDF_BOX : {
            //vec2 d = abs(p)-b;
            //in = length(max(d,0.0)) + min(max(d.x,d.y),0.0);
        } break;
        case CLIPPING_SDF_CIRCLE : {
            //in = length(p) - r;
        } break;
    }

    return in;
};

#ifndef RUN_ON_COMPUTE_SHADER
  #define _texer_for_every_pixel(thread_id, thread_count) \
      for (int pixel_x = thread_id; pixel_x < temp.atlas_width; pixel_x += thread_count) \
          for (int pixel_y = 0; pixel_y < temp.atlas_height; pixel_y++)
#else
  /* NOTE: in a compute shader, we only use the for loop to sneak in the values for pixel_x and pixel_y */
  #define _texer_for_every_pixel(thread_id, thread_count) \
      for (int pixel_x = gl_GlobalInvocationID.x, pixel_y = gl_GlobalInvocationID.y, UNIQUE_VAR(i) = 0; UNIQUE_VAR(i) == 0; UNIQUE_VAR(i)++ )
#endif

#define _texer_threaded(tex, builder, thread_id, thread_count)                       \
    for (texer_t temp = builder; temp.i == 0; (temp.i+=1, tex = _create(temp)))      \
        _texer_for_every_pixel(thread_id, thread_count)

#define _texer_rect(x,y,w,h) \
    for (texer_t UNIQUE_VAR(old_builder) = _set_mask(&temp, x,y,w,h); \
         UNIQUE_VAR(old_builder).i == 0;                                    \
         (temp = UNIQUE_VAR(old_builder), UNIQUE_VAR(old_builder).i+=1))

#define _texer_rectcut_top(cut)    _texer_rect(                 0,                  0, temp.mask.w,         cut)
#define _texer_rectcut_left(cut)   _texer_rect(                 0,                  0,         cut, temp.mask.h)
#define _texer_rectcut_right(cut)  _texer_rect((temp.mask.w- cut),                  0,         cut, temp.mask.h)
#define _texer_rectcut_bottom(cut) _texer_rect(                 0, (temp.mask.h- cut), temp.mask.w,         cut)

/* internal */
#ifdef TEXER_IMPLEMENTATION
#include <stdlib.h> // for malloc
#include <assert.h> // TODO take in assert macro from user
texer_t _color(texer_t tex, int pixel_x, int pixel_y, color_t color) {
    float clip = clip_to_region(tex.mask, pixel_x, pixel_y);

    color.r *= clip;
    color.g *= clip;
    color.b *= clip;
    color.a *= clip;

    /* color the subtexture */
    uint index = get_index(tex, pixel_x, pixel_y);
    tex.tex.rgb[index] = alpha_blend(color, tex.tex.rgb[index]);

    return tex;
}
texer_t _noise(texer_t tex, int pixel_x, int pixel_y, float intensity)  {
    float clip = clip_to_region(tex.mask, pixel_x, pixel_y);

    if (!(clip > 0.0f)) { return tex; }; /* NOTE: early out is actually faster on CPUs (still needs testing with shaders)  */

    uint idx = get_index(tex, pixel_x, pixel_y);
    color_t noise;

    /* Add noise to each color component based on intensity */
    // TODO better pseudo random number generation
    noise.r = tex.tex.rgb[idx].r + intensity * ((float)_rand(tex.seed+pixel_x+pixel_y)/(float) U32_MAX - 0.5f);
    noise.g = tex.tex.rgb[idx].g + intensity * ((float)_rand(tex.seed+pixel_x+pixel_y)/(float) U32_MAX - 0.5f);
    noise.b = tex.tex.rgb[idx].b + intensity * ((float)_rand(tex.seed+pixel_x+pixel_y)/(float) U32_MAX - 0.5f);

    /* NOTE: sheared stripes pattern, could be nice to have as a drawing operation */
    //noise.r = tex.tex.rgb[idx].r + intensity * ((float)_rand(tex.seed+pixel_x+pixel_y)/(float) U32_MAX - 0.5f);
    //noise.g = tex.tex.rgb[idx].g + intensity * ((float)_rand(tex.seed+pixel_x+pixel_y)/(float) U32_MAX - 0.5f);
    //noise.b = tex.tex.rgb[idx].b + intensity * ((float)_rand(tex.seed+pixel_x+pixel_y)/(float) U32_MAX - 0.5f);

    /* clamp colors to [0, 1] and clip if needed */
    noise.r = CLAMP(noise.r, 0.0f, 1.0f) * clip;
    noise.g = CLAMP(noise.g, 0.0f, 1.0f) * clip;
    noise.b = CLAMP(noise.b, 0.0f, 1.0f) * clip;
    noise.a = clip;

    tex.tex.rgb[idx] = alpha_blend(noise, tex.tex.rgb[idx]);

    return tex;
}
texer_t _outline(texer_t tex, int pixel_x, int pixel_y, color_t color, uint thickness) {
    float clip = clip_to_region(tex.mask, pixel_x, pixel_y);

    color.r *= clip;
    color.g *= clip;
    color.b *= clip;
    color.a *= clip;

    uint index = get_index(tex, pixel_x, pixel_y);

    /* TODO remove if statements */
    /* top side */
    if (pixel_y < tex.mask.y + thickness) { tex.tex.rgb[index] = alpha_blend(color, tex.tex.rgb[index]); }

    /* bottom side */
    if (pixel_y >= tex.mask.y + tex.mask.h - thickness) { tex.tex.rgb[index] = alpha_blend(color, tex.tex.rgb[index]); }

    /* left side */
    if (pixel_x < tex.mask.x + thickness) { tex.tex.rgb[index] = alpha_blend(color, tex.tex.rgb[index]); }

    /* right side */
    if (pixel_x >= tex.mask.x + tex.mask.w - thickness) { tex.tex.rgb[index] = alpha_blend(color, tex.tex.rgb[index]); }

    return tex;
}
texer_t _voronoi(texer_t tex, int pixel_x, int pixel_y, uint seed_points) {
    float clip = clip_to_region(tex.mask, pixel_x, pixel_y);

    /* determine nearest seed point for current pixel */
    int nearest_seed_index = -1;
    int min_distance       = I32_MAX;
    for (int i = 0; i < seed_points; i++) {
        uint seed_point_x = tex.mask.x + _rand(tex.seed+(i * seed_points)) % tex.mask.w;
        uint seed_point_y = tex.mask.y + _rand(tex.seed+(i+1)*seed_points) % tex.mask.h;
        int distance = squared_distance(pixel_x, pixel_y, seed_point_x, seed_point_y);
        if (distance < min_distance) {
            min_distance = distance;
            nearest_seed_index = i;
        }
    }

    /* generate random color based off seed index */
    color_t color;
    color.r = ((float) _rand(nearest_seed_index + 0)/ (float) U32_MAX) * clip;
    color.g = ((float) _rand(nearest_seed_index + 1)/ (float) U32_MAX) * clip;
    color.b = ((float) _rand(nearest_seed_index + 2)/ (float) U32_MAX) * clip;
    color.a = 1.0f * clip;

    uint index = get_index(tex, pixel_x, pixel_y);
    tex.tex.rgb[index] = alpha_blend(color, tex.tex.rgb[index]);

    return tex;
}

texer_t texture(int w, int h) {
    texer_t texer;

    /* init builder */
    texer.atlas_width  = w;
    texer.atlas_height = h;
    texer.i            = 0;

    /* set mask */
    texer.mask.x = 0;
    texer.mask.y = 0;
    texer.mask.w = w;
    texer.mask.h = h;

    /* init texture */
    texer.tex.width    = w;
    texer.tex.height   = h;
    texer.tex.rgb      = malloc(w * h * sizeof(color_t));

    return texer;
}

/* return copy of old builder, modify current builder's mask */
texer_t _set_mask(texer_t* builder, uint x, uint y, uint width, uint height) {
    texer_t old = *builder;

    builder->mask.x = CLAMP(x + builder->mask.x, builder->mask.x, builder->mask.x + builder->mask.w); // NOTE: makes it so the rect is still visible for x outside of bounds
    builder->mask.y = CLAMP(y + builder->mask.y, builder->mask.y, builder->mask.y + builder->mask.h);

    /* clamp width and height */

    /* first case: rectangle is past clipping region  */
    if (old.mask.x + x + width  > old.mask.x + old.mask.w) { width  -= (old.mask.x + x + width)  - (old.mask.x + old.mask.w); }
    if (old.mask.y + y + height > old.mask.y + old.mask.h) { height -= (old.mask.y + y + height) - (old.mask.y + old.mask.h); }

    /* second case: rectangle is before clipping region  */
    /* NOTE: not yet possible because x/y is unsigned, but we might want to change them to signed ints */
    if (x < 0) { width  = (old.mask.x + old.mask.w) - (old.mask.x + x + width);  }
    if (y < 0) { height = (old.mask.y + old.mask.h) - (old.mask.y + y + height); }

    /* third case: rectangle is inside clipping region  */
    // nothing needs to be done here

    /* TODO we are off by one pixel if we don't do -1 here, find out why & and if this was the case before the rewrite */
    //builder->mask.w = min(width - 1, builder->mask.w);
    //builder->mask.h = min(height- 1, builder->mask.h);
    builder->mask.w = width  ;
    builder->mask.h = height ;

    return old;
}

texer_t _pixel(texer_t tex) {
   uint index = get_index(tex, tex.mask.x, tex.mask.y);
   tex.tex.rgb[index] = (color_t){1,1,1,1};
   return tex;
}

/* NOTE: this is called for every single thread right now */
texture_t _create(texer_t texer) {
    return texer.tex;
}
#endif
