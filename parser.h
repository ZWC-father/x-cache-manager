#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include <nlohmann/json.hpp>

class Parser{
public:
    struct UnknownType{bool exist; std::string value;};
    using LogValue = std::variant<std::nullptr_t, bool, int64_t, uint64_t, double, std::string, UnknownType>; 
    //TODO: remove int64_t (since nginx only provide unsigned int. To use the proper type, it must be removed)
    // the last is for unknown type
    Parser(const std::vector<std::string>& keys);
    std::vector<LogValue> parse(std::string data_raw);

private:
    using json = nlohmann::json;
    const std::vector<std::string> json_keys;
    
};
