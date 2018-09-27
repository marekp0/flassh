#pragma once
#include <map>
#include <vector>

namespace Symbols {

enum Terminals {
    VARNAME,    // must match the regex [A-Za-z_]\w* and have no quotes or escapes
    STR,        // arbitrary string of characters that isn't a VARNAME

    SPACE,      // whitespace, excluding \n
    NEWLINE,    // \n

    COLON,      // :
    SEMICOLON,  // ;
    EQUALS,     // =
    PIPE,       // |
    AMPERSAND,  // &
    
    COLON2,     // ::
    COLON_EQ,   // :=
    LOG_OR,     // &&
    LOG_AND,    // ||

    NUM_TERMINAL_SYMBOLS
};

enum NonTerminals {
    SCRIPT = NUM_TERMINAL_SYMBOLS,
    FULL_COMMAND,
    SET_HOST,
    OPT_SET_HOST,
    COMMAND,
    COMMAND2,
    SIMPLE_COMMAND,
    PIPE_COMMAND,
    DEFINE_HOST,
    HOST_PORT,
    OPT_HOST_PORT,
    ARG,
    ARG_LIST,
    OPT_SPACE,
};

inline bool isTerminal(int symbol)
{
    return symbol < NUM_TERMINAL_SYMBOLS;
}

}   // namespace symbols
