#include "parser.hpp"

CommandList Parser::parse(const std::string& buf)
{
    tokenize(buf);

    CommandList ret;
    
    // for now, assume only simple commands, and that commands are always complete
    std::vector<std::string> args;
    for (auto& tok : tokens) {
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

    tokens.clear();

    return ret;
}

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

void Parser::tokenize(const std::string& buf)
{
    for (auto c : buf) {
        // check start or end of quotes
        if (isQuote(c)) {
            if (tokState.prevWasBackSlash) {
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
        else if (isSep(c)) {
            if (tokState.prevWasBackSlash || tokState.curQuote != 0) {
                // escaped or quoted space
                tokState.curTok.push_back(c);
            }
            else {
                if (!tokState.curTok.empty()) {
                    // end of arg or command
                    tokens.push_back({ tokState.curTok, false });
                    tokState.curTok.clear();
                }

                if (isEnd(c)) {
                    // \n and ; are the same thing
                    tokens.push_back({ ";", true });
                }
            }
        }
        else {
            // not a special character
            tokState.curTok.push_back(c);
        }

        tokState.prevWasBackSlash = c == '\\';
    }
}
