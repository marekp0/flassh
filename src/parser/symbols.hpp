#pragma once

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
    COMMAND_LIST,
    COMMAND,
    SIMPLE_COMMAND,
    CMD_HOST,
    PIPE_COMMAND,
    DEFINE_HOST,
    HOST_PORT,
    ARG,
    ARG_LIST,
    SPACE_OR_NEWLINE,
};

inline bool isTerminal(int symbol)
{
    return symbol < NUM_TERMINAL_SYMBOLS && symbol >= 0;
}

}   // namespace symbols
