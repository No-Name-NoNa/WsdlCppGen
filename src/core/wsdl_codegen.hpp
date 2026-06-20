#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace WsdlCppGen {
    struct GenerateOptions {
        std::filesystem::path wsdl_path;
        std::filesystem::path output_dir;
        std::string service_name_override;
    };

    struct GenerateResult {
        std::filesystem::path header_path;
        std::filesystem::path source_path;
        std::string service_name;
        std::vector<std::string> operations;
    };

    GenerateResult GenerateFromWsdlFile(const GenerateOptions &options);
}
