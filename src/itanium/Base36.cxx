
#include "Base36.hxx"
#include <array>

namespace swartalf {

namespace {
constexpr std::array<char, 36> base36_chars = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'};
}

std::string encodeNumberBase36(uint64_t value) {
	std::string result;
	if (value == 0) {
		return "0";
	}
	if (value <= 35) {
		return std::string(1, base36_chars[value]);
	}
	std::string tmp = "0";
	while (value > 0) {
		tmp[0] = base36_chars[value % 36];
		tmp += result;
		result.swap(tmp);
		value /= 36;
	}
	return result;
}

std::string encodeItaniumIndexBase36(uint64_t value) {
	if (value == 0) {
		return "_";
	};
	return encodeNumberBase36(value - 1) + "_";
}

}
