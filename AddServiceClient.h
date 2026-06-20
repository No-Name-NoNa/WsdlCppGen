#pragma once

#include <string>

namespace GeneratedWsdl
{
struct AddRequest
{
    int num1{};
    int num2{};
};

struct AddResponse
{
    int result{};
};

class AddServiceClient
{
public:
    explicit AddServiceClient(std::string endpoint = "http://localhost:8080/services/AddService");
    const std::string& Endpoint() const noexcept;

    AddResponse Add(const AddRequest& request) const;
    AddResponse Add(int num1, int num2) const;

private:
    std::string endpoint_;
};
}
