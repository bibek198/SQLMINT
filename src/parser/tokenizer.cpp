#include "tokenizer.h"
#include <cctype>
#include <stdexcept>

namespace sqldb {

// Keywords mapping
const std::unordered_map<std::string, TokenType> Tokenizer::keywords = {
    {"CREATE", TokenType::CREATE},
    {"DROP", TokenType::DROP},
    {"TABLE", TokenType::TABLE},
    {"INSERT", TokenType::INSERT},
    {"INTO", TokenType::INTO},
    {"SELECT", TokenType::SELECT},
    {"FROM", TokenType::FROM},
    {"WHERE", TokenType::WHERE},
    {"VALUES", TokenType::VALUES},
    {"INTEGER", TokenType::INTEGER},
    {"VARCHAR", TokenType::VARCHAR},
    {"BOOLEAN", TokenType::BOOLEAN},
    {"PRIMARY", TokenType::PRIMARY},
    {"KEY", TokenType::KEY},
    {"NOT", TokenType::NOT},
    {"NULL", TokenType::NULL_KEYWORD},
    {"TRUE", TokenType::BOOLEAN_LITERAL},
    {"FALSE", TokenType::BOOLEAN_LITERAL}
};

Tokenizer::Tokenizer(const std::string& input) 
    : input(input), current_pos(0), line(1), column(1) {}

char Tokenizer::peek() const {
    if (current_pos >= input.length()) {
        return '\0';
    }
    return input[current_pos];
}

char Tokenizer::advance() {
    if (current_pos >= input.length()) {
        return '\0';
    }
    
    char c = input[current_pos++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

void Tokenizer::skip_whitespace() {
    while (std::isspace(peek())) {
        advance();
    }
}

void Tokenizer::skip_comment() {
    if (peek() == '-' && current_pos + 1 < input.length() && input[current_pos + 1] == '-') {
        // Skip until end of line
        while (peek() != '\n' && peek() != '\0') {
            advance();
        }
    }
}

bool Tokenizer::is_alpha(char c) const {
    return std::isalpha(c) || c == '_';
}

bool Tokenizer::is_digit(char c) const {
    return std::isdigit(c);
}

bool Tokenizer::is_alnum(char c) const {
    return std::isalnum(c) || c == '_';
}

Token Tokenizer::read_identifier() {
    int start_line = line;
    int start_column = column;
    std::string value;
    
    while (is_alnum(peek())) {
        value += advance();
    }
    
    // Convert to uppercase for keyword comparison
    std::string upper_value = value;
    for (char& c : upper_value) {
        c = std::toupper(c);
    }
    
    // Check if it's a keyword
    auto it = keywords.find(upper_value);
    if (it != keywords.end()) {
        return Token(it->second, upper_value, start_line, start_column);
    }
    
    return Token(TokenType::IDENTIFIER, value, start_line, start_column);
}

Token Tokenizer::read_number() {
    int start_line = line;
    int start_column = column;
    std::string value;
    
    while (is_digit(peek())) {
        value += advance();
    }
    
    return Token(TokenType::INTEGER_LITERAL, value, start_line, start_column);
}

Token Tokenizer::read_string() {
    int start_line = line;
    int start_column = column;
    std::string value;
    
    // Skip opening quote
    advance();
    
    while (peek() != '\'' && peek() != '\0') {
        if (peek() == '\\') {
            advance(); // Skip escape character
            char escaped = advance();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '\'': value += '\''; break;
                default: value += escaped; break;
            }
        } else {
            value += advance();
        }
    }
    
    if (peek() == '\'') {
        advance(); // Skip closing quote
    } else {
        throw std::runtime_error("Unterminated string literal");
    }
    
    return Token(TokenType::STRING_LITERAL, value, start_line, start_column);
}

Token Tokenizer::read_operator() {
    int start_line = line;
    int start_column = column;
    char c = advance();
    
    switch (c) {
        case '=':
            return Token(TokenType::EQUALS, "=", start_line, start_column);
        case '!':
            if (peek() == '=') {
                advance();
                return Token(TokenType::NOT_EQUALS, "!=", start_line, start_column);
            }
            break;
        case '<':
            if (peek() == '=') {
                advance();
                return Token(TokenType::LESS_EQUAL, "<=", start_line, start_column);
            } else if (peek() == '>') {
                advance();
                return Token(TokenType::NOT_EQUALS, "<>", start_line, start_column);
            }
            return Token(TokenType::LESS_THAN, "<", start_line, start_column);
        case '>':
            if (peek() == '=') {
                advance();
                return Token(TokenType::GREATER_EQUAL, ">=", start_line, start_column);
            }
            return Token(TokenType::GREATER_THAN, ">", start_line, start_column);
    }
    
    return Token(TokenType::UNKNOWN, std::string(1, c), start_line, start_column);
}

Token Tokenizer::next_token() {
    skip_whitespace();
    skip_comment();
    skip_whitespace();
    
    if (current_pos >= input.length()) {
        return Token(TokenType::END_OF_FILE, "", line, column);
    }
    
    char c = peek();
    int start_line = line;
    int start_column = column;
    
    // Identifiers and keywords
    if (is_alpha(c)) {
        return read_identifier();
    }
    
    // Numbers
    if (is_digit(c)) {
        return read_number();
    }
    
    // String literals
    if (c == '\'') {
        return read_string();
    }
    
    // Operators
    if (c == '=' || c == '!' || c == '<' || c == '>') {
        return read_operator();
    }
    
    // Single character tokens
    advance();
    switch (c) {
        case ';': return Token(TokenType::SEMICOLON, ";", start_line, start_column);
        case ',': return Token(TokenType::COMMA, ",", start_line, start_column);
        case '(': return Token(TokenType::LEFT_PAREN, "(", start_line, start_column);
        case ')': return Token(TokenType::RIGHT_PAREN, ")", start_line, start_column);
        case '*': return Token(TokenType::ASTERISK, "*", start_line, start_column);
        default:
            return Token(TokenType::UNKNOWN, std::string(1, c), start_line, start_column);
    }
}

std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;
    
    while (true) {
        Token token = next_token();
        tokens.push_back(token);
        if (token.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    
    return tokens;
}

std::string Tokenizer::token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";
        case TokenType::CREATE: return "CREATE";
        case TokenType::DROP: return "DROP";
        case TokenType::TABLE: return "TABLE";
        case TokenType::INSERT: return "INSERT";
        case TokenType::INTO: return "INTO";
        case TokenType::SELECT: return "SELECT";
        case TokenType::FROM: return "FROM";
        case TokenType::WHERE: return "WHERE";
        case TokenType::VALUES: return "VALUES";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::VARCHAR: return "VARCHAR";
        case TokenType::BOOLEAN: return "BOOLEAN";
        case TokenType::PRIMARY: return "PRIMARY";
        case TokenType::KEY: return "KEY";
        case TokenType::NOT: return "NOT";
        case TokenType::NULL_KEYWORD: return "NULL";
        case TokenType::EQUALS: return "EQUALS";
        case TokenType::NOT_EQUALS: return "NOT_EQUALS";
        case TokenType::LESS_THAN: return "LESS_THAN";
        case TokenType::GREATER_THAN: return "GREATER_THAN";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::ASTERISK: return "ASTERISK";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        default: return "UNKNOWN";
    }
}

bool Tokenizer::is_keyword(const std::string& word) {
    std::string upper_word = word;
    for (char& c : upper_word) {
        c = std::toupper(c);
    }
    return keywords.find(upper_word) != keywords.end();
}

TokenType Tokenizer::get_keyword_type(const std::string& word) {
    std::string upper_word = word;
    for (char& c : upper_word) {
        c = std::toupper(c);
    }
    auto it = keywords.find(upper_word);
    return (it != keywords.end()) ? it->second : TokenType::UNKNOWN;
}

} // namespace sqldb
