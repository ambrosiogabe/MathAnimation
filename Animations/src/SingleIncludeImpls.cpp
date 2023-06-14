#define GABE_CPP_PRINT_IMPL
#include <cppUtils/cppPrint.hpp>
#undef GABE_CPP_PRINT_IMPL

#define GABE_CPP_UTILS_IMPL
#include <cppUtils/cppUtils.hpp>
#undef GABE_CPP_UTILS_IMPL

#define GABE_CPP_TESTS_IMPL
#include <cppUtils/cppTests.hpp>
#undef GABE_CPP_TESTS_IMPL

#define GABE_CPP_STRING_IMPL
#include <cppUtils/cppStrings.hpp>
#undef GABE_CPP_STRING_IMPL

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_write.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>
