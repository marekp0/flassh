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
    OPT_SET_HOST,
    COMMAND,
    COMMAND2,
    SIMPLE_COMMAND,
    OPT_CMD_HOST,
    CMD_HOST,
    PIPE_COMMAND,
    DEFINE_HOST,
    HOST_PORT,
    OPT_HOST_PORT,
    ARG,
    ARG_LIST,
    OPT_SPACE,
    MULTI_SPACE,
    MULTI_SPACE_2,
    OPT_SEMICOLON,
    COMMAND_LIST,
    SPACE_OR_NEWLINE,
    OPT_SPACE_OR_NEWLINE,
};

inline bool isTerminal(int symbol)
{
    return symbol < NUM_TERMINAL_SYMBOLS;
}

}   // namespace symbols
