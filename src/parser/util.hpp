#pragma once

#include <vector>
#include <map>
#include <stack>
#include <functional>

/**
 * A production rule for a context-free grammar
 */
struct ProductionRule {
    int nonTerminal;
    std::vector<int> replacement;
};

/**
 * Contains all production rules for a context-free grammar
 */
class ContextFreeGrammar {
public:
    void addRule(const ProductionRule& rule);

    std::map<int, std::vector<ProductionRule>> rules;
    int startSymbol;
};

/**
 * A stack of symbols used while parsing. This is the stack used by the
 * push-down automaton.
 */
typedef std::stack<int> SymbolStack;

class Token;

/**
 * A node in the parse tree.
 */
class ParseTreeNode {
public:
    ~ParseTreeNode();

    typedef std::function<void(ParseTreeNode*)> TraverseCallback;

    /**
     * Calls the callback with the current node first, then each of the
     * children in order.
     */
    void traverse(const TraverseCallback& onEnter, const TraverseCallback& onLeave = nullptr);

    /**
     * Returns all child nodes with the given symbol in traversal order.
     */
    std::vector<ParseTreeNode*> findSymbol(int symbol);

    /**
     * Concatenates all tokens under the current node
     */
    std::string concatTokens();

    /**
     * Builds a parse tree given a grammar, list of decisions for production
     * rules in depth-first order, and the list of tokens
     */
    static ParseTreeNode* createParseTree(const ContextFreeGrammar& grammar,
                                          const std::vector<int>& decisions,
                                          const std::deque<Token*>& tokens);

    const std::vector<ParseTreeNode*>& getChildren() { return children; }
    int getSymbol() const { return symbol; }
    const Token * getToken() const { return token; }
    const ProductionRule * getProductionRule() const { return rule; }

    int getLine() const;

private:
    std::vector<ParseTreeNode*> children;

    int symbol;
    Token* token = nullptr;                 // non-null if terminal
    const ProductionRule* rule = nullptr;   // non-null if nonterminal
};
