#pragma once

#if __has_include("../../fmt/include/fmt/format.h")
#include "../../fmt/include/fmt/format.h"
#elif defined(FMT_HEADER_ONLY) &&  __has_include("/usr/include/fmt/format.h")
#include "/usr/include/fmt/format.h"
#else
#include <print>
namespace fmt {
    using std::print;
}
#endif
