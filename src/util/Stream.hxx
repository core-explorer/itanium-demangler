#pragma once
#include <format>
#include <string>
#include <string_view>
namespace fmt {
using namespace std;
}
namespace swartalf {
template <typename CharType>
struct BasicStream {
	std::basic_string<CharType> buffer;

	std::string& str() {
		return buffer;
	}

	const std::string& str() const {
		return buffer;
	}

	void clear() {
		buffer.clear();
	}
};

template <typename CharType>
BasicStream<CharType>& operator<<(BasicStream<CharType>& stream, std::basic_string_view<CharType> str) {
	stream.buffer += str;
	return stream;
}

template <typename CharType>
BasicStream<CharType>& operator<<(BasicStream<CharType>& stream, CharType str) {
	stream.buffer += str;
	return stream;
}

template <typename CharType, typename T>
BasicStream<CharType>& operator<<(BasicStream<CharType>& stream, const T& value) {
	stream.buffer += fmt::format("{}", value);
	return stream;
}

using Stream = BasicStream<char>;
}
