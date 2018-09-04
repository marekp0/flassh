#pragma once

#include "../command.hpp"
#include "lexer.hpp"
#include <queue>
#include <deque>
#include <stack>

class ParseTreeNode;

class Parser {
public:
    /**
     * Returns `true` if the parser was able to parse all input given. This is
     * useful for implementing interactive mode.
     */
    bool isComplete() const;

    /**
     * Pops a command off the command queue. Returns `nullptr` if there are no
     * commands to pop. Ownership of the pointer is transferred to the caller.
     */
    Command* popCommand();

    /**
     * Parses the given script or piece of script. This will be appended to any
     * incompletely parsed pieces of a script. If the parser expects more
     * input, it is not considered an error.
     */
    void parse(const std::string& buf);

private:
    std::queue<Command*> commands;

    Lexer lex;
    std::deque<Token*> tokens;

    bool parseIncomplete = false;

    // commands that are currently being built
    std::stack<Command*> cmdStack;

    size_t traverseRules(const std::vector<int>& decisions);
    void buildCommands(ParseTreeNode* node);

    void deleteTokens(size_t numTokens);
};
