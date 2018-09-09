#include "parser/parser.hpp"
#include "parser/lexer.hpp"
#include "context.hpp"
#include <cstdio>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    Context ctx;
    if (argc == 1) {
        // interactive mode
        Parser p;
        std::string line;
        printf("> ");
        while (std::getline(std::cin, line)) {
            p.parse(line + "\n");
            Command* c = nullptr;
            while ((c = p.popCommand()) != nullptr) {
                c->run(&ctx);
                delete c;
            }
            printf(p.isComplete() ? "> " : ">> ");
        }
    }
    else {
        fprintf(stderr, "Running scripts not implemented");
    }
    return 0;
}

