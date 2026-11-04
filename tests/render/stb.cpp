// The one translation unit that compiles stb's implementations.
// NOLINTBEGIN(cppcoreguidelines-macro-usage): stb is configured through macros.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STB_IMAGE_WRITE_IMPLEMENTATION
// NOLINTEND(cppcoreguidelines-macro-usage)
#include <stb_image.h>
#include <stb_image_write.h>
