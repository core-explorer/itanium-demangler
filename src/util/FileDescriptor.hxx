#pragma once
#include <compare>
namespace swartalf {

struct FileDescriptor {
	int fd = -1;

	constexpr int get() const {
		return fd;
	}
	constexpr FileDescriptor() = default;
	explicit constexpr FileDescriptor(int f) :
	    fd(f) {}
	constexpr auto operator<=>(const FileDescriptor& other) const = default;
	constexpr auto operator<=>(int other) const {
		return fd <=> other;
	}
	constexpr auto& operator=(int f) {
		fd = f;
		return *this;
	}
	constexpr bool operator==(int f) const {
		return fd == f;
	}
	constexpr bool operator==(const FileDescriptor&) const = default;
};

}