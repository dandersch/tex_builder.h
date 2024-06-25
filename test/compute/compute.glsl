SHADER_VERSION_STRING
#include "common.h"
S(
writeonly uniform image2D output_texture;

uniform float time;

layout (local_size_x = WORK_GROUP_SIZE_X, local_size_y = WORK_GROUP_SIZE_Y, local_size_z = 1) in;
void main() {
    uint  x                    = gl_GlobalInvocationID.x;
    uint  y                    = gl_GlobalInvocationID.y;
    uvec3 work_group_size      = gl_NumWorkGroups;
    uvec3 work_group_id        = gl_WorkGroupID;
    uvec3 local_invocation_id  = gl_LocalInvocationID;

    float zero_to_one = (sin(time) + 1/2);
    float one_to_zero = (cos(time) + 1/2);

    color_t RED   = color_t(one_to_zero,0,0,1);
    color_t GREEN = color_t(0,zero_to_one,0,1);
    color_t BLUE  = color_t(0,0,1,1);

    texture_t atlas;
    scope_tex_build(atlas, texture(TEXTURE_WIDTH, TEXTURE_HEIGHT))
    {
        scope_tex_rect(SUBTEXTURE_WIDTH * int(work_group_id.x), SUBTEXTURE_HEIGHT * int(work_group_id.y), SUBTEXTURE_WIDTH, SUBTEXTURE_HEIGHT)
        {
            color(GREEN);
            outline(BLUE, (int(time)%4));
        }

        switch (work_group_id.x) {
            case 0: {
                switch (work_group_id.y) {
                    case 0: {
                        scope_tex_rect(0,0,16,16)
                        {
                            color(RED);
                        }
                    } break;
                }
            } break;
            case 8: {
                switch (work_group_id.y) {
                    case 8: {
                        scope_tex_rect(16 * 8, 16 * 8,16,16)
                        {
                            color(RED);
                            scope_tex_rect(8, 8,2,2) {
                                color(BLUE);
                            }
                        }
                    } break;
                }
            } break;
        }
    }

    /* NOTE: we don't use the flip() operation because that would be too slow */
    ivec2 tex_size = imageSize(output_texture);
    color_t flipped = rgba[(tex_size.y - y) * tex_size.x + x];
    vec4 color = vec4(flipped.r, flipped.g, flipped.b, flipped.a);
    imageStore(output_texture, ivec2(x, y), color);
}
)
