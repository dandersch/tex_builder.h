#pragma once
/*
   IDEA: a texture generation api in C that mimicks the following javascript api:

   texture(0x468f).create(); // Sky
   texture(0x340f).grunge().create(); // Grass
   texture(0x340f).grunge().noise(0x108a, 1.5).create(); // Grass dark
   texture(0x236f).noise(0x1005, 4.5).noise(Ox100a, 2.5).create(); // Water

   inspiration:
   https://x.com/phoboslab/status/1432363394418491394/photo/1
*/

/* USAGE
   texture_t tex = TEXTURE_CREATE(texture(color), noise(2), ....);
*/

/*
  Limitations:
  - color (first argument) cannot be specified as an initializer containing commas, e.g. TEXTURE_CREATE(texture((color_t){0.2,0.3,0.5}), ...)
  - requires GNU C-extension of statement expressions
  - ...
*/

#include <stddef.h>

// NOTE RGBA vs BGRA layout could be set with a macro (?)
typedef struct color_t { float r; float g; float b; float a; } color_t;
typedef struct texture_t { size_t width; size_t height; color_t* rgb; } texture_t;
typedef struct texture_builder_t { texture_t tex; } texture_builder_t;

/* api */
texture_t texture(int w, int h, color_t rgb);
#define noise(...) _noise(&temp, __VA_ARGS__)
void _noise(texture_builder_t* texer, float intensity);
#define grunge(...) _grunge(&temp, __VA_ARGS__)
void _grunge(texture_builder_t* texer, float intensity);
#define smear(...) _smear(&temp, ##__VA_ARGS__)
void _smear(texture_builder_t* texer, color_t rgb);

#define test(...) _test(&temp, ##__VA_ARGS__)
void _test(texture_builder_t* texer, color_t rgb);

#define tex_build(tex, ...) ({texture_builder_t temp = _texture_builder(tex); __VA_ARGS__; _create(temp); })

/* internal */
#ifdef TEX_BUILDER_IMPLEMENTATION
#include <stdlib.h>
void _noise(texture_builder_t* texer, float intensity)  {
    size_t color_count = texer->tex.width * texer->tex.height;
    for (int i = 0; i < color_count; i++) {
        if (i % 2) { texer->tex.rgb[i] = (color_t){1,0.5,1,1}; }
        else       { }
    }
}

void _test(texture_builder_t* texer, color_t rgb) {
    size_t color_count = texer->tex.width * texer->tex.height;
    for (int i = 0; i < color_count; i++) {
          texer->tex.rgb[i % texer->tex.width] = (color_t){0,0,0.5,1};
    }
}

void _grunge(texture_builder_t* texer, float intensity) {
    texer->tex.rgb[0].b += intensity;
}

void _smear(texture_builder_t* texer, color_t rgb) {
    texer->tex.rgb[0] = rgb;
}

texture_t texture(int w, int h, color_t rgba)
{
    texture_t tex;
    tex.width       = w;
    tex.height      = h;
    tex.rgb         = malloc(w * h * sizeof(color_t));
    for (int i = 0; i < (w * h); i++)
    {
        tex.rgb[i] = rgba;
    }
    return tex;
}

texture_builder_t _texture_builder(texture_t tex)
{
    texture_builder_t texer;
    texer.tex = tex;
    return texer;
}
texture_t _create(texture_builder_t texer) { texture_t tex = texer.tex; return tex; }
#endif
