#ifndef COMMAND_LINE_PARSER_H
#define COMMAND_LINE_PARSER_H

#include <string>

struct CommandLineArgs {
    std::string configPath;
    bool localFlag;
    bool parallelFlag;
    bool helpFlag;
};

CommandLineArgs parseCommandLine(int argc, char* argv[]);
void printHelp();

#endif // COMMAND_LINE_PARSER_H