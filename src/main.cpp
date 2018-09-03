#include "parser/parser.hpp"
#include "parser/lexer.hpp"
#include <cstdio>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    if (argc == 1) {
        // interactive mode
        Lexer lex;
        std::string line;
        printf("> ");
        while (std::getline(std::cin, line)) {
            lex.input(line + "\n");
            while (Token* tok = lex.popToken()) {
                printf("%d ", tok->symbol);
            }
            printf("\n> ", getenv("PS1"));
        }
        /*
        Parser p;
        std::string line;
        printf("> ", getenv("PS1"));
        while (std::getline(std::cin, line)) {
            auto cmds = p.parse(line + "\n");
            for (auto& c : cmds) {
                c->run();
            }
            printf("> ", getenv("PS1"));
        }*/
    }
    else {
        fprintf(stderr, "Running scripts not implemented");
    }
    return 0;
}

