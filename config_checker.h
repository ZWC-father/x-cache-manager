#include "config_parser.h"
#include <regex>
#include <unordered_map>
#include <cctype>

namespace config_checks {

struct string_rule {
    size_t min_length;
    size_t max_length;
    bool trim_whitespace; // if true, disallow leading/trailing spaces
};

struct path_rule {
    std::regex pattern;
};

struct host_rule {
    bool allow_localhost;
    std::regex ipv4_pattern;
    std::regex ipv6_pattern;
};

struct port_rule {
    int min_port;
    int max_port;
};

struct rules {
    std::unordered_map<std::string, string_rule> string_rules;
    std::unordered_map<std::string, path_rule> path_rules;
    std::unordered_map<std::string, host_rule> host_rules;
    std::unordered_map<std::string, port_rule> port_rules;
};

// Example: default rules instantiation
inline rules get_default_rules() {
    rules r;
    // String rule: app.name (length 1-50, no leading/trailing spaces)
    r.string_rules["app.name"] = {1, 50, true};
    // Path rule: sqlite.path (absolute or relative Linux path)
    r.path_rules["sqlite.path"] = {std::regex(R"(^(/[^/\s]+)*(/)?$)")};
    // Host rule: redis.host
    r.host_rules["redis.host"] = {
        true,
        std::regex(R"(^((25[0-5]|2[0-4]\d|[01]?\d?\d)\.){3}(25[0-5]|2[0-4]\d|[01]?\d?\d)$)"),  // IPv4
        std::regex(R"(^\[?[0-9a-fA-F:]+\]?$)")  // IPv6 (simple check)
    };
    // Port rule: redis.port
    r.port_rules["redis.port"] = {1, 65535};

    return r;
}

} // namespace config_checks


// ===============================
// ConfigChecker Class Definition
// ===============================

class ConfigChecker : public ConfigParser {
public:
    // Constructs parser and sets validation rules
    ConfigChecker(const std::string& conf_file,
                  const config_checks::rules& rules)
        : ConfigParser(conf_file), rules_(rules) {}

    // Validate all parameters according to rules
    void validate() const {
        for (const auto& p : rules_.string_rules) {
            check_string(p.first, p.second);
        }
        for (const auto& p : rules_.path_rules) {
            check_path(p.first, p.second);
        }
        for (const auto& p : rules_.host_rules) {
            check_host(p.first, p.second);
        }
        for (const auto& p : rules_.port_rules) {
            check_port(p.first, p.second);
        }
    }

private:
    config_checks::rules rules_;

    // Check string length and whitespace
    void check_string(const std::string& path,
                      const config_checks::string_rule& rule) const {
        std::string val = get_required<std::string>(path);
        size_t len = val.size();
        if (len < rule.min_length || len > rule.max_length) {
            throw ConfigError("string length for '" + path + "' out of range");
        }
        if (rule.trim_whitespace) {
            if (std::isspace(static_cast<unsigned char>(val.front())) ||
                std::isspace(static_cast<unsigned char>(val.back()))) {
                throw ConfigError("string '" + path + "' has leading or trailing whitespace");
            }
        }
    }

    // Check Linux-style path with regex
    void check_path(const std::string& path,
                    const config_checks::path_rule& rule) const {
        std::string val = get_required<std::string>(path);
        if (!std::regex_match(val, rule.pattern)) {
            throw ConfigError("path '" + path + "' does not match Linux path format");
        }
    }

    // Check hostname (localhost, IPv4, or IPv6)
    void check_host(const std::string& path,
                    const config_checks::host_rule& rule) const {
        std::string val = get_required<std::string>(path);
        if (rule.allow_localhost && val == "localhost") {
            return;
        }
        if (!std::regex_match(val, rule.ipv4_pattern) &&
            !std::regex_match(val, rule.ipv6_pattern)) {
            throw ConfigError("host '" + path + "' is not valid localhost, IPv4, or IPv6");
        }
    }

    // Check port range
    void check_port(const std::string& path,
                    const config_checks::port_rule& rule) const {
        int val = get_required<int>(path);
        if (val < rule.min_port || val > rule.max_port) {
            throw ConfigError("port '" + path + "' out of range");
        }
    }
};
