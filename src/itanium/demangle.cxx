#include "FiltApp.hxx"
#include "FiltArgs.hxx"
#include "../cli/ArgParser.hxx"
#include "../cli/ColorMode.hxx"
#include "../cli/cli_color.hxx"
#include "../util/print.hxx"

int main(int argc, char** argv) {
	using namespace swartalf;

	init_color_mode();
	FiltApp app;
	app.exe = argv[0];
	app.parser = makeFiltParser();

	auto& parser = app.parser;
	parser.programName = "itanium-demangle";

	auto parseResult = parser.parse(argc, argv);
	if (not parseResult) {
		const auto& error = parseResult.error();

		colored_println(stderr, error.message, "", "error");
		fmt::print(stderr, "\n");
		parser.printUsage(stderr);

		return 1;
	}
	return app.run(parseResult.value());
}
