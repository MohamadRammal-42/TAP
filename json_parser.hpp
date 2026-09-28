#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

struct JsonValue;

using JsonObject = std::unordered_map<std::string, JsonValue>;

using JsonArray = std::vector<JsonValue>;

struct JsonValue {
    using Value = std::variant<
        std::nullptr_t,
        bool,
        double,
        int,
        std::string,
        JsonArray,
        JsonObject
    >;

    Value value;

    JsonValue() : value(nullptr) {}
    JsonValue(std::nullptr_t v) : value(v) {}
    JsonValue(bool v) : value(v) {}
    JsonValue(double v) : value(v) {}
    JsonValue(int v) : value(v) {}
    JsonValue(const std::string& v) : value(v) {}
    JsonValue(const char* v) : value(std::string(v)) {}
    JsonValue(const JsonArray& v) : value(v) {}
    JsonValue(const JsonObject& v) : value(v) {}

    JsonValue& operator[](const std::string& key) {
    return std::get<JsonObject>(value)[key];
    }

    JsonValue& operator[](size_t index) {
        return std::get<JsonArray>(value).at(index);
    }

    bool isNull() const {
        return std::holds_alternative<std::nullptr_t>(value);
    }

    void push_back(const JsonValue& v) {
        std::get<JsonArray>(value).push_back(v);
    }

    void erase(size_t index) {
        std::get<JsonArray>(value).erase(
            std::get<JsonArray>(value).begin() + index
        );
    }

    bool contains(const std::string& key) const {
        if (!std::holds_alternative<JsonObject>(value))
            return false;

        const JsonObject& object = std::get<JsonObject>(value);
        return object.find(key) != object.end();
    }

    std::string getString() const {
        return std::get<std::string>(value);
    }

    double getDouble() const {
        return std::get<double>(value);
    }

    bool isArray() const {
        return std::holds_alternative<JsonArray>(value);
    }

    size_t size() const {
        return std::get<JsonArray>(value).size();
    }


};

class JsonParser {
private:
    const std::string& input;
    size_t pos = 0;

    bool atEnd() const;
    char current() const;
    void advance();
    void skipWhitespace();
    void expect(char expected);
    JsonValue parseTrue();
    JsonValue parseFalse();
    JsonValue parseNull();
    std::string parseString();
    JsonValue parseNumber();
    JsonValue parseArray();
    JsonValue parseObject();
    JsonValue parseValue();

public:

    JsonParser(const std::string& input);
    JsonValue parse();

};

void printJson(const JsonValue& json, int indent = 0);
std::string ReturnJson(const JsonValue& json, int indent = 0);
