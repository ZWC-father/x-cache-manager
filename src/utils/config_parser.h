#include <optional>
#include <iostream>
#include <stdexcept>
#include <regex>
#include <sstream>
#include <typeinfo>
#include <cxxabi.h>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

class ConfigError : public std::runtime_error{
public:
    explicit ConfigError(const std::string& err) : std::runtime_error(err) {}
};

class ConfigParser{
public:
    ConfigParser(const std::string& conf_file);

    template<typename T>
    T get_required(const std::string& path) const{
        auto node = find(path);
        if(!node.has_value())throw ConfigError("path missing: " + path);
        try{
            return node.value().as<T>();
        }catch(const YAML::BadConversion& e){
            throw ConfigError(std::string("type mismatch for '") + path + "': cannot convert to ["
                              + demangle(typeid(T).name()) + "]");
        }catch(const std::runtime_error& e){
            throw ConfigError(std::string("unknown error: ") + e.what());
        }
    }
    
    template<typename T>
    T get_optional(const std::string& path, const T& default_val) const{
        auto node = find(path);
        if(!node.has_value())return default_val;
        try{
            return node.value().as<T>();
        }catch(const YAML::BadConversion& e){
            throw ConfigError(std::string("type mismatch: can not convert ") + path +
                                     " to [" + demangle(typeid(T).name()) + "]");
        }catch(const std::runtime_error& e){
            throw ConfigError(std::string("unknown error: ") + e.what());
        }
    }

    template<typename T>
    std::vector<T> get_array(const std::string& path) const{
        auto node = find(path);
        if(!node.has_value())throw std::runtime_error("array path missing: " + path);
        if(!node.value().IsSequence())throw std::runtime_error("type mismatch for path '" + path + "': expected a sequence");
        std::vector<T> v;
        for(auto it : node.value()){
            try{
                v.push_back(it.as<T>());
            }catch(const YAML::BadConversion& e){
                throw ConfigError("type mismatch in sequence for '" + path +
                                         "': cannot convert to [" + demangle(typeid(T).name()) + "]");
            }catch(const std::runtime_error& e){
                throw ConfigError(std::string("unknown error: ") + e.what());
            }
        }
        return v;
    }

private:
    YAML::Node root;

    std::optional<YAML::Node> find(const std::string& path) const;
    static std::string demangle(const char* name);
    
};
