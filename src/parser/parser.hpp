#pragma once

#include "../command.hpp"
#include <queue>

/*
addrule(HOSTDEF, VARNAME, optional{SPACE}, ":=", optional{SPACE}, ALNUM, "@", )
*/

class Parser {
public:
    /**
     * Returns `true` if the parser is expecting more input to form a complete
     * command. Useful for interactive mode.
     */
    bool isParseIncomplete() const;

    /**
     * Pops a command off the command queue. Returns `nullptr` if there are no
     * commands to pop.
     */
    std::unique_ptr<Command> popCommand();

    /**
     * Parses the given script or piece of script. This will be appended to any
     * incompletely parsed pieces of a script.
     */
    void parse(const std::string& buf);
};

/*
class Parser {
public:
    CommandList parse(const std::string& buf);

private:
    void tokenize(const std::string& buf);

    struct Token {
        std::string str;

        // true if str is an operator, e.g. an actual pipe operator as opposed
        // to just the "|" character to be sent as an argument
        bool isOp;

        int line;
        int col;
    };

    std::queue<Token> tokens;
    struct {
        std::string curTok;
        char curQuote = 0;
        char prev = 0;
        
        Token prevTok;
        bool isCommented = false;

        int line = 1;
        int col = 1;        
    } tokState;

    void pushToken(const Token& tok);
};
*/