#include "util.hpp"
#include "symbols.hpp"
#include "lexer.hpp"

void ContextFreeGrammar::addRule(int lhs, const std::vector<SymbolSeq>& rhs)
{
    for (auto& r : rhs) {
        rules[lhs].push_back(ProductionRule{ lhs, r });
    }
}

int ContextFreeGrammar::opt(const SymbolSeq& symbols)
{
    // get existing symbol if we already made one for this sequence
    auto it = optionalSymbols.find(symbols);
    if (it != optionalSymbols.end())
        return it->second;

    // OPT_X ==> <empty> | X
    int sym = nextVirtSymbol--;
    optionalSymbols[symbols] = sym;
    addRule(sym, { {}, symbols });
    return sym;
}

int ContextFreeGrammar::ge0(const SymbolSeq& symbols)
{
    // get existing symbol if we already made one for this sequence
    auto it = atLeast0Symbols.find(symbols);
    if (it != atLeast0Symbols.end())
        return it->second;

    // AT_LEAST_0_X ==> <empty> | X AT_LEAST_0_X
    int sym = nextVirtSymbol--;
    atLeast0Symbols[symbols] = sym;
    SymbolSeq rhs = symbols;
    rhs.push_back(sym);
    addRule(sym, { {}, rhs });
    return sym;
}

int ContextFreeGrammar::ge1(const SymbolSeq& symbols)
{
    // get existing symbol if we already made one for this sequence
    auto it = atLeast1Symbols.find(symbols);
    if (it != atLeast1Symbols.end())
        return it->second;

    // AT_LEAST_1_X ==> X AT_LEAST_0_X
    int sym = nextVirtSymbol--;
    atLeast1Symbols[symbols] = sym;
    SymbolSeq rhs = symbols;
    rhs.push_back(ge0(symbols));
    addRule(sym, { rhs });
    return sym;
}



void ParseTreeNode::traverse(const TraverseCallback& onEnter, const TraverseCallback& onLeave)
{
    if (onEnter) onEnter(this);
    for (auto c : children) {
        c->traverse(onEnter, onLeave);
    }
    if (onLeave) onLeave(this);
}

std::vector<ParseTreeNode*> ParseTreeNode::findSymbol(int symbol)
{
    std::vector<ParseTreeNode*> ret;
    traverse([&ret, symbol](ParseTreeNode* node) {
        if (node->symbol == symbol) {
            ret.push_back(node);
        }
    });

    return ret;
}

std::string ParseTreeNode::concatTokens()
{
    std::string str;
    traverse([&str](ParseTreeNode* node) {
        if (node->token != nullptr) {
            str.append(node->token->str);
        }
    });

    return str;
}

ParseTreeNode* ParseTreeNode::createParseTree(const ContextFreeGrammar& grammar,
                                              const std::vector<int>& decisions,
                                              const std::deque<Token*>& tokens)
{
    // basically, we're replaying the PDA that was successful and turning that
    // into the parse tree

    // TODO: memory leaks on error

    SymbolStack symStack;
    symStack.push(grammar.startSymbol);

    std::stack<ParseTreeNode*> nodeStack;
    size_t nextToken = 0;
    size_t nextDecision = 0;
    ParseTreeNode* root = nullptr;

    while (!symStack.empty()) {
        // pop PDA stack
        int curSym = symStack.top();
        symStack.pop();

        // create new tree node
        ParseTreeNode* newNode = new ParseTreeNode;
        newNode->symbol = curSym;

        // add node to parent
        if (!nodeStack.empty()) {
            auto& top = nodeStack.top();
            top->children.push_back(newNode);

            // if filled up all children of parent node, pop it from the stack
            if (top->children.size() >= top->rule->replacement.size()) {
                nodeStack.pop();
            }
        }
        else {
            if (root != nullptr) {
                // should never happen
                throw std::runtime_error("parser error, multiple parse tree roots");
            }
            root = newNode;
        }

        if (Symbols::isTerminal(curSym)) {
            // set token
            newNode->token = tokens.at(nextToken++);
        }
        else {
            // get production rule, using at() for bounds checking
            auto& rule = grammar.rules.at(curSym).at(decisions.at(nextDecision++));
            newNode->rule = &rule;

            // push replacement symbols onto PDA stack in right-to-left order
            for (size_t i = 0; i < rule.replacement.size(); i++) {
                symStack.push(rule.replacement[rule.replacement.size() - i - 1]);
            }

            // push tree node onto the stack so that future nodes will be added
            // as children to this node
            if (rule.replacement.size() > 0)
                nodeStack.push(newNode);
        }
    }

    // should never happen
    if (!nodeStack.empty())
        throw std::runtime_error("parser error, unknown nodes");

    return root;
}

int ParseTreeNode::getLine() const
{
    if (token == nullptr) {
        if (!children.empty()) {
            return children[0]->getLine();
        }
        else {
            // should never happen
            return -1;
        }
    }
    else {
        return token->line;
    }
}
