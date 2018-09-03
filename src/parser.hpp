#pragma once

#include "command.hpp"

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
    };

    std::vector<Token> tokens;
    struct {
        std::string curTok;
        char curQuote = 0;
        bool prevWasBackSlash = false;
    } tokState;
};
