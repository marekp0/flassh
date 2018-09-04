#include "parser.hpp"
#include "util.hpp"
#include "symbols.hpp"
#include <stack>

using namespace Symbols;

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
    startSymbol = COMMAND;

    addRule({ COMMAND, { ARG_LIST, NEWLINE } });

    addRule({ ARG_LIST, { ARG }});
    addRule({ ARG_LIST, { ARG, SPACE, ARG_LIST } });

    addRule({ ARG, { VARNAME } });
    addRule({ ARG, { STR } });
}



/**
 * ???
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
    return lex.isComplete();
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
        }

        // if the top of the stack is a terminal symbol, then we made a wrong decision
        if (!symStack.empty() && isTerminal(symStack.top())) {
            // try the next rule next time
            ++dstk.top().ruleChosen;
            continue;
        }

        // check if there's a new non-terminal symbol
        if (!symStack.empty() && !isTerminal(symStack.top())) {
            // try rules from the beginning next time
            state.ruleChosen = 0;
            dstk.push(state);
        }
        
        if (symStack.empty()) {
            // we're done!
            break;
        }
    }

    if (dstk.empty()) {
        fprintf(stderr, "syntax error\n");
        deleteTokens(tokens.size());
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
    parseTree->traverse([this](ParseTreeNode* node) {
        buildCommands(node);
    });

    // FIXME
    deleteTokens(tokens.size());
}

void Parser::buildCommands(ParseTreeNode* n)
{
    if (n->getSymbol() == COMMAND) {
        auto argNodes = n->findSymbol(ARG);
        std::vector<std::string> args;
        for (auto an : argNodes) {
            args.push_back(an->concatTokens());
        }
        commands.push(new SimpleCommand(args));
    }
}

void Parser::deleteTokens(size_t numTokens)
{
    while (tokens.size() > 0 && numTokens > 0) {
        delete tokens.front();
        tokens.pop_front();
    }
}
