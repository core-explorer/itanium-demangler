#include "terminal.hxx"
#include "../util/FileDescriptor.hxx"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
namespace swartalf {
using namespace swartalf;

std::mutex terminal_mutex;
swartalf::FileDescriptor terminal_fd;

std::optional<std::pair<int, int>> get_cursor_position_impl() {
	write(terminal_fd.fd, "\x1b[6n", 4);

	char c;
	std::string row_str;
	std::string col_str;
	ssize_t n = -1;

	while (true) {
		n = read(terminal_fd.fd, &c, 1);
		if (n == -1) {
			int err = errno;
			if (err == EAGAIN) {
				usleep(1000);
				continue;
			}
			return std::nullopt;
		} else {
			break;
		}
	}
	if (n != 1 or c != '\x1b') {
		return std::nullopt;
	}
	n = read(terminal_fd.fd, &c, 1);
	if (n != 1 or c != '[') {
		return std::nullopt;
	}

	n = read(terminal_fd.fd, &c, 1);
	if (n != 1 or !isdigit(c)) {
		return std::nullopt;
	}
	row_str += c;
	constexpr size_t max_len = 4;

	do {
		n = read(terminal_fd.fd, &c, 1);
		if (n != 1) {
			return std::nullopt;
		}
		if (c == ';') {
			break;
		}
		if (not isdigit(c)) {
			break;
		}
		row_str += c;
		if (row_str.size() > max_len) {
			return std::nullopt;
		}

	} while (true);

	n = read(terminal_fd.fd, &c, 1);
	if (n != 1 or !isdigit(c)) {
		return std::nullopt;
	}
	col_str += c;

	do {
		n = read(terminal_fd.fd, &c, 1);
		if (n != 1) {
			return std::nullopt;
		}
		if (c == 'R') {
			break;
		}
		if (not isdigit(c)) {
			break;
		}
		col_str += c;
		if (col_str.size() > max_len) {
			return std::nullopt;
		}

	} while (true);

	int row_value = 0;
	auto [rptr, rec] = std::from_chars(row_str.data(), row_str.data() + row_str.size(), row_value);
	(void)rec;
	int col_value = 0;
	auto [cptr, cec] = std::from_chars(col_str.data(), col_str.data() + col_str.size(), col_value);
	(void)cec;
	return std::make_pair(row_value, col_value);
}

std::optional<std::pair<int, int>> get_cursor_position() {
	auto lk = std::lock_guard(terminal_mutex);
	if (terminal_fd.fd == -1) {
		terminal_fd.fd = open("/dev/tty", O_RDWR);
	}
	if (terminal_fd.fd == -1) {
		return std::nullopt;
	}
	struct termios old_state;


	tcgetattr(terminal_fd.fd, &old_state);
	auto new_state = old_state;
	new_state.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(terminal_fd.fd, TCSANOW, &new_state);

	auto val = get_cursor_position_impl();

	tcsetattr(terminal_fd.fd, TCSANOW, &old_state);

	return val;
}

}
