#pragma once
#include "DemanglingDialect.hxx"
#include "Grammar.hxx"
#include <expected>
#include <string>
#include <string_view>
#include <vector>
namespace swartalf {

struct AnalysisError {
	std::string message;
	std::string_view path;
	std::string_view severity;
	static inline constexpr std::string_view severity_error = "error";

	AnalysisError(std::string_view msg, std::string_view p, std::string_view sev = severity_error) :
	    message(), path(p), severity(sev) { (void)msg; }
};

struct FiltAnalyzerConfig {
	bool remangle = false;
	bool partial = false;
	bool light_mode = false;
	bool short_names = false;
	bool subs = false;
	bool templ = false;
	bool pack = false;
	bool hide_return = false;
	bool hide_function = false;
	bool debug = false;
	bool check = false;
	bool trailing = false;
	bool expression = false;
	unsigned length = 100000;
	unsigned function_params = 100;
	unsigned template_params = 100;
#ifdef __clang__
	CompilerDialect dialect = CompilerDialect::clang;
#else
	CompilerDialect dialect = CompilerDialect::gcc;
#endif

	itanium::ProductionType category = itanium::ProductionType::any_mangled_name;
};

struct FiltAnalyzer {
	FiltAnalyzerConfig config;
	struct AnalysisResult {
		std::vector<std::string> words;
	};
	std::expected<AnalysisResult, AnalysisError> analyze();
	void stream() const;

	std::vector<std::string_view> words;
};

std::string demangle(std::string_view mangledName);

}
