#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "../common/types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace sqldb {

class Tokenizer {
private:
    std::string input;
    size_t current_pos;
    int line;
    int column;
    
    // Keywords map
    static const std::unordered_map<std::string, TokenType> keywords;
    
    // Helper methods
    char peek() const;
    char advance();
    void skip_whitespace();
    void skip_comment();
    bool is_alpha(char c) const;
    bool is_digit(char c) const;
    bool is_alnum(char c) const;
    
    Token read_identifier();
    Token read_number();
    Token read_string();
    Token read_operator();
    
public:
    explicit Tokenizer(const std::string& input);
    
    std::vector<Token> tokenize();
    Token next_token();
    
    // Utility functions
    static std::string token_type_to_string(TokenType type);
    static bool is_keyword(const std::string& word);
    static TokenType get_keyword_type(const std::string& word);
};

} // namespace sqldb

#endif // TOKENIZER_H
