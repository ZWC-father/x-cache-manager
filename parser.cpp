#include "parser.h"

Parser::Parser(const std::vector<std::string>& keys) : json_keys(keys){
    if(json_keys.empty())throw std::runtime_error("parser: invalid args");
}

std::vector<Parser::LogValue> Parser::parse(std::string data_raw){
    size_t ptr = data_raw.find('{');
    if(ptr == data_raw.size())throw std::runtime_error("parser: invalid json");
    data_raw = data_raw.substr(ptr);
    
    json json_parsed;
    
    try{
        json_parsed = json::parse(data_raw);
    }catch(json::parse_error& e){
        throw std::runtime_error(std::string("parser: fail to parse json: ") + e.what());
    }

    std::vector<LogValue> values;

    for(auto& keys:json_keys){
        if(json_parsed.contains(keys)){
            const auto& val = json_parsed[keys];
            if(val.is_null()){
                values.emplace_back(nullptr);
            }else if(val.is_boolean()){
                values.emplace_back(val.get<bool>());
            }else if(val.is_number_integer()){
                values.emplace_back(val.get<int64_t>());//TODO: add support for BigInt
            }else if(val.is_number_unsigned()){
                values.emplace_back(val.get<uint64_t>());
            }else if(val.is_number_float()){
                values.emplace_back(val.get<double>());
            }else if(val.is_string()){
                values.emplace_back(val.get<std::string>());
            }else{
                values.emplace_back((UnknownType){1, val.dump()});
            }
        }else{
            values.emplace_back((UnknownType){0, ""});
        }
    }

    return values;
}

