#include "HtmlFormat.hxx"
#include "../itanium/FiltAnalyzer.hxx"
#include "../itanium/FormatDemangleLimit.hxx"
#include "../itanium/ItaniumDemangler.hxx"
#include "../itanium/ParseName.hxx"
#include "../util/Stream.hxx"
#include "../util/string.hxx"
#include "../util/version.hxx"
#include <atomic>
#include <forward_list>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#include <emscripten.h>
#endif

using namespace swartalf;

struct State {
	std::atomic<size_t> done = 0;
	Stream os;

	std::string autonav;

	bool darkMode = false;
	FiltAnalyzerConfig config;
};

std::unique_ptr<State> ustate;

namespace swartalf {
using namespace swartalf::itanium;

void showGrammar() {
	static ItaniumDemangler demangler;
	auto cat = itanium::ProductionType::any_mangled_name;
	auto& rules = demangler.rules;
	auto g = "<pre>" + format_grammar_recursive_html(rules, cat) + "</pre>";

	auto& escaped = g;
	EM_ASM_({ document.getElementById('grammar').innerHTML = UTF8ToString($0, $1); }, escaped.data(), escaped.size());
}

void setDarkMode(bool value) {
	ustate->darkMode = value;
}

void setMaxLength(int nrows, int ncols) {
	ustate->config.length = nrows * ncols;
}
}

#ifdef __EMSCRIPTEN__

extern "C" {
EMSCRIPTEN_KEEPALIVE
void setDarkMode(int val) {
	swartalf::setDarkMode(val);
}
}

extern "C" {
EMSCRIPTEN_KEEPALIVE
void showGrammar() {
	swartalf::showGrammar();
}
}

extern "C" {
EMSCRIPTEN_KEEPALIVE
void setMaxLength(int nrows, int ncols) {
	swartalf::setMaxLength(nrows, ncols);
}
}
#endif

void main_loop(void* arg) {
	(void)arg;
}

void init_loop(void*) {
#ifdef __EMSCRIPTEN__
	emscripten_cancel_main_loop();

	EM_ASM_({
		Module.showGrammar = Module.cwrap('showGrammar', null, []);
		Module.setMaxLength = Module.cwrap('setMaxLength', null, [ 'number', 'number' ]);
		Module.setDarkMode = Module.cwrap('setDarkMode', null, ['number']);
		is_emscripten_initialized = 1;
		set_darkmode(darkmode);
	});
	swartalf::showGrammar();
	auto version = get_version_string();
	if (not version.empty()) {
		EM_ASM_({ document.getElementById('version').innerHTML = UTF8ToString($0, $1);document.getElementById('version').style.visibility = ""; }, version.data(), version.size());
	}
	auto source = get_source_string();
	if (not source.empty()) {
		EM_ASM_({ document.getElementById('source-link').href = UTF8ToString($0, $1);document.getElementById('source-span').style.visibility = ""; }, source.data(), source.size());
	}
	emscripten_set_main_loop_arg(main_loop, ustate.get(), 0, true);
#endif
}

int main(int, char**) {
	ustate = std::make_unique<State>();
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(init_loop, nullptr, 0, false);
#else
	init_loop(nullptr);
#endif

#ifdef __EMSCRIPTEN__
#else

	float max_fps = 60;
	auto frame_time = std::chrono::nanoseconds(long(int(1.0e9f / max_fps)));
	while (not ustate->done) {
		auto tbegin = std::chrono::steady_clock::now();
		main_loop(ustate.get());
		auto tend = std::chrono::steady_clock::now();
		auto dur = tend - tbegin;
		if (frame_time > dur) {
			std::this_thread::sleep_for(frame_time - dur);
		}
	}
#endif
	return 0;
}
