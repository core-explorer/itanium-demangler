#pragma once
#include "Unsanitized.hxx"
#include <string>
namespace swartalf {
__attribute__((visibility("default"))) std::string escape_ascii(std::string_view s);
inline std::string escape_ascii(swartalf::Unsanitized<std::string_view> s) {
	return escape_ascii(s.get_value());
}
inline std::string escape_ascii(const swartalf::Unsanitized<std::string>& s) {
	return escape_ascii(s.get_value());
}
}