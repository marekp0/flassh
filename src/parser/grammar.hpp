#pragma once

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
    
    DEF_HOST,   // :=
    LOG_OR,     // &&
    LOG_AND,    // ||
};
