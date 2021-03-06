#include "parser.hpp"
#include "util.hpp"
#include "symbols.hpp"
#include <stack>

using namespace Symbols;
using namespace std::placeholders;

/**
 * Helps us initialize the grammar when the process starts
 */
class FlasshGrammar : public ContextFreeGrammar {
public:
    FlasshGrammar();
} flasshGrammar;

/**
 * flassh grammar definition
 */
FlasshGrammar::FlasshGrammar()
{
    // TODO: automatic conversion of left-recursive rules?
    startSymbol = SCRIPT;

    addRule(SCRIPT, {{ ge0({ ge0(SPACE), FULL_COMMAND, ge0(SPACE), NEWLINE }) }});

    addRule(FULL_COMMAND, {
        { opt(SET_HOST), COMMAND_LIST },
        { DEFINE_HOST },
        {} });

    addRule(SET_HOST, {{ VARNAME, ge0(SPACE), COLON, ge0(SPACE) }});

    addRule(COMMAND_LIST, {
        { COMMAND, ge0(SPACE), opt(SEMICOLON) },
        { COMMAND, ge0(SPACE), SEMICOLON, ge0(SPACE), COMMAND_LIST }});

    addRule(COMMAND, {
        { SIMPLE_COMMAND },
        { PIPE_COMMAND }});

    addRule(PIPE_COMMAND, {{ SIMPLE_COMMAND, ge0(SPACE), PIPE, ge0(SPACE_OR_NEWLINE), COMMAND }});
    addRule(SIMPLE_COMMAND, {{ opt(CMD_HOST), ARG_LIST }});
    addRule(CMD_HOST, {{ opt({ VARNAME, ge0(SPACE) }), COLON2, ge0(SPACE) }});
    addRule(DEFINE_HOST, {{ VARNAME, ge0(SPACE), COLON_EQ, ge0(SPACE), ARG, opt(HOST_PORT) }});
    addRule(HOST_PORT, {{ COLON, ARG }});

    addRule(ARG_LIST, {
        { ARG },
        { ARG, ge1(SPACE), ARG_LIST }});

    addRule(ARG, {
        { VARNAME },
        { STR }});

    addRule(SPACE_OR_NEWLINE, {
        { SPACE },
        { NEWLINE }});
}



/**
 * The state of a deterministic PDA at some point in time. Also includes the
 * production rule that is to be chosen.
 */
struct PdaState {
    // index of the production rule chosen at this point
    unsigned int ruleChosen = 0;
    // the PDA stack before substituting the chosen rule
    SymbolStack symStack;
    // the index of the next token to be read
    size_t nextToken;
};

/**
 * Stack used to perform depth-first search over the possible expansions
 * of the production rules
 */
typedef std::stack<PdaState> DecisionStack;



bool Parser::isComplete() const
{
    return lex.isComplete() && tokens.empty();
}

Command* Parser::popCommand()
{
    if (commands.empty())
        return nullptr;

    auto ret = commands.front();
    commands.pop();
    return ret;
}

void Parser::parse(const std::string& buf)
{
    lex.input(buf);

    // if lexer got incomplete input, wait until we get more input
    if (!lex.isComplete())
        return;

    // get tokens from the lexer
    for (Token* tok = lex.popToken(); tok != nullptr; tok = lex.popToken()) {
        tokens.push_back(tok);
    }

    // index of the first token that could not be matched
    size_t maxUnmatchedToken = 0;

    // initial state
    PdaState initialState;
    initialState.ruleChosen = 0;
    initialState.symStack.push(flasshGrammar.startSymbol);
    initialState.nextToken = 0;

    // push initial state onto decision stack
    DecisionStack dstk;
    dstk.push(initialState);

    while (!dstk.empty() && !dstk.top().symStack.empty()) {
        // get the top PDA state, and make a copy
        auto state = dstk.top();
        auto& symStack = state.symStack;

        // find the production rule corresponding to the decision made
        // if we ran out of rules, then we made a wrong decision higher up
        int currentSymbol = state.symStack.top();
        auto& rules = flasshGrammar.rules[currentSymbol];
        if (state.ruleChosen >= rules.size()) {
            dstk.pop();

            // try the next rule on the previous decision
            if (!dstk.empty())
                ++dstk.top().ruleChosen;
            continue;
        }
        auto rule = flasshGrammar.rules[currentSymbol][state.ruleChosen];

        // make the substitution by popping the non-terminal and pushing the
        // replacement symbols in right-to-left order
        symStack.pop();
        for (size_t i = 0; i < rule.replacement.size(); i++) {
            symStack.push(rule.replacement[rule.replacement.size() - i - 1]);
        }

        // try to match terminal symbols
        while (!symStack.empty() && isTerminal(symStack.top())) {
            // check if there are any more tokens to read
            if (tokens.size() <= state.nextToken)
                break;
 
            // check if the next token matches
            if (tokens[state.nextToken]->symbol != symStack.top())
                break;

            // it matches, advance nextToken and pop off the PDA stack
            ++state.nextToken;
            symStack.pop();

            // update maxUnmatchedToken
            if (state.nextToken > maxUnmatchedToken) {
                maxUnmatchedToken = state.nextToken;
            }
        }

        // we made a wrong decision if the top of the stack is a terminal symbol, or
        // if the stack is empty but we haven't read all input yet
        if ((!symStack.empty() && isTerminal(symStack.top()) ||
            (symStack.empty() && state.nextToken < tokens.size())))
        {
            // try the next rule next time
            ++dstk.top().ruleChosen;
            continue;
        }

        // check if there's a new non-terminal symbol
        if (!symStack.empty() && !isTerminal(symStack.top())) {
            // try the new non-terminal symbol's rules from the beginning in the
            // next iteration
            state.ruleChosen = 0;
            dstk.push(state);
        }
        
        if (symStack.empty()) {
            // we're done!
            break;
        }
    }

    if (dstk.empty()) {
        // parsing failed, ran out of possibilities to try

        if (maxUnmatchedToken == tokens.size()) {
            // unexpected EOF, wait for more input
        }
        else if (maxUnmatchedToken < tokens.size()) {
            // TODO: line number?
            fprintf(stderr, "Syntax error: unexpected token %s\n", tokens[maxUnmatchedToken]->str.c_str());
            deleteTokens(tokens.size());
        }
        else {
            // should never happen
            throw std::runtime_error("parser error");
        }
        
        return;
    }

    // get a list of just the indices of the rules chosen
    std::vector<int> decisionList(dstk.size());
    while (!dstk.empty()) {
        decisionList[dstk.size() - 1] = dstk.top().ruleChosen;
        dstk.pop();
    }

    auto parseTree = ParseTreeNode::createParseTree(flasshGrammar, decisionList, tokens);

    // build commands
    parseTree->traverse(std::bind(&Parser::enter, this, _1), std::bind(&Parser::leave, this, _1));

    // FIXME
    deleteTokens(tokens.size());
}

void Parser::enter(ParseTreeNode* n)
{
    if (n->getSymbol() == FULL_COMMAND) {
        // get host alias, push it onto hostAliasStack
        std::string alias;
        auto setHost = n->findSymbol(SET_HOST);
        if (!setHost.empty()) {
            alias = setHost[0]->findSymbol(VARNAME).at(0)->concatTokens();
        }
        hostAliasStack.push(alias);
    }
    else if (n->getSymbol() == SIMPLE_COMMAND) {
        auto argNodes = n->findSymbol(ARG);
        std::vector<std::string> args;
        for (auto an : argNodes) {
            args.push_back(an->concatTokens());
        }
        auto hostOverride = n->findSymbol(CMD_HOST);
        std::string hostAlias;
        if (!hostOverride.empty()) {
            auto hostName = hostOverride.at(0)->findSymbol(VARNAME);
            if (!hostName.empty()) {
                hostAlias = hostName.at(0)->concatTokens();
            }
        }
        else {
            hostAlias = hostAliasStack.top();
        }
        cmdStack.push(new SimpleCommand(hostAlias, args));
    }
    else if (n->getSymbol() == PIPE_COMMAND) {
        
    }
    else if (n->getSymbol() == DEFINE_HOST) {
        // get host alias
        std::string alias = n->getChildren()[0]->concatTokens();

        // get username, hostname, and port
        HostInfo info;
        std::string hostStr = n->findSymbol(ARG).at(0)->concatTokens();
        hostStr += n->findSymbol(flasshGrammar.opt(HOST_PORT)).at(0)->concatTokens();
        
        if (!info.parse(hostStr)) {
            throw std::runtime_error("bad host definition");
        }

        cmdStack.push(new NewHostCommand(alias, info));
    }
}

void Parser::leave(ParseTreeNode* n)
{
    if (n->getSymbol() == FULL_COMMAND) {
        hostAliasStack.pop();
        // push all commands on the command stack to the command queue in reverse order
        std::stack<Command*> reverseCmdStack;
        while (!cmdStack.empty()) {
            reverseCmdStack.push(cmdStack.top());
            cmdStack.pop();
        }
        while (!reverseCmdStack.empty()) {
            commands.push(reverseCmdStack.top());
            reverseCmdStack.pop();
        }
    }
    else if (n->getSymbol() == PIPE_COMMAND) {
        auto right = cmdStack.top();
        cmdStack.pop();
        auto left = cmdStack.top();
        cmdStack.pop();
        cmdStack.push(new PipeCommand(left, right));
    }
}

void Parser::deleteTokens(size_t numTokens)
{
    while (tokens.size() > 0 && numTokens > 0) {
        delete tokens.front();
        tokens.pop_front();
        --numTokens;
    }
}
