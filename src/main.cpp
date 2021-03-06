#include "parser/parser.hpp"
#include "parser/lexer.hpp"
#include "context.hpp"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

int runScript(const std::vector<std::string>& args);

int main(int argc, char** argv)
{
    if (argc == 1) {
        // interactive mode
        Context ctx;
        Parser p;
        std::string line;
        printf("> ");
        while (std::getline(std::cin, line)) {
            p.parse(line + "\n");
            Command* c = nullptr;
            while ((c = p.popCommand()) != nullptr) {
                ctx.enqueueCommand(c);
                ctx.flushCmdQueue();
            }
            printf(p.isComplete() ? "> " : ">> ");
        }
    }
    else {
        std::vector<std::string> args;
        for (int i = 1; i < argc; i++) {
            args.push_back(argv[i]);
        }
        return runScript(args);
    }
    return 0;
}

int runScript(const std::vector<std::string>& args)
{
    // http://tldp.org/LDP/abs/html/exitcodes.html
    // TODO: give names to these return value constants
    std::ifstream inFile(args[0], std::ios::binary);
    if (!inFile) {
        fprintf(stderr, "flassh: failed to open %s\n", args[0].c_str());
        return 127;
    }

    // read file into buffer
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    buffer << "\n";     // newline required to end commands

    Context ctx;
    Parser p;
    p.parse(buffer.str());
    if (!p.isComplete()) {
        fprintf(stderr, "flassh: Unexpected EOF\n");
        return 2;
    }
    Command* c = nullptr;
    while ((c = p.popCommand()) != nullptr) {
        ctx.enqueueCommand(c);
    }
    ctx.flushCmdQueue();

    return 0;
}
