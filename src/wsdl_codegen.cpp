#include "wsdl_codegen.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace WsdlCppGen
{
namespace
{
struct XmlNode
{
    std::string name;
    std::map<std::string, std::string> attrs;
    std::vector<XmlNode> children;
    std::string text;
};

class XmlParser
{
public:
    explicit XmlParser(std::string_view input) : input_(input) {}

    XmlNode ParseDocument()
    {
        SkipSpaceAndSpecial();
        XmlNode root = ParseNode();
        SkipSpaceAndSpecial();
        return root;
    }

private:
    void SkipWhitespace()
    {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_])))
            ++pos_;
    }

    bool StartsWith(std::string_view text) const
    {
        return input_.substr(pos_, text.size()) == text;
    }

    void Expect(char ch)
    {
        if (pos_ >= input_.size() || input_[pos_] != ch)
            throw std::runtime_error(std::string("Malformed XML: expected '") + ch + "'");
        ++pos_;
    }

    void SkipUntil(std::string_view marker)
    {
        const size_t found = input_.find(marker, pos_);
        if (found == std::string_view::npos)
            throw std::runtime_error("Malformed XML: unterminated special section");
        pos_ = found + marker.size();
    }

    void SkipSpaceAndSpecial()
    {
        for (;;)
        {
            SkipWhitespace();
            if (StartsWith("<?"))
                SkipUntil("?>");
            else if (StartsWith("<!--"))
                SkipUntil("-->");
            else
                break;
        }
    }

    static bool IsNameChar(char ch)
    {
        return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-' || ch == ':' || ch == '.';
    }

    std::string ParseName()
    {
        const size_t begin = pos_;
        while (pos_ < input_.size() && IsNameChar(input_[pos_]))
            ++pos_;
        if (begin == pos_)
            throw std::runtime_error("Malformed XML: expected name");
        return std::string(input_.substr(begin, pos_ - begin));
    }

    std::string ParseQuoted()
    {
        SkipWhitespace();
        if (pos_ >= input_.size() || (input_[pos_] != '"' && input_[pos_] != '\''))
            throw std::runtime_error("Malformed XML: expected quoted value");
        const char quote = input_[pos_++];
        const size_t begin = pos_;
        while (pos_ < input_.size() && input_[pos_] != quote)
            ++pos_;
        if (pos_ >= input_.size())
            throw std::runtime_error("Malformed XML: unterminated quoted value");
        std::string value(input_.substr(begin, pos_ - begin));
        ++pos_;
        return value;
    }

    XmlNode ParseNode()
    {
        Expect('<');
        if (pos_ < input_.size() && input_[pos_] == '/')
            throw std::runtime_error("Malformed XML: unexpected closing tag");

        XmlNode node;
        node.name = ParseName();

        for (;;)
        {
            SkipWhitespace();
            if (StartsWith("/>"))
            {
                pos_ += 2;
                return node;
            }
            if (StartsWith(">"))
            {
                ++pos_;
                break;
            }

            std::string key = ParseName();
            SkipWhitespace();
            Expect('=');
            node.attrs[key] = ParseQuoted();
        }

        for (;;)
        {
            if (pos_ >= input_.size())
                throw std::runtime_error("Malformed XML: unterminated element " + node.name);

            if (StartsWith("<!--"))
            {
                SkipUntil("-->");
                continue;
            }
            if (StartsWith("</"))
            {
                pos_ += 2;
                std::string close_name = ParseName();
                if (close_name != node.name)
                    throw std::runtime_error("Malformed XML: mismatched closing tag " + close_name);
                SkipWhitespace();
                Expect('>');
                return node;
            }
            if (input_[pos_] == '<')
            {
                node.children.push_back(ParseNode());
                continue;
            }

            const size_t begin = pos_;
            while (pos_ < input_.size() && input_[pos_] != '<')
                ++pos_;
            node.text.append(input_.substr(begin, pos_ - begin));
        }
    }

    std::string_view input_;
    size_t pos_ = 0;
};

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open WSDL file: " + path.string());
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

void WriteFile(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to write file: " + path.string());
    file << content;
}

std::string LocalName(std::string_view qname)
{
    const size_t colon = qname.rfind(':');
    if (colon == std::string_view::npos)
        return std::string(qname);
    return std::string(qname.substr(colon + 1));
}

std::string AttrLocal(const XmlNode& node, std::string_view key)
{
    for (const auto& [attr_name, value] : node.attrs)
        if (LocalName(attr_name) == key)
            return value;
    return {};
}

const XmlNode* FirstChildLocal(const XmlNode& node, std::string_view name)
{
    for (const XmlNode& child : node.children)
        if (LocalName(child.name) == name)
            return &child;
    return nullptr;
}

std::vector<const XmlNode*> ChildrenLocal(const XmlNode& node, std::string_view name)
{
    std::vector<const XmlNode*> result;
    for (const XmlNode& child : node.children)
        if (LocalName(child.name) == name)
            result.push_back(&child);
    return result;
}

std::vector<const XmlNode*> DescendantsLocal(const XmlNode& node, std::string_view name)
{
    std::vector<const XmlNode*> result;
    for (const XmlNode& child : node.children)
    {
        if (LocalName(child.name) == name)
            result.push_back(&child);
        auto nested = DescendantsLocal(child, name);
        result.insert(result.end(), nested.begin(), nested.end());
    }
    return result;
}

std::string ToIdentifier(std::string_view text, bool upper_first)
{
    std::string out;
    bool upper_next = upper_first;
    for (const char ch : text)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)))
        {
            char value = ch;
            if (out.empty() && std::isdigit(static_cast<unsigned char>(value)))
                out.push_back('_');
            if (upper_next)
                value = static_cast<char>(std::toupper(static_cast<unsigned char>(value)));
            out.push_back(value);
            upper_next = false;
        }
        else
        {
            upper_next = true;
        }
    }
    if (out.empty())
        out = upper_first ? "Generated" : "generated";
    return out;
}

std::string ToVariableIdentifier(std::string_view text)
{
    std::string id = ToIdentifier(text, false);
    if (!id.empty())
        id[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(id[0])));
    static const std::vector<std::string> keywords = {
        "class", "struct", "int", "double", "float", "long", "bool", "return", "namespace", "template", "operator", "string"};
    if (std::find(keywords.begin(), keywords.end(), id) != keywords.end())
        id += "_";
    return id;
}

std::string MapXsdType(std::string_view xsd_type)
{
    const std::string local = LocalName(xsd_type);
    if (local == "int" || local == "integer")
        return "int";
    if (local == "long")
        return "long long";
    if (local == "double")
        return "double";
    if (local == "float")
        return "float";
    if (local == "bool" || local == "boolean")
        return "bool";
    if (local == "string")
        return "std::string";
    throw std::runtime_error("Unsupported XSD type: " + std::string(xsd_type));
}

struct Field
{
    std::string xml_name;
    std::string cpp_name;
    std::string cpp_type;
};

struct Element
{
    std::string xml_name;
    std::string cpp_name;
    std::vector<Field> fields;
};

struct Operation
{
    std::string name;
    std::string cpp_name;
    std::string input_element;
    std::string output_element;
    std::string soap_action;
};

struct WsdlModel
{
    std::string target_namespace;
    std::string service_name;
    std::string endpoint;
    std::map<std::string, Element> elements;
    std::vector<Operation> operations;
};

Element ParseSchemaElement(const XmlNode& element_node)
{
    Element element;
    element.xml_name = AttrLocal(element_node, "name");
    element.cpp_name = ToIdentifier(element.xml_name, true);
    if (element.xml_name.empty())
        throw std::runtime_error("Schema element is missing name");

    const XmlNode* complex_type = FirstChildLocal(element_node, "complexType");
    if (!complex_type)
        throw std::runtime_error("Schema element " + element.xml_name + " is missing complexType");
    const XmlNode* sequence = FirstChildLocal(*complex_type, "sequence");
    if (!sequence)
        throw std::runtime_error("Schema element " + element.xml_name + " is missing sequence");

    for (const XmlNode* field_node : ChildrenLocal(*sequence, "element"))
    {
        Field field;
        field.xml_name = AttrLocal(*field_node, "name");
        field.cpp_name = ToVariableIdentifier(field.xml_name);
        field.cpp_type = MapXsdType(AttrLocal(*field_node, "type"));
        if (field.xml_name.empty())
            throw std::runtime_error("Field in " + element.xml_name + " is missing name");
        element.fields.push_back(field);
    }
    return element;
}

WsdlModel ParseWsdl(const std::string& xml, const std::string& service_name_override)
{
    XmlParser parser(xml);
    XmlNode root = parser.ParseDocument();
    if (LocalName(root.name) != "definitions")
        throw std::runtime_error("WSDL root must be wsdl:definitions");

    WsdlModel model;
    model.target_namespace = AttrLocal(root, "targetNamespace");
    if (model.target_namespace.empty())
        throw std::runtime_error("WSDL definitions missing targetNamespace");

    const XmlNode* types = FirstChildLocal(root, "types");
    if (!types)
        throw std::runtime_error("WSDL missing types");
    const XmlNode* schema = FirstChildLocal(*types, "schema");
    if (!schema)
        throw std::runtime_error("WSDL types missing xsd:schema");

    for (const XmlNode* element_node : ChildrenLocal(*schema, "element"))
    {
        Element element = ParseSchemaElement(*element_node);
        model.elements.emplace(element.xml_name, std::move(element));
    }

    std::map<std::string, std::string> message_to_element;
    for (const XmlNode* message : ChildrenLocal(root, "message"))
    {
        const std::string message_name = AttrLocal(*message, "name");
        const XmlNode* part = FirstChildLocal(*message, "part");
        if (message_name.empty() || !part)
            continue;
        message_to_element[message_name] = LocalName(AttrLocal(*part, "element"));
    }

    std::map<std::string, std::string> soap_actions;
    const XmlNode* binding = FirstChildLocal(root, "binding");
    if (binding)
    {
        for (const XmlNode* operation : ChildrenLocal(*binding, "operation"))
        {
            const std::string operation_name = AttrLocal(*operation, "name");
            const XmlNode* soap_operation = FirstChildLocal(*operation, "operation");
            if (!operation_name.empty() && soap_operation)
                soap_actions[operation_name] = AttrLocal(*soap_operation, "soapAction");
        }
    }

    const XmlNode* port_type = FirstChildLocal(root, "portType");
    if (!port_type)
        throw std::runtime_error("WSDL missing portType");
    for (const XmlNode* operation_node : ChildrenLocal(*port_type, "operation"))
    {
        Operation operation;
        operation.name = AttrLocal(*operation_node, "name");
        operation.cpp_name = ToIdentifier(operation.name, true);
        const XmlNode* input = FirstChildLocal(*operation_node, "input");
        const XmlNode* output = FirstChildLocal(*operation_node, "output");
        if (operation.name.empty() || !input || !output)
            throw std::runtime_error("portType operation is missing name/input/output");

        const std::string input_message = LocalName(AttrLocal(*input, "message"));
        const std::string output_message = LocalName(AttrLocal(*output, "message"));
        if (!message_to_element.contains(input_message) || !message_to_element.contains(output_message))
            throw std::runtime_error("Operation " + operation.name + " references unknown message");

        operation.input_element = message_to_element[input_message];
        operation.output_element = message_to_element[output_message];
        operation.soap_action = soap_actions[operation.name];
        if (!model.elements.contains(operation.input_element) || !model.elements.contains(operation.output_element))
            throw std::runtime_error("Operation " + operation.name + " references unknown schema element");
        model.operations.push_back(std::move(operation));
    }
    if (model.operations.empty())
        throw std::runtime_error("WSDL has no operations");

    const XmlNode* service = FirstChildLocal(root, "service");
    if (!service)
        throw std::runtime_error("WSDL missing service");
    model.service_name = service_name_override.empty() ? AttrLocal(*service, "name") : service_name_override;
    if (model.service_name.empty())
        model.service_name = "WebService";
    const XmlNode* address = nullptr;
    for (const XmlNode* candidate : DescendantsLocal(*service, "address"))
    {
        if (!AttrLocal(*candidate, "location").empty())
        {
            address = candidate;
            break;
        }
    }
    if (!address)
        throw std::runtime_error("WSDL service missing soap:address");
    model.endpoint = AttrLocal(*address, "location");
    return model;
}

std::string CppStringLiteral(std::string_view text)
{
    std::string out = "\"";
    for (const char ch : text)
    {
        switch (ch)
        {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += ch; break;
        }
    }
    out += "\"";
    return out;
}

bool NeedsStringInclude(const WsdlModel& model)
{
    for (const auto& [_, element] : model.elements)
        for (const Field& field : element.fields)
            if (field.cpp_type == "std::string")
                return true;
    return true;
}

std::string GenerateHeader(const WsdlModel& model)
{
    const std::string class_name = ToIdentifier(model.service_name, true) + "Client";
    std::ostringstream out;
    out << "#pragma once\n\n";
    if (NeedsStringInclude(model))
        out << "#include <string>\n";
    out << "\n";
    out << "namespace GeneratedWsdl\n{\n";

    for (const auto& [_, element] : model.elements)
    {
        out << "struct " << element.cpp_name << "\n{\n";
        for (const Field& field : element.fields)
            out << "    " << field.cpp_type << " " << field.cpp_name << "{};\n";
        out << "};\n\n";
    }

    out << "class " << class_name << "\n{\n";
    out << "public:\n";
    out << "    explicit " << class_name << "(std::string endpoint = " << CppStringLiteral(model.endpoint) << ");\n";
    out << "    const std::string& Endpoint() const noexcept;\n\n";

    for (const Operation& operation : model.operations)
    {
        const Element& input = model.elements.at(operation.input_element);
        const Element& output = model.elements.at(operation.output_element);
        out << "    " << output.cpp_name << " " << operation.cpp_name << "(const " << input.cpp_name << "& request) const;\n";
        out << "    " << output.cpp_name << " " << operation.cpp_name << "(";
        for (size_t i = 0; i < input.fields.size(); ++i)
        {
            const Field& field = input.fields[i];
            if (i)
                out << ", ";
            if (field.cpp_type == "std::string")
                out << "const std::string& ";
            else
                out << field.cpp_type << " ";
            out << field.cpp_name;
        }
        out << ") const;\n";
    }

    out << "\nprivate:\n";
    out << "    std::string endpoint_;\n";
    out << "};\n";
    out << "}\n";
    return out.str();
}

std::string ToXmlValueExpression(const Field& field, const std::string& access)
{
    if (field.cpp_type == "std::string")
        return "XmlEscape(" + access + ")";
    if (field.cpp_type == "bool")
        return "(" + access + " ? \"true\" : \"false\")";
    return "std::to_string(" + access + ")";
}

std::string FromXmlExpression(const Field& field, const std::string& value_expr)
{
    if (field.cpp_type == "std::string")
        return value_expr;
    if (field.cpp_type == "bool")
        return "(" + value_expr + " == \"true\" || " + value_expr + " == \"1\")";
    if (field.cpp_type == "long long")
        return "std::stoll(" + value_expr + ")";
    if (field.cpp_type == "double")
        return "std::stod(" + value_expr + ")";
    if (field.cpp_type == "float")
        return "std::stof(" + value_expr + ")";
    return "std::stoi(" + value_expr + ")";
}

std::string GenerateSource(const WsdlModel& model)
{
    const std::string service_id = ToIdentifier(model.service_name, true);
    const std::string class_name = service_id + "Client";
    const std::string header_name = service_id + "Client.h";

    std::ostringstream out;
    out << "#include " << CppStringLiteral(header_name) << "\n\n";
    out << "#include <sstream>\n#include <stdexcept>\n#include <string>\n#include <utility>\n\n";
    out << "#if defined(_WIN32) && !defined(WSDLCPPGEN_USE_LIBCURL)\n";
    out << "#ifndef NOMINMAX\n#define NOMINMAX\n#endif\n#include <windows.h>\n#include <winhttp.h>\n";
    out << "#if defined(_MSC_VER)\n#pragma comment(lib, \"winhttp.lib\")\n#endif\n";
    out << "#else\n#include <curl/curl.h>\n#endif\n\n";

    out << "namespace GeneratedWsdl\n{\nnamespace\n{\n";
    out << "std::string XmlEscape(const std::string& value)\n{\n";
    out << "    std::string out;\n    for (char ch : value)\n    {\n";
    out << "        switch (ch)\n        {\n";
    out << "        case '&': out += \"&amp;\"; break;\n";
    out << "        case '<': out += \"&lt;\"; break;\n";
    out << "        case '>': out += \"&gt;\"; break;\n";
    out << "        case '\\\"': out += \"&quot;\"; break;\n";
    out << "        case '\\'': out += \"&apos;\"; break;\n";
    out << "        default: out += ch; break;\n";
    out << "        }\n    }\n    return out;\n}\n\n";

    out << "std::string XmlUnescape(std::string value)\n{\n";
    out << "    const std::pair<const char*, const char*> replacements[] = {{\"&lt;\", \"<\"}, {\"&gt;\", \">\"}, {\"&quot;\", \"\\\"\"}, {\"&apos;\", \"'\"}, {\"&amp;\", \"&\"}};\n";
    out << "    for (const auto& [from, to] : replacements)\n    {\n";
    out << "        size_t pos = 0;\n        while ((pos = value.find(from, pos)) != std::string::npos)\n        {\n";
    out << "            value.replace(pos, std::char_traits<char>::length(from), to);\n";
    out << "            pos += std::char_traits<char>::length(to);\n";
    out << "        }\n    }\n    return value;\n}\n\n";

    out << "bool TagNameMatches(const std::string& tag, const std::string& local_name)\n{\n";
    out << "    size_t begin = tag.find_first_not_of(\" \\t\\r\\n/<\");\n";
    out << "    if (begin == std::string::npos) return false;\n";
    out << "    size_t end = tag.find_first_of(\" \\t\\r\\n>/\", begin);\n";
    out << "    std::string name = tag.substr(begin, end == std::string::npos ? std::string::npos : end - begin);\n";
    out << "    size_t colon = name.rfind(':');\n";
    out << "    if (colon != std::string::npos) name = name.substr(colon + 1);\n";
    out << "    return name == local_name;\n}\n\n";

    out << "std::string ExtractTagText(const std::string& xml, const std::string& local_name)\n{\n";
    out << "    size_t pos = 0;\n";
    out << "    while ((pos = xml.find('<', pos)) != std::string::npos)\n    {\n";
    out << "        if (pos + 1 < xml.size() && xml[pos + 1] == '/') { ++pos; continue; }\n";
    out << "        size_t tag_end = xml.find('>', pos);\n";
    out << "        if (tag_end == std::string::npos) break;\n";
    out << "        std::string tag = xml.substr(pos, tag_end - pos + 1);\n";
    out << "        if (!TagNameMatches(tag, local_name)) { pos = tag_end + 1; continue; }\n";
    out << "        size_t name_begin = tag.find_first_not_of(\" \\t\\r\\n/<\");\n";
    out << "        size_t name_end = tag.find_first_of(\" \\t\\r\\n>/\", name_begin);\n";
    out << "        std::string full_name = tag.substr(name_begin, name_end == std::string::npos ? std::string::npos : name_end - name_begin);\n";
    out << "        std::string close = \"</\" + full_name + \">\";\n";
    out << "        size_t close_pos = xml.find(close, tag_end + 1);\n";
    out << "        if (close_pos == std::string::npos) throw std::runtime_error(\"SOAP response missing closing tag for \" + local_name);\n";
    out << "        return XmlUnescape(xml.substr(tag_end + 1, close_pos - tag_end - 1));\n";
    out << "    }\n";
    out << "    throw std::runtime_error(\"SOAP response missing tag: \" + local_name);\n}\n\n";

    out << "#if defined(_WIN32) && !defined(WSDLCPPGEN_USE_LIBCURL)\n";
    out << "std::wstring Widen(const std::string& value)\n{\n";
    out << "    if (value.empty()) return {};\n";
    out << "    int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);\n";
    out << "    std::wstring out(count, L'\\0');\n";
    out << "    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), count);\n";
    out << "    return out;\n}\n\n";
    out << "std::string SendSoapRequest(const std::string& endpoint, const std::string& soap_action, const std::string& body)\n{\n";
    out << "    std::wstring wide_endpoint = Widen(endpoint);\n";
    out << "    URL_COMPONENTS parts{};\n";
    out << "    parts.dwStructSize = sizeof(parts);\n";
    out << "    parts.dwSchemeLength = parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength = static_cast<DWORD>(-1);\n";
    out << "    if (!WinHttpCrackUrl(wide_endpoint.c_str(), 0, 0, &parts)) throw std::runtime_error(\"Invalid endpoint URL\");\n";
    out << "    std::wstring host(parts.lpszHostName, parts.dwHostNameLength);\n";
    out << "    std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);\n";
    out << "    if (parts.dwExtraInfoLength) path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);\n";
    out << "    HINTERNET session = WinHttpOpen(L\"WsdlCppGen/1.0\", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);\n";
    out << "    if (!session) throw std::runtime_error(\"WinHttpOpen failed\");\n";
    out << "    HINTERNET connect = WinHttpConnect(session, host.c_str(), parts.nPort, 0);\n";
    out << "    if (!connect) { WinHttpCloseHandle(session); throw std::runtime_error(\"WinHttpConnect failed\"); }\n";
    out << "    DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;\n";
    out << "    HINTERNET request = WinHttpOpenRequest(connect, L\"POST\", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);\n";
    out << "    if (!request) { WinHttpCloseHandle(connect); WinHttpCloseHandle(session); throw std::runtime_error(\"WinHttpOpenRequest failed\"); }\n";
    out << "    std::wstring headers = L\"Content-Type: text/xml; charset=utf-8\\r\\nSOAPAction: \\\"\" + Widen(soap_action) + L\"\\\"\\r\\n\";\n";
    out << "    BOOL ok = WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()), const_cast<char*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0);\n";
    out << "    if (ok) ok = WinHttpReceiveResponse(request, nullptr);\n";
    out << "    if (!ok) { WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session); throw std::runtime_error(\"WinHTTP request failed\"); }\n";
    out << "    DWORD status = 0, status_size = sizeof(status);\n";
    out << "    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX);\n";
    out << "    std::string response;\n";
    out << "    DWORD available = 0;\n";
    out << "    while (WinHttpQueryDataAvailable(request, &available) && available > 0)\n    {\n";
    out << "        std::string chunk(available, '\\0');\n";
    out << "        DWORD read = 0;\n";
    out << "        if (!WinHttpReadData(request, chunk.data(), available, &read)) break;\n";
    out << "        chunk.resize(read);\n";
    out << "        response += chunk;\n";
    out << "    }\n";
    out << "    WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session);\n";
    out << "    if (status < 200 || status >= 300) throw std::runtime_error(\"HTTP request failed with status \" + std::to_string(status) + \": \" + response);\n";
    out << "    return response;\n}\n";
    out << "#else\n";
    out << "size_t CurlWrite(char* data, size_t size, size_t nmemb, void* user_data)\n{\n";
    out << "    auto* response = static_cast<std::string*>(user_data);\n";
    out << "    response->append(data, size * nmemb);\n";
    out << "    return size * nmemb;\n}\n\n";
    out << "std::string SendSoapRequest(const std::string& endpoint, const std::string& soap_action, const std::string& body)\n{\n";
    out << "    CURL* curl = curl_easy_init();\n";
    out << "    if (!curl) throw std::runtime_error(\"curl_easy_init failed\");\n";
    out << "    std::string response;\n";
    out << "    struct curl_slist* headers = nullptr;\n";
    out << "    headers = curl_slist_append(headers, \"Content-Type: text/xml; charset=utf-8\");\n";
    out << "    std::string action_header = \"SOAPAction: \\\"\" + soap_action + \"\\\"\";\n";
    out << "    headers = curl_slist_append(headers, action_header.c_str());\n";
    out << "    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());\n";
    out << "    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);\n";
    out << "    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());\n";
    out << "    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));\n";
    out << "    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite);\n";
    out << "    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);\n";
    out << "    CURLcode code = curl_easy_perform(curl);\n";
    out << "    long status = 0;\n";
    out << "    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);\n";
    out << "    curl_slist_free_all(headers);\n";
    out << "    curl_easy_cleanup(curl);\n";
    out << "    if (code != CURLE_OK) throw std::runtime_error(std::string(\"curl request failed: \") + curl_easy_strerror(code));\n";
    out << "    if (status < 200 || status >= 300) throw std::runtime_error(\"HTTP request failed with status \" + std::to_string(status) + \": \" + response);\n";
    out << "    return response;\n}\n";
    out << "#endif\n\n";
    out << "} // namespace\n\n";

    out << class_name << "::" << class_name << "(std::string endpoint) : endpoint_(std::move(endpoint)) {}\n\n";
    out << "const std::string& " << class_name << "::Endpoint() const noexcept\n{\n    return endpoint_;\n}\n\n";

    for (const Operation& operation : model.operations)
    {
        const Element& input = model.elements.at(operation.input_element);
        const Element& output = model.elements.at(operation.output_element);
        out << output.cpp_name << " " << class_name << "::" << operation.cpp_name << "(const " << input.cpp_name << "& request) const\n{\n";
        out << "    std::ostringstream body;\n";
        out << "    body << \"<?xml version=\\\"1.0\\\" encoding=\\\"utf-8\\\"?>\";\n";
        out << "    body << \"<soap:Envelope xmlns:soap=\\\"http://schemas.xmlsoap.org/soap/envelope/\\\" xmlns:tns=\\\"" << model.target_namespace << "\\\">\";\n";
        out << "    body << \"<soap:Body><tns:" << input.xml_name << ">\";\n";
        for (const Field& field : input.fields)
        {
            out << "    body << \"<" << field.xml_name << ">\" << " << ToXmlValueExpression(field, "request." + field.cpp_name) << " << \"</" << field.xml_name << ">\";\n";
        }
        out << "    body << \"</tns:" << input.xml_name << "></soap:Body></soap:Envelope>\";\n";
        out << "    std::string xml = SendSoapRequest(endpoint_, " << CppStringLiteral(operation.soap_action) << ", body.str());\n";
        out << "    " << output.cpp_name << " response{};\n";
        for (const Field& field : output.fields)
        {
            const std::string temp = field.cpp_name + "_text";
            out << "    std::string " << temp << " = ExtractTagText(xml, " << CppStringLiteral(field.xml_name) << ");\n";
            out << "    response." << field.cpp_name << " = " << FromXmlExpression(field, temp) << ";\n";
        }
        out << "    return response;\n";
        out << "}\n\n";

        out << output.cpp_name << " " << class_name << "::" << operation.cpp_name << "(";
        for (size_t i = 0; i < input.fields.size(); ++i)
        {
            const Field& field = input.fields[i];
            if (i)
                out << ", ";
            if (field.cpp_type == "std::string")
                out << "const std::string& ";
            else
                out << field.cpp_type << " ";
            out << field.cpp_name;
        }
        out << ") const\n{\n";
        out << "    " << input.cpp_name << " request{};\n";
        for (const Field& field : input.fields)
            out << "    request." << field.cpp_name << " = " << field.cpp_name << ";\n";
        out << "    return " << operation.cpp_name << "(request);\n";
        out << "}\n\n";
    }

    out << "} // namespace GeneratedWsdl\n";
    return out.str();
}
}

GenerateResult GenerateFromWsdlFile(const GenerateOptions& options)
{
    if (options.wsdl_path.empty())
        throw std::runtime_error("Missing WSDL path");
    if (options.output_dir.empty())
        throw std::runtime_error("Missing output directory");

    std::string xml = ReadFile(options.wsdl_path);
    WsdlModel model = ParseWsdl(xml, options.service_name_override);

    std::filesystem::create_directories(options.output_dir);
    const std::string service_id = ToIdentifier(model.service_name, true);
    std::filesystem::path header_path = options.output_dir / (service_id + "Client.h");
    std::filesystem::path source_path = options.output_dir / (service_id + "Client.cpp");

    WriteFile(header_path, GenerateHeader(model));
    WriteFile(source_path, GenerateSource(model));

    GenerateResult result;
    result.header_path = header_path;
    result.source_path = source_path;
    result.service_name = model.service_name;
    for (const Operation& operation : model.operations)
        result.operations.push_back(operation.name);
    return result;
}
}
