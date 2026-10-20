add_library(swartalf-cli OBJECT
ArgParser.cxx 
cli_color.cxx
cli_color256.cxx
ColorMode.cxx
terminal.cxx
CliDecorator.cxx
terminal_size.cxx
)
add_library(swartalf::cli ALIAS swartalf-cli)
settings(swartalf-cli)

