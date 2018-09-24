#include "parser/parser.hpp"
#include "parser/lexer.hpp"
#include "context.hpp"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

void runScript(const std::vector<std::string>& args);

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
        runScript(args);
    }
    return 0;
}

void runScript(const std::vector<std::string>& args)
{
    std::ifstream inFile(args[0], std::ios::binary);
    if (!inFile) {
        fprintf(stderr, "flassh: failed to open %s\n", args[0].c_str());
        return;
    }

    // read file into buffer
    inFile.seekg(0, inFile.end);
    int len = inFile.tellg();
    inFile.seekg(0, inFile.beg);

    std::string buf(len + 1, 0);
    inFile.read(buf.data(), len);
    buf[len] = '\n';

    Context ctx;
    Parser p;
    p.parse(buf);
    if (!p.isComplete()) {
        fprintf(stderr, "flassh: Unexpected EOF\n");
        return;
    }
    Command* c = nullptr;
    while ((c = p.popCommand()) != nullptr) {
        ctx.enqueueCommand(c);
    }
    ctx.flushCmdQueue();
}
