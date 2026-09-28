#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <stdexcept>
#include <sstream>
#include <cctype>
#include "json_parser.hpp"

bool JsonParser::atEnd() const {
    return pos >= input.size();
}

char JsonParser::current() const {
    if (atEnd()) {
        throw std::runtime_error("Unexpected end of input");
    }
    return input[pos];
}

void JsonParser::advance() {
    if (!atEnd()) {
        pos++;
    }
}

void JsonParser::skipWhitespace() {
    while (!atEnd() && std::isspace(static_cast<unsigned char>(current()))) {
        advance();
    }
}

void JsonParser::expect(char expected) {
    skipWhitespace();

    if (atEnd() || current() != expected) {
        throw std::runtime_error("Expected '" + std::string(1, expected) + "' at position " + std::to_string(pos));
    }
    advance();
}

JsonValue JsonParser::parseTrue() {
    if (input.substr(pos, 4) != "true") {
        throw std::runtime_error("Expected 'true' at position " + std::to_string(pos));
    }
    pos += 4;
    return JsonValue(true);
}

JsonValue JsonParser::parseFalse() {
    if (input.substr(pos, 5) != "false") {
        throw std::runtime_error("Expected 'false' at position " + std::to_string(pos));
    }
    pos += 5;
    return JsonValue(false);
}

JsonValue JsonParser::parseNull() {
    if (input.substr(pos, 4) != "null") {
        throw std::runtime_error("Expected 'null' at position " + std::to_string(pos));
    }
    pos += 4;
    return JsonValue(nullptr);
}

std::string JsonParser::parseString() {
    skipWhitespace();
    if (atEnd() || current() != '"') {
        throw std::runtime_error("Expected string at position " + std::to_string(pos));
    }
    advance();
    std::string result;
    while (!atEnd()) {
        char c = current();
        if (c == '"') {
            advance();
            return result;
        }
        if (c == '\\') {
            advance();
            if (atEnd()) {
                throw std::runtime_error("Unterminated escape sequence");
            }
            char escaped = current();
            switch (escaped) {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '/':
                    result += '/';
                    break;
                case 'b':
                    result += '\b';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                default:
                    throw std::runtime_error("Invalid escape sequence at position " + std::to_string(pos));
            }
            advance();
            continue;
        }
        result += c;
        advance();
    }
    throw std::runtime_error("Unterminated string");
}

JsonValue JsonParser::parseNumber() {
    skipWhitespace();
    size_t start = pos;
    if (current() == '-') {
        advance();
    }
    if (atEnd() || !std::isdigit(static_cast<unsigned char>(current()))) {
        throw std::runtime_error("Invalid number at position " +std::to_string(pos));
    }
    while (!atEnd() && std::isdigit(static_cast<unsigned char>(current()))) {
        advance();
    }
    if (!atEnd() && current() == '.') {
        advance();
        if (atEnd() || !std::isdigit(static_cast<unsigned char>(current()))) {
            throw std::runtime_error("Invalid decimal number at position " + std::to_string(pos));
        }
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(current()))) {
            advance();
        }
    }
    if (!atEnd() && (current() == 'e' || current() == 'E')) {
        advance();
        if (!atEnd() && (current() == '+' || current() == '-')) {
            advance();
        }
        if (atEnd() || !std::isdigit(static_cast<unsigned char>(current()))) {
            throw std::runtime_error("Invalid exponent at position " + std::to_string(pos));
        }
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(current()))) {
            advance();
        }
    }
    std::string numberText = input.substr(start, pos - start);
    try {
        double number = std::stod(numberText);
        return JsonValue(number);
    }
    catch (...) {
        throw std::runtime_error(
            "Invalid number: " + numberText
        );
    }
}

JsonValue JsonParser::parseArray() {
    expect('[');
    JsonArray array;
    skipWhitespace();
    if (!atEnd() && current() == ']') {
        advance();
        return JsonValue(array);
    }
    while (true) {
        JsonValue value = parseValue();
        array.push_back(value);
        skipWhitespace();
        if (!atEnd() && current() == ']') {
            advance();
            break;
        }
        if (atEnd() || current() != ',') {
            throw std::runtime_error("Expected ',' or ']' at position " + std::to_string(pos));
        }
        advance();
        skipWhitespace();
    }
    return JsonValue(array);
}

JsonValue JsonParser::parseObject() {
    expect('{');
    JsonObject object;
    skipWhitespace();
    if (!atEnd() && current() == '}') {
        advance();
        return JsonValue(object);
    }
    while (true) {
        skipWhitespace();
        if (atEnd() || current() != '"') {
            throw std::runtime_error("Expected object key at position " + std::to_string(pos));
        }
        std::string key = parseString();
        skipWhitespace();
        expect(':');
        JsonValue value = parseValue();
        object[key] = value;
        skipWhitespace();
        if (!atEnd() && current() == '}') {
            advance();
            break;
        }
        if (atEnd() || current() != ',') {
            throw std::runtime_error("Expected ',' or '}' at position " + std::to_string(pos));
        }
        advance();
        skipWhitespace();
    }
    return JsonValue(object);
}

JsonValue JsonParser::parseValue() {
    skipWhitespace();
    if (atEnd()) {
        throw std::runtime_error("Unexpected end of input");
    }
    char c = current();
    if (c == '{') {
        return parseObject();
    }
    if (c == '[') {
        return parseArray();
    }
    if (c == '"') {
        return JsonValue(parseString());
    }
    if (c == 't') {
        return parseTrue();
    }
    if (c == 'f') {
        return parseFalse();
    }
    if (c == 'n') {
        return parseNull();
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
        return parseNumber();
    }
    throw std::runtime_error("Unexpected character '" + std::string(1, c) + "' at position " + std::to_string(pos));
}

JsonParser::JsonParser(const std::string& input)
    : input(input) {}

JsonValue JsonParser::parse() {
    JsonValue result = parseValue();
    skipWhitespace();
    if (!atEnd()) {
        throw std::runtime_error("Unexpected characters after JSON at position " + std::to_string(pos));
    }
    return result;
}

void printJson(const JsonValue& json, int indent) {
    std::string spaces(indent, ' ');

    if (std::holds_alternative<std::nullptr_t>(json.value)) {
        std::cout << "null";
    }

    else if (std::holds_alternative<bool>(json.value)) {
        std::cout << std::boolalpha
                  << std::get<bool>(json.value);
    }

    else if (std::holds_alternative<double>(json.value)) {
        std::cout << std::get<double>(json.value);
    }

    else if (std::holds_alternative<int>(json.value)) {
        std::cout << std::get<int>(json.value);
    }

    else if (std::holds_alternative<std::string>(json.value)) {
        std::cout << '"' 
                  << std::get<std::string>(json.value)
                  << '"';
    }

    else if (std::holds_alternative<JsonArray>(json.value)) {
        const JsonArray& array =
            std::get<JsonArray>(json.value);

        std::cout << "[\n";

        for (size_t i = 0; i < array.size(); i++) {
            std::cout << std::string(indent + 4, ' ');

            printJson(array[i], indent + 4);

            if (i < array.size() - 1)
                std::cout << ",";

            std::cout << "\n";
        }

        std::cout << spaces << "]";
    }

    else if (std::holds_alternative<JsonObject>(json.value)) {
        const JsonObject& object =
            std::get<JsonObject>(json.value);

        std::cout << "{\n";

        size_t count = 0;

        for (const auto& [key, value] : object) {

            std::cout << std::string(indent + 4, ' ')
                      << '"'
                      << key
                      << "\": ";

            printJson(value, indent + 4);

            if (count < object.size() - 1)
                std::cout << ",";

            std::cout << "\n";

            count++;
        }
        std::cout << spaces << "}";
    }
}

std::string ReturnJson(const JsonValue& json, int indent) {
    std::string spaces(indent, ' ');

    if (std::holds_alternative<std::nullptr_t>(json.value)) {
        return "null";
    }

    else if (std::holds_alternative<int>(json.value)) {
    return std::to_string(std::get<int>(json.value));
    }

    else if (std::holds_alternative<bool>(json.value)) {
        return std::get<bool>(json.value) ? "true" : "false";
    }

    else if (std::holds_alternative<double>(json.value)) {
        return std::to_string(std::get<double>(json.value));
    }

    else if (std::holds_alternative<std::string>(json.value)) {
        return "\"" + std::get<std::string>(json.value) + "\"";
    }

    else if (std::holds_alternative<JsonArray>(json.value)) {
        const JsonArray& array =
            std::get<JsonArray>(json.value);

        std::string result = "[\n";

        for (size_t i = 0; i < array.size(); ++i) {

            result += std::string(indent + 4, ' ');

            result += ReturnJson(array[i], indent + 4);

            if (i < array.size() - 1)
                result += ",";

            result += "\n";
        }

        result += spaces + "]";

        return result;
    }

    else if (std::holds_alternative<JsonObject>(json.value)) {
        const JsonObject& object =
            std::get<JsonObject>(json.value);

        std::string result = "{\n";

        size_t count = 0;

        for (const auto& [key, value] : object) {

            result += std::string(indent + 4, ' ');
            result += "\"" + key + "\": ";

            result += ReturnJson(value, indent + 4);

            if (count < object.size() - 1)
                result += ",";

            result += "\n";

            ++count;
        }

        result += spaces + "}";

        return result;
    }

    return "";
}
