#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#define CHAR_TO_COLOR_TABLE(X) \
    X('#', RED)\
    X(' ', BLUE)\
    X('-', GREEN)\

#define tex_from_string(...) _tex_from_string(__VA_ARGS__+0, (void*)0)

void _tex_from_string(const char* first, ...) {
    const char* ptr;

    size_t buf_size = 0;

    va_list args;
    va_start(args, first);
    for (ptr = first; ptr != NULL ; ptr = va_arg(args,char*)) { buf_size += strlen(ptr); }
    va_end(args);

    va_start(args, first);
    size_t last_width = strlen(first);
    for (ptr = first; ptr != NULL ; ptr = va_arg(args,char*))
    {
        assert(last_width == strlen(ptr)); // texture needs to be rectangular
        last_width = strlen(ptr);
        for (int i = 0; i < strlen(ptr); i++) {
            switch(ptr[i]) {
              #define COLOR_FROM_CHAR(character,color) case (character): { printf("%s ", #color); } break;
              CHAR_TO_COLOR_TABLE(COLOR_FROM_CHAR)
              default: { printf("Char not in colormap\n"); } break;
            }
        }
        printf("\n");
    }
    va_end(args);
}

int main() {
    tex_from_string("####",
                    "#--#",
                    "#--#",
                    "####");
    return 0;
}
