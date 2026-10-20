#include "FiltAnalyzer.hxx"
#include "../cli/ArgParser.hxx"
#include "../cli/ColorMode.hxx"
#include "../cli/cli_color.hxx"
#include "../fuzz/afl.hxx"
#include "../util/print.hxx"
#include <unistd.h>
#include <cstdio>
#include <span>

AFL_FUZZ_INIT();

using namespace swartalf;

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	FiltAnalyzer analyzer;

#ifdef __AFL_HAVE_MANUAL_CONTROL
	__AFL_INIT();
#endif

	const char* buf = (const char*)AFL_FUZZ_TESTCASE_BUF;

	while (AFL_LOOP(10000)) {
		size_t len = AFL_FUZZ_TESTCASE_LEN;

		if (len < 3) {
			continue;
		}

		auto data = std::span<const char>(buf, len);
		auto n = strnlen(data.data(), data.size());
		auto word = std::string_view(data.data(), n);
		while (not word.empty() and word.back() == '\n') {
			word = word.substr(0, word.size() - 1);
		}
		if (word.size() < 3) {
			continue;
		}
		bool skip = false;
		for (auto c : word) {
			if ((uint8_t(c) < uint8_t('$')) or uint8_t(c) > uint8_t('z')) {
				skip = true;
				break;
			}
		}

		if (skip) {
			continue;
		}

		analyzer.words.emplace_back(word);
		auto success = analyzer.analyze();
		(void)success;
	}

	return 0;
}
