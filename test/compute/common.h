/*
 * Contains defines that are common to both shaders and c-code.
 *
 * Also contains centralized struct defs that are shared between glsl and C.
 *
 * NOTE: Can also contain e.g. helper functions that are shared between shaders.
 */
#define WINDOW_TITLE     "compute raytracer"
#define WINDOW_WIDTH     512
#define WINDOW_HEIGHT    512
#define TEXTURE_WIDTH    512
#define TEXTURE_HEIGHT   512
#define TEXTURE_SIZE     (TEXTURE_WIDTH * TEXTURE_HEIGHT)

#define SUBTEXTURE_COUNT 16
// NOTE: only square subtextures; make sure its divisible
#define SUBTEXTURE_WIDTH   (TEXTURE_WIDTH  / SUBTEXTURE_COUNT)
#define SUBTEXTURE_HEIGHT  (TEXTURE_HEIGHT / SUBTEXTURE_COUNT)

/* NOTE no compile-time constant expressions before glsl 4.4 ... */
#define WORK_GROUP_SIZE_X 32 // == (TEXTURE_WIDTH  / SUBTEXTURE_COUNT)
#define WORK_GROUP_SIZE_Y 32 // == (TEXUTRE_HEIGHT / SUBTEXTURE_COUNT)

/* used for lack of enums in glsl */
#define PRIMITIVE_TYPE_NONE      0
#define PRIMITIVE_TYPE_TRIANGLE  1
#define PRIMITIVE_TYPE_SPHERE    2
#define PRIMITIVE_TYPE_COUNT     3

/* common structs */
/* NOTE: we need to comply with std430 layout in C code. For now, specifiying padding
 * manually (always aligned to the size of a vec4) seems to work. We could also make use of
 * _Pragma ("pack(push,n)") _Pragma ("pack(pop)") when expanding the macro on the C-side.
 */
T(color_t,       { float r;  float g; float b; float a;         })
T(texture_t,     { uint width; uint height; /* color_t* rgb; */ }) // NOTE: no pointers in glsl

T(tex_builder_t, { texture_t tex;  uint atlas_width; uint atlas_height; int x_start; int y_start; int width; int height; int i;} )

/* scope api */
#define scope_tex_build(tex, builder) _scope_tex_build(tex, builder)
#define scope_tex_rect(x,y,h,w)       _scope_tex_rect(x,y,h,w)

/* helper macros */
#define TOKEN_PASTE(a, b) a##b
#define CONCAT(a,b) TOKEN_PASTE(a,b)
#define UNIQUE_VAR(name) CONCAT(name, __LINE__)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

#define _scope_tex_build(tex, builder) \
    for (tex_builder_t temp = builder; temp.i == 0; (temp.i+=1, atlas = _create(temp)))
// TODO do x,y,w,h
#define _scope_tex_rect(x,y,h,w) \
    for (int UNIQUE_VAR(old_x_start) = temp.x_start, \
             UNIQUE_VAR(old_y_start) = temp.y_start, \
             UNIQUE_VAR(old_width)   = temp.width,   \
             UNIQUE_VAR(old_height)  = temp.height,  \
             UNIQUE_VAR(j)           = (temp = _rect(temp,                        \
                                                      CLAMP(x + UNIQUE_VAR(old_x_start), UNIQUE_VAR(old_x_start), UNIQUE_VAR(old_x_start) + temp.width), \
                                                      min(y + UNIQUE_VAR(old_y_start), UNIQUE_VAR(old_y_start) + temp.height), \
                                                      (((x + UNIQUE_VAR(old_x_start) + w)  > (UNIQUE_VAR(old_x_start) + temp.width))  ? (w - ((x + UNIQUE_VAR(old_x_start) + w)  - (UNIQUE_VAR(old_x_start) + temp.width))) \
                                                                                                                                      : (x + UNIQUE_VAR(old_x_start) < UNIQUE_VAR(old_x_start)) ? ((x + UNIQUE_VAR(old_x_start) + w) - UNIQUE_VAR(old_x_start)) : w), \
                                                      (((y + UNIQUE_VAR(old_y_start) + h)  > (UNIQUE_VAR(old_y_start) + temp.height)) ? (h - ((y + UNIQUE_VAR(old_y_start) + h)  - (UNIQUE_VAR(old_y_start) + temp.height))) \
                                                                                                                                      : (y + UNIQUE_VAR(old_y_start) < UNIQUE_VAR(old_y_start)) ? ((y + UNIQUE_VAR(old_y_start) + h) - UNIQUE_VAR(old_y_start)) : h) \
                                                      ), 0);                       \
         UNIQUE_VAR(j) == 0; \
         (UNIQUE_VAR(j)+=1,                       \
          temp.x_start = UNIQUE_VAR(old_x_start), \
          temp.y_start = UNIQUE_VAR(old_y_start), \
          temp.width   = UNIQUE_VAR(old_width),   \
          temp.height  = UNIQUE_VAR(old_height)))

SHADER_CODE(
/* shader storage buffer objects */
layout(std430, binding = 0) buffer tex_buf { color_t rgba[]; };

tex_builder_t texture(int w, int h) {
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
    //texer.tex.rgb      = malloc(w * h * sizeof(color_t));

    return texer;
}

texture_t _create(tex_builder_t texer) {
    return texer.tex;
}

tex_builder_t _rect(tex_builder_t texer, int x, int y, int width, int height) {
    tex_builder_t temp = texer;
    temp.x_start = x;
    temp.y_start = y;
    temp.width   = width;
    temp.height  = height;
    return temp;
}

#define        color(...) temp = _color(temp, __VA_ARGS__)
tex_builder_t _color(tex_builder_t tex, color_t color) {
#if 0
    /* color the subtexture */
    for (uint y = tex.y_start; y < (tex.y_start + tex.height); y++) {
        for (uint x = tex.x_start; x < (tex.x_start + tex.width); x++) {
            uint index = (y * tex.atlas_width) + x;
            rgba[index] = color;
        }
    }
#else
    uint x_i = gl_GlobalInvocationID.x;
    uint y_i = gl_GlobalInvocationID.y;
    uint x_l = gl_LocalInvocationID.x;
    uint y_l = gl_LocalInvocationID.y;
    uint y   = tex.y_start + x_l;
    uint x   = tex.x_start + y_l;

    /* color the subtexture */
    uint index = (y_i * tex.atlas_width) + x_i;
    if ((y < (tex.y_start + tex.height)) &&
        (x < (tex.x_start + tex.width))) {
        rgba[index] = color;
    }
#endif


    return tex;
}

#define        outline(...) temp = _outline(temp, __VA_ARGS__)
tex_builder_t _outline(tex_builder_t tex, color_t color, uint thickness) {
#if 0
    int x, y;

    // Iterate over the boundary of the subtexture
    for (x = tex.x_start; x < tex.x_start + tex.width; x++) {
        for (y = tex.y_start; y < tex.y_start + thickness; y++) {
            // Top boundary
            tex.tex.rgb[y * tex.tex.width + x] = color;
        }
        for (y = tex.y_start + tex.height - thickness; y < tex.y_start + tex.height; y++) {
            // Bottom boundary
            tex.tex.rgb[y * tex.tex.width + x] = color;
        }
    }

    for (y = tex.y_start + thickness; y < tex.y_start + tex.height - thickness; y++) {
        for (x = tex.x_start; x < tex.x_start + thickness; x++) {
            // Left boundary
            tex.tex.rgb[y * tex.tex.width + x] = color;
        }
        for (x = tex.x_start + tex.width - thickness; x < tex.x_start + tex.width; x++) {
            // Right boundary
            tex.tex.rgb[y * tex.tex.width + x] = color;
        }
    }
#else
    uint x_i = gl_GlobalInvocationID.x;
    uint y_i = gl_GlobalInvocationID.y;
    uint x_l = gl_LocalInvocationID.x;
    uint y_l = gl_LocalInvocationID.y;
    uint x   = tex.x_start + y_l;
    uint y   = tex.y_start + x_l;
    /* color the subtexture */
    uint index = (y_i * tex.atlas_width) + x_i;

    /* left side */
    if (y < (tex.y_start + thickness)) { rgba[index] = color; }

    /* top side */
    if (x < (tex.x_start + thickness)) { rgba[index] = color; }

    /* right side */
    if (y > (tex.y_start + tex.width - thickness)) { rgba[index] = color; }

    ///* bottom side */
    if (x > (tex.x_start + tex.height - thickness)) { rgba[index] = color; }
#endif

    return tex;
}
)
