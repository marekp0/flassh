#pragma once

#include <vector>
#include <map>
#include <stack>
#include <functional>

typedef std::vector<int> SymbolSeq;

/**
 * A production rule for a context-free grammar
 */
struct ProductionRule {
    int nonTerminal;
    SymbolSeq replacement;
};

/**
 * Contains all production rules for a context-free grammar
 */
class ContextFreeGrammar {
public:
    /**
     * Adds a new production rule to the grammar of the form
     * `lhs ==> rhs[0] | rhs[1] | ... | rhs[n]`.
     */
    void addRule(int lhs, const std::vector<SymbolSeq>& rhs);

    /**
     * Returns a new non-terminal symbol representing either 0 or 1 of the
     * input symbol sequence
     */
    int opt(const SymbolSeq& symbols);
    int opt(int symb) { return opt(SymbolSeq{ symb }); }

    /**
     * Returns a new non-terminal symbol representing 0 or more of the input
     * symbol sequence
     */
    int ge0(const SymbolSeq& symbols);
    int ge0(int symb) { return ge0(SymbolSeq{ symb }); }

    /**
     * Returns a new non-terminal symbol representing 1 or more of the input
     * symbol sequence
     */
    int ge1(const SymbolSeq& symbols);
    int ge1(int symb) { return ge1(SymbolSeq{ symb }); }

    std::map<int, std::vector<ProductionRule>> rules;
    int startSymbol;

private:
    // a "virtual symbol" is created implicitly by optional(), etc
    int nextVirtSymbol = -1;

    // reuse virtual symbols when possible
    std::map<SymbolSeq, int> optionalSymbols;
    std::map<SymbolSeq, int> atLeast0Symbols;
    std::map<SymbolSeq, int> atLeast1Symbols;
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
