#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>
namespace swartalf {

enum class OptionMultiplicity {
	single,
	multiple
};
using ArgumentMultiplicity = OptionMultiplicity;

enum class ArgumentSkip {
	dontSkip,
	skipArg
};

enum class ArgumentHandling {
	normal,
	disabled
};

struct ExpectedArg {
	std::string_view name;
	std::string_view description;
	ArgumentMultiplicity multiple = ArgumentMultiplicity::single;
	ArgumentSkip skip = ArgumentSkip::dontSkip;
	ArgumentHandling handling = ArgumentHandling::normal;

	ExpectedArg(std::string_view n = "arg", std::string_view d = "") :
	    name(n),
	    description(d) {}

	ExpectedArg(std::string_view n, ArgumentMultiplicity m) :
	    name(n),
	    multiple(m) {}

	ExpectedArg(std::string_view n, ArgumentMultiplicity m, ArgumentHandling h) :
	    name(n),
	    multiple(m), handling(h) {}

	ExpectedArg(std::string_view n, ArgumentHandling h) :
	    name(n),
	    handling(h) {}
};

enum class OptionRequirement {
	optional,
	mandatory

};

enum class OptionValueType {
	none,
	required
};

struct ExpectedOption {
	std::string_view long_name;
	std::string_view short_name;
	std::string_view description;

	OptionValueType argument = OptionValueType::none;
	OptionMultiplicity multiple = OptionMultiplicity::single;
	ArgumentHandling handling = ArgumentHandling::normal;
	OptionRequirement required = OptionRequirement::optional;

	void set(OptionValueType arg) {
		argument = arg;
	}

	void set(OptionMultiplicity arg) {
		multiple = arg;
	}
	void set(ArgumentHandling arg) {
		handling = arg;
	}
	void set(OptionRequirement arg) {
		required = arg;
	}

	template <typename... Args>
	ExpectedOption(std::string_view l, std::string_view s, std::string_view d, Args&&... args) :
	    long_name(l),
	    short_name(s),
	    description(d) {
		(set(args), ...);
	}
};

struct ParsedOption {
	std::string_view long_name;
	std::string_view value;
	constexpr ParsedOption() noexcept = default;
	constexpr ParsedOption(std::string_view v) noexcept :
	    value(v) {}
};

struct ParsedArg {
	std::string_view value;
	constexpr ParsedArg() noexcept = default;
	constexpr ParsedArg(std::string_view v) noexcept :
	    value(v) {}
};
struct ParsingError {
	std::string message;
};
struct ParsingResult {
	std::vector<ParsedArg> args;
	std::vector<ParsedOption> options;
};

struct ArgParser {
	std::expected<ParsingResult, ParsingError> parse(int argc, char** argv);

	std::vector<ExpectedArg> expectedArgs;
	std::vector<ExpectedOption> expectedOptions;
	bool allowUnknownOptions = false;

	template <typename... Args>
	void option(Args&&... args) {
		expectedOptions.emplace_back(std::forward<Args>(args)...);
	}

	template <typename... Args>
	void argument(Args&&... args) {
		expectedArgs.emplace_back(std::forward<Args>(args)...);
	}

	std::string get_usage() const;
	std::string getUsage() const {
		return get_usage();
	}
	void print_usage(FILE* stream) const;
	void printUsage(FILE* stream) const {
		print_usage(stream);
	}
	std::string_view programName;
};
}