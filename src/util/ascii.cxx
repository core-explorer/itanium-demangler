#include "ascii.hxx"
#include <cstdint>
namespace swartalf {

std::string escape_ascii(std::string_view s) {
	std::string result;
	for (uint8_t c : s) {
		if (unsigned(' ') <= c and c <= unsigned('~')) {
			result += char(c);
		}
	}
	return result;
}

}