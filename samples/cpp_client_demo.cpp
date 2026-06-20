#include "AddServiceClient.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        GeneratedWsdl::AddServiceClient client("http://localhost:8080/services/AddService");
        GeneratedWsdl::AddResponse response = client.Add(3, 5);
        std::cout << "3 + 5 = " << response.result << '\n';
        return response.result == 8 ? 0 : 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Call failed: " << error.what() << '\n';
        return 1;
    }
}
