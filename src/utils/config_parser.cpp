#include "config_parser.h"
#include <optional>
#include <yaml-cpp/node/node.h>

ConfigParser::ConfigParser(const std::string& conf_file){
    try{
        root = YAML::LoadFile(conf_file);
    }catch(const YAML::BadFile& err){
        throw ConfigError("failed to open config file: " + err.msg);
    }catch(const YAML::ParserException& err){
        throw ConfigError("failed to parse config file: " + err.msg);
    }catch(const std::runtime_error& err){
        throw ConfigError(std::string("parser unknown error: ") + err.what());
    }
}

std::optional<YAML::Node> ConfigParser::find(const std::string& path) const{
    std::stringstream ss(path);
    std::string segment;
    auto node = YAML::Clone(root);
    while(std::getline(ss, segment, '.')){
        if(!node[segment].IsDefined()){
            return std::nullopt;
        }
        node = node[segment];
    }
    return node;
}


std::string ConfigParser::demangle(const char* name){
    std::string pretty;

    int status = 0;
    char* realname = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    pretty = (status == 0 && realname) ? realname : name;
    std::free(realname);

    static const std::regex cxx11_ns(R"(std::__cxx11::)");
    pretty = std::regex_replace(pretty, cxx11_ns, "std::");

    static const std::unordered_map<std::string, std::string> aliasMap = {
        { R"(std::basic_string<char, std::char_traits<char>, std::allocator<char> >)", "std::string"  },
        { R"(std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >)", "std::wstring" },
        { R"(std::basic_string<char16_t, std::char_traits<char16_t>, std::allocator<char16_t> >)", "std::u16string" },
        { R"(std::basic_string<char32_t, std::char_traits<char32_t>, std::allocator<char32_t> >)", "std::u32string" }
    };

    auto it = aliasMap.find(pretty);
    if(it != aliasMap.end())pretty = it->second;

    return pretty;
}
