#pragma once

#include <optional>
#include <string>

namespace WsdlCppGen
{
struct DialogResult
{
    std::optional<std::string> path;
    std::string error;
};

DialogResult SelectWsdlFile();
DialogResult SelectOutputDirectory();
DialogResult OpenDirectoryInShell(const std::string& path);
}
