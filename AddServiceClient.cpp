#include "AddServiceClient.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(_WIN32) && !defined(WSDLCPPGEN_USE_LIBCURL)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#if defined(_MSC_VER)
#pragma comment(lib, "winhttp.lib")
#endif
#else
#include <curl/curl.h>
#endif

namespace GeneratedWsdl
{
namespace
{
std::string XmlEscape(const std::string& value)
{
    std::string out;
    for (char ch : value)
    {
        switch (ch)
        {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '\"': out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default: out += ch; break;
        }
    }
    return out;
}

std::string XmlUnescape(std::string value)
{
    const std::pair<const char*, const char*> replacements[] = {{"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}, {"&amp;", "&"}};
    for (const auto& [from, to] : replacements)
    {
        size_t pos = 0;
        while ((pos = value.find(from, pos)) != std::string::npos)
        {
            value.replace(pos, std::char_traits<char>::length(from), to);
            pos += std::char_traits<char>::length(to);
        }
    }
    return value;
}

bool TagNameMatches(const std::string& tag, const std::string& local_name)
{
    size_t begin = tag.find_first_not_of(" \t\r\n/<");
    if (begin == std::string::npos) return false;
    size_t end = tag.find_first_of(" \t\r\n>/", begin);
    std::string name = tag.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    size_t colon = name.rfind(':');
    if (colon != std::string::npos) name = name.substr(colon + 1);
    return name == local_name;
}

std::string ExtractTagText(const std::string& xml, const std::string& local_name)
{
    size_t pos = 0;
    while ((pos = xml.find('<', pos)) != std::string::npos)
    {
        if (pos + 1 < xml.size() && xml[pos + 1] == '/') { ++pos; continue; }
        size_t tag_end = xml.find('>', pos);
        if (tag_end == std::string::npos) break;
        std::string tag = xml.substr(pos, tag_end - pos + 1);
        if (!TagNameMatches(tag, local_name)) { pos = tag_end + 1; continue; }
        size_t name_begin = tag.find_first_not_of(" \t\r\n/<");
        size_t name_end = tag.find_first_of(" \t\r\n>/", name_begin);
        std::string full_name = tag.substr(name_begin, name_end == std::string::npos ? std::string::npos : name_end - name_begin);
        std::string close = "</" + full_name + ">";
        size_t close_pos = xml.find(close, tag_end + 1);
        if (close_pos == std::string::npos) throw std::runtime_error("SOAP response missing closing tag for " + local_name);
        return XmlUnescape(xml.substr(tag_end + 1, close_pos - tag_end - 1));
    }
    throw std::runtime_error("SOAP response missing tag: " + local_name);
}

#if defined(_WIN32) && !defined(WSDLCPPGEN_USE_LIBCURL)
std::wstring Widen(const std::string& value)
{
    if (value.empty()) return {};
    int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring out(count, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), count);
    return out;
}

std::string SendSoapRequest(const std::string& endpoint, const std::string& soap_action, const std::string& body)
{
    std::wstring wide_endpoint = Widen(endpoint);
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wide_endpoint.c_str(), 0, 0, &parts)) throw std::runtime_error("Invalid endpoint URL");
    std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength) path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    HINTERNET session = WinHttpOpen(L"WsdlCppGen/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) throw std::runtime_error("WinHttpOpen failed");
    HINTERNET connect = WinHttpConnect(session, host.c_str(), parts.nPort, 0);
    if (!connect) { WinHttpCloseHandle(session); throw std::runtime_error("WinHttpConnect failed"); }
    DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) { WinHttpCloseHandle(connect); WinHttpCloseHandle(session); throw std::runtime_error("WinHttpOpenRequest failed"); }
    std::wstring headers = L"Content-Type: text/xml; charset=utf-8\r\nSOAPAction: \"" + Widen(soap_action) + L"\"\r\n";
    BOOL ok = WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()), const_cast<char*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0);
    if (ok) ok = WinHttpReceiveResponse(request, nullptr);
    if (!ok) { WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session); throw std::runtime_error("WinHTTP request failed"); }
    DWORD status = 0, status_size = sizeof(status);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX);
    std::string response;
    DWORD available = 0;
    while (WinHttpQueryDataAvailable(request, &available) && available > 0)
    {
        std::string chunk(available, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(request, chunk.data(), available, &read)) break;
        chunk.resize(read);
        response += chunk;
    }
    WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session);
    if (status < 200 || status >= 300) throw std::runtime_error("HTTP request failed with status " + std::to_string(status) + ": " + response);
    return response;
}
#else
size_t CurlWrite(char* data, size_t size, size_t nmemb, void* user_data)
{
    auto* response = static_cast<std::string*>(user_data);
    response->append(data, size * nmemb);
    return size * nmemb;
}

std::string SendSoapRequest(const std::string& endpoint, const std::string& soap_action, const std::string& body)
{
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");
    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: text/xml; charset=utf-8");
    std::string action_header = "SOAPAction: \"" + soap_action + "\"";
    headers = curl_slist_append(headers, action_header.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode code = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (code != CURLE_OK) throw std::runtime_error(std::string("curl request failed: ") + curl_easy_strerror(code));
    if (status < 200 || status >= 300) throw std::runtime_error("HTTP request failed with status " + std::to_string(status) + ": " + response);
    return response;
}
#endif

} // namespace

AddServiceClient::AddServiceClient(std::string endpoint) : endpoint_(std::move(endpoint)) {}

const std::string& AddServiceClient::Endpoint() const noexcept
{
    return endpoint_;
}

AddResponse AddServiceClient::Add(const AddRequest& request) const
{
    std::ostringstream body;
    body << "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
    body << "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:tns=\"http://example.com/addservice\">";
    body << "<soap:Body><tns:AddRequest>";
    body << "<num1>" << std::to_string(request.num1) << "</num1>";
    body << "<num2>" << std::to_string(request.num2) << "</num2>";
    body << "</tns:AddRequest></soap:Body></soap:Envelope>";
    std::string xml = SendSoapRequest(endpoint_, "http://example.com/addservice/Add", body.str());
    AddResponse response{};
    std::string result_text = ExtractTagText(xml, "result");
    response.result = std::stoi(result_text);
    return response;
}

AddResponse AddServiceClient::Add(int num1, int num2) const
{
    AddRequest request{};
    request.num1 = num1;
    request.num2 = num2;
    return Add(request);
}

} // namespace GeneratedWsdl
