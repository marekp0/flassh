#pragma once

#include <string>
#include <vector>
#include <deque>

struct Token {
    int line;
    int col;

    int symbol;
    std::string str;
};

class Lexer {
public:
    /**
     * Inputs data into the lexer. State is saved between calls to this method.
     * 
     * @param buf  String to be inputted
     */
    void input(const std::string& buf);

    /**
     * Pops a token from the token queue. Ownership of the pointer is
     * transferred to the caller.
     */
    Token* popToken();

    /**
     * Returns true if the lexer is not in the middle of parsing a token.
     */
    bool isComplete() const;

private:
    int line = 1;
    int col = 1;

    Token* curTok = nullptr;
    std::deque<Token*> tokenQueue;

    /**
     * Pushes a character to the current token
     */
    void pushChar(char c);

    /**
     * Pushes the current token to the token queue
     */
    void pushToken(int symbol);

    // state machine / finite automaton-ish thing
    typedef void(Lexer::*CharHandler)(char c);
    CharHandler nextHandler = &Lexer::handleDefault;

    void handleDefault(char c);
    void handleOp(char c);
    void handleCommented(char c);
    void handleQuoted(char c);
    void handleEscaped(char c);
    void handleQuotedEscaped(char c);

    // these get rid of a few state permutations
    char quote = 0;
    bool tokenWasEverQuotedOrEscaped;
};
