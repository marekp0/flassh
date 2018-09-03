#include "parser.hpp"
/*
CommandList Parser::parse(const std::string& buf)
{
    tokenize(buf);

    CommandList ret;
    
    // for now, assume only simple commands, and that commands are always complete
    std::vector<std::string> args;
    while (!tokens.empty()) {
        auto tok = tokens.front();
        tokens.pop();

        if (tok.isOp && !args.empty()) {
            ret.push_back(std::unique_ptr<Command>(new SimpleCommand(args)));
            args.clear();
        }
        else {
            args.push_back(tok.str);
        }
    }
    if (!args.empty()) {
        fprintf(stderr, "incomplete command\n");
    }

    return ret;
}
*/
/**
 * Returns true if c is a quote character
 */
static bool isQuote(char c)
{
    return c == '\'' || c == '\"';
}

/**
 * Returns true if c is a command ending character
 */
static bool isEnd(char c)
{
    return c == '\n' || c == ';';
}

/**
 * Returns true if c typically separates arguments or commands
 */
static bool isSep(char c)
{
    return isspace(c) || isEnd(c);
}

/**
 * Returns true if the token following the given token would start a new command
 */
static bool tokStartsNewCmd(const std::string& tok)
{
    return (tok == ";"  ||
            tok == "|"  ||
            tok == "||" ||
            tok == "&&" ||
            tok == "");     // empty string --> first token
}
/*
void Parser::tokenize(const std::string& buf)
{
    for (auto c : buf) {
        bool verbatim = tokState.prev == '\\' || tokState.curQuote != 0;

        // figure out if current token is the first argument of a command
        auto& prevTok = tokState.prevTok;
        bool curTokIsCmd = prevTok.isOp && tokStartsNewCmd(prevTok.str);

        // check start or end of quotes
        if (isQuote(c)) {
            if (tokState.prev == '\\') {
                if (tokState.curQuote == 0 || tokState.curQuote == c) {
                    // not in quote or matching quote, write " or '
                    tokState.curTok.push_back(c);
                }
                else {
                    // different quote type, write \" or \'
                    tokState.curTok.push_back('\\');
                    tokState.curTok.push_back(c);
                }
            }
            else {
                if (tokState.curQuote == 0) {
                    // start quote
                    tokState.curQuote = c;
                }
                else if (tokState.curQuote == c) {
                    // matching end quote
                    tokState.curQuote = 0;
                }
                else {
                    // non-matching quote
                    tokState.curTok.push_back(c);
                }
            }
        }
        // TODO: these conditionals could probably be expressed more simply
        else if (isSep(c)) {
            if (verbatim) {
                // escaped or quoted space
                tokState.curTok.push_back(c);
            }
            else {
                if (!tokState.curTok.empty()) {
                    // end of arg or command
                    pushToken({ tokState.curTok, false, tokState.line, tokState.col });
                }

                if (isEnd(c)) {
                    // \n and ; are the same thing
                    pushToken({ ";", true, tokState.line, tokState.col });
                }
            }
        }
        else if (verbatim || c != '\\') {
            // not a special character
            tokState.curTok.push_back(c);
        }

        tokState.prev = c;
        if (verbatim && c == '\\') {
            // hack to handle multiple backslashes in a row
            tokState.prev = 0;
        }

        // advance line or column number
        if (c == '\n') {
            ++tokState.line;
            tokState.col = 1;
        }
        else {
            ++tokState.col;
        }
    }
}

void Parser::pushToken(const Token& tok)
{
    tokens.push(tok);
    tokState.prevTok = tok;
    tokState.curTok.clear();
}
*/