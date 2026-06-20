#include "wsdl_codegen.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

int RunGuiApp(const char* executable_path);

namespace
{
void PrintUsage()
{
    std::cout
        << "Usage:\n"
        << "  WsdlCppGen --wsdl <file.wsdl> --out <dir> [--service-name <Name>]\n"
        << "  WsdlCppGen --help\n"
        << "\n"
        << "No arguments starts the ImGui interface.\n";
}

bool HasArg(const std::vector<std::string>& args, const std::string& name)
{
    return std::find(args.begin(), args.end(), name) != args.end();
}

std::string ValueAfter(const std::vector<std::string>& args, const std::string& name)
{
    const auto it = std::find(args.begin(), args.end(), name);
    if (it == args.end() || std::next(it) == args.end())
        return {};
    return *std::next(it);
}
}

int main(int argc, char** argv)
{
    try
    {
        std::vector<std::string> args(argv + 1, argv + argc);
        if (args.empty())
            return RunGuiApp(argv[0]);

        if (HasArg(args, "--help") || HasArg(args, "-h"))
        {
            PrintUsage();
            return 0;
        }

        WsdlCppGen::GenerateOptions options;
        options.wsdl_path = ValueAfter(args, "--wsdl");
        options.output_dir = ValueAfter(args, "--out");
        options.service_name_override = ValueAfter(args, "--service-name");

        WsdlCppGen::GenerateResult result = WsdlCppGen::GenerateFromWsdlFile(options);
        std::cout << "Generated service: " << result.service_name << "\n";
        std::cout << "Header: " << result.header_path.string() << "\n";
        std::cout << "Source: " << result.source_path.string() << "\n";
        std::cout << "Operations:";
        for (const std::string& operation : result.operations)
            std::cout << ' ' << operation;
        std::cout << "\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << "\n";
        PrintUsage();
        return 1;
    }
}
