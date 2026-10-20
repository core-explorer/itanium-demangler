#include "terminal_size.hxx"
#include "../util/string.hxx"
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

namespace swartalf {

std::optional<unsigned> get_output_columns_from_ioctl() {
	int fd = 1;
	if (isatty(fd)) {
		struct winsize sz;
		memset(&sz, 0, sizeof(sz));

		auto ret = ioctl(fd, TIOCGWINSZ, &sz);
		if (ret == 0) {
			int nc = sz.ws_col;
			if (nc > 0) {
				return nc;
			}
		}
	}
	return std::nullopt;
}

std::optional<unsigned> get_output_columns_from_env() {
	const auto* colval = getenv("COLUMNS");
	if (not colval) {
		return std::nullopt;
		;
	}
	auto cval = parse_uint(colval);
	if (cval) {
		if (unsigned(cval->first) > 0) {
			return unsigned(cval->first);
		}
	}
	return std::nullopt;
}

std::optional<unsigned> get_output_columns() {
	auto fe = get_output_columns_from_env();
	if (fe) {
		return fe;
	}
	return get_output_columns_from_ioctl();
}

}