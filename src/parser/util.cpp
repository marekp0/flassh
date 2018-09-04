#include "util.hpp"
#include "symbols.hpp"
#include "lexer.hpp"

void ContextFreeGrammar::addRule(const ProductionRule& rule)
{
    rules[rule.nonTerminal].push_back(rule);
}



void ParseTreeNode::traverse(const std::function<void(ParseTreeNode*)>& callback)
{
    callback(this);
    for (auto c : children) {
        c->traverse(callback);
    }
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
                throw std::runtime_error("parser error");
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
            nodeStack.push(newNode);
        }
    }

    // should never happen
    if (!nodeStack.empty())
        throw std::runtime_error("parser error");

    return root;
}
