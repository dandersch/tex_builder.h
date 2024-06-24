#pragma once

#define TEXTURE_COUNT 1 // only one big texture atlas for now

typedef struct state_t
{
    texture_t tex[TEXTURE_COUNT];
    tex_builder_t tex_builder; // used for generating textures
} state_t;
