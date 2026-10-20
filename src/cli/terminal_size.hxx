#pragma once
#include <optional>
namespace swartalf {
std::optional<unsigned> get_output_columns_from_ioctl();
std::optional<unsigned> get_output_columns_from_env();
std::optional<unsigned> get_output_columns();
}