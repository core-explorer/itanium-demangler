#include "version.hxx"

namespace swartalf {
std::string_view global_version_string = "0.0.30";
std::string_view get_version_string() {
	return global_version_string;
}
std::string_view global_source_string = "https://github.com/core-explorer/itanium-demangler/";
std::string_view get_source_string() {
	return global_source_string;
}
}
