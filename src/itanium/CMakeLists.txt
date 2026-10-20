add_library(swartalf-demangle-itaniumlib OBJECT
ParseName.cxx
Grammar.cxx
FormatAst.cxx
Parsed.cxx
FormatMangle.cxx
HighlightedText.cxx
DumpGrammar.cxx
FormatDemangle.cxx
DemangleBasicType.cxx
OptimizedGrammar.cxx
FormatDemangleLimit.cxx
FunctionClassification.cxx
)

settings(swartalf-demangle-itaniumlib)
add_library(swartalf::demangle-itanium ALIAS swartalf-demangle-itaniumlib)
target_link_libraries(swartalf-demangle-itaniumlib PUBLIC swartalf::util  swartalf::cli swartalf::deco)



add_library(swartalf-demanglelib STATIC 
ItaniumDemangler.cxx
Base36.cxx
)

settings(swartalf-demanglelib)
add_library(swartalf::demangle ALIAS swartalf-demanglelib)
target_link_libraries(swartalf-demanglelib PUBLIC swartalf::util  swartalf::demangle-itanium swartalf::cli swartalf::deco)



add_library(swartalf-filt OBJECT 
FiltAnalyzer.cxx
FiltArgs.cxx
FiltApp.cxx
)

settings(swartalf-filt)
add_library(swartalf::filt ALIAS swartalf-filt)
target_link_libraries(swartalf-filt PUBLIC swartalf::util swartalf::cli swartalf::demangle )


if (NOT "${EMSCRIPTEN_ENV}" STREQUAL "web")
if(NOT CDX_AFL OR CDX_AFL_BOTH)
add_executable(itanium-demangle  demangle.cxx )
settings(itanium-demangle)
target_link_libraries(itanium-demangle PRIVATE  swartalf::filt swartalf::demangle swartalf::util swartalf::cli swartalf::version)
endif()
endif()

if(CDX_AFL OR CDX_AFL_BOTH)
add_executable(afl-demangle afl-demangle.cxx)
settings(afl-demangle)
target_link_libraries(afl-demangle PRIVATE swartalf::filt swartalf::demangle swartalf::util swartalf::cli)
target_compile_options(afl-demangle PRIVATE -Wno-error)

endif()
