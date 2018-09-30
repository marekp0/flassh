#include "lexer.hpp"
#include "symbols.hpp"
#include <stdexcept>

using namespace Symbols;

void Lexer::input(const std::string& str)
{
    for (char c : str) {
        (this->*nextHandler)(c);

        // advance column and line numbers
        if (c == '\n') {
            ++line;
            col = 1;
        }
        else {
            ++col;
        }
    }
}

Token* Lexer::popToken()
{
    if (tokenQueue.empty())
        return nullptr;

    auto ret = tokenQueue.front();
    tokenQueue.pop_front();
    return ret;
}

bool Lexer::isComplete() const
{
    return curTok == nullptr;
}

void Lexer::pushChar(char c)
{
    if (curTok == nullptr) {
        curTok = new Token;
        curTok->line = line;
        curTok->col = col;
    }
    curTok->str.push_back(c);
}

/**
 * Returns true if `str` matches [A-Za-z_]\w+
 */
static bool isVarname(const std::string& str)
{
    // Using C++ regex would be a bit overkill
    if (str.empty())
        return false;

    // first char must be a letter or underscore
    if (!isalpha(str[0]) && str[0] != '_')
        return false;

    // remaining characters must be alphanumeric or an underscore
    for (char c : str) {
        if (!isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}

void Lexer::pushToken(int symbol)
{
    if (curTok == nullptr)
        return;

    curTok->symbol = symbol;

    // see if we can upgrade from STR to VARNAME
    if (curTok->symbol == STR && !tokenWasEverQuotedOrEscaped && isVarname(curTok->str)) {
        curTok->symbol = VARNAME;
    }

    tokenQueue.push_back(curTok);
    curTok = nullptr;
    tokenWasEverQuotedOrEscaped = false;
}

// Helper functions to make lexer code cleaner

static bool isQuote(char c)
{
    return c == '\'' || c == '\"';
}

static bool isOp(char c)
{
    return (c == ':' ||
            c == ';' ||
            c == '=' ||
            c == '|' ||
            c == '&');
}

/**
 * Returns `true` if the character can be the start of a two-character operator
 */
static bool isOpStart(char c)
{
    return (c == ':' ||
            c == '|' ||
            c == '&');
}

static int oneCharOpToSymbol(char c)
{
    switch (c) {
    case ':':
        return COLON;
    case ';':
        return SEMICOLON;
    case '=':
        return EQUALS;
    case '|':
        return PIPE;
    case '&':
        return AMPERSAND;
    default:
        throw std::invalid_argument("bad arg for oneCharOpToSymbol");
    }
}

/**
 * Handler for the default state, i.e. not commented, quoted, or escaped
 */
void Lexer::handleDefault(char c)
{
    // '#' character starts a comment and ends the token
    if (c == '#') {
        pushToken(STR);
        nextHandler = &Lexer::handleCommented;
    }
    // start of a quote
    else if (isQuote(c)) {
        quote = c;
        nextHandler = &Lexer::handleQuoted;
        tokenWasEverQuotedOrEscaped = true;
    }
    // escaped character
    else if (c == '\\') {
        nextHandler = &Lexer::handleEscaped;
        tokenWasEverQuotedOrEscaped = true;
    }
    // space characters end the current token
    else if (isspace(c)) {
        pushToken(STR);
        pushChar(c);
        if (c == '\n') {
            pushToken(NEWLINE);
        }
        else {
            pushToken(SPACE);
        }
    }
    // operators end the current token
    else if (isOp(c) && !tokenWasEverQuotedOrEscaped) {
        pushToken(STR);
        pushChar(c);

        if (isOpStart(c)) {
            nextHandler = &Lexer::handleOp;
        }
        else {
            pushToken(oneCharOpToSymbol(c));
        }
    }
    // non-special character
    else {
        pushChar(c);
    }
}

/**
 * Previous character might have been the start of a two-char operator
 */
void Lexer::handleOp(char c)
{
    bool handled = false;
    if (curTok->str == ":") {
        if (c == '=') {
            pushChar(c);
            pushToken(COLON_EQ);
            handled = true;
        }
        else if (c == ':') {
            pushChar(c);
            pushToken(COLON2);
            handled = true;
        }
    }
    else if (curTok->str == "|") {
        if (c == '|') {
            pushChar(c);
            pushToken(LOG_OR);
            handled = true;
        }
    }
    else if (curTok->str == "&") {
        if (c == '&') {
            pushChar(c);
            pushToken(LOG_AND);
            handled = true;
        }
    }
    else {
        throw std::logic_error("bad curTok->str in Lexer::handleOp");
    }

    nextHandler = &Lexer::handleDefault;
    if (!handled) {
        pushToken(oneCharOpToSymbol(curTok->str[0]));
        handleDefault(c);
    }
}

/**
 * Currently in a comment
 */
void Lexer::handleCommented(char c)
{
    // end comment on newline
    if (c == '\n') {
        nextHandler = &Lexer::handleDefault;
    }
}

/**
 * Currently inside quotes, character is not escaped
 */
void Lexer::handleQuoted(char c)
{
    // matching quote: leave quoted state
    if (c == quote) {
        nextHandler = &Lexer::handleDefault;
        quote = 0;
    }
    // backslash: go to QuotedEscaped state if inside double quote
    else if (c == '\\' && quote == '\"') {
        nextHandler = &Lexer::handleQuotedEscaped;
    }
    // otherwise: write character as-is
    else {
        pushChar(c);
    }
}

/**
 * Character escaped, not inside quotes
 */
void Lexer::handleEscaped(char c)
{
    // write character as-is, but ignore newline for bash compatibility
    if (c != '\n')
        pushChar(c);
    nextHandler = &Lexer::handleDefault;
}

/**
 * Character escaped inside quotes
 */
void Lexer::handleQuotedEscaped(char c)
{
    // two backslashes --> one backslash
    if (c == '\\') {
        pushChar(c);
    }
    // escaped matching quote --> just the quote
    else if (c == quote) {
        pushChar(c);
    }
    // newline --> do nothing for bash compatibility
    else if (c == '\n') { }
    // escaped any other character --> backslash + char
    else {
        pushChar('\\');
        pushChar(c);
    }
    nextHandler = &Lexer::handleQuoted;
}
