#ifndef PARSER_H
#define PARSER_H

#include "../common/types.h"
#include "tokenizer.h"
#include <memory>
#include <vector>
#include <stdexcept>

namespace sqldb {

class Parser {
private:
    std::vector<Token> tokens;
    size_t current_pos;
    
    // Helper methods
    const Token& peek() const;
    const Token& advance();
    bool match(TokenType type);
    bool match_any(const std::vector<TokenType>& types);
    void expect(TokenType type, const std::string& error_message);
    
    // Parsing methods
    std::unique_ptr<CreateTableStatement> parse_create_table();
    std::unique_ptr<DropTableStatement> parse_drop_table();
    std::unique_ptr<InsertStatement> parse_insert();
    std::unique_ptr<SelectStatement> parse_select();
    
    Column parse_column_definition();
    DataType parse_data_type(int& varchar_length);
    std::vector<ConstraintType> parse_constraints();
    Value parse_value();
    std::unique_ptr<WhereCondition> parse_where_clause();
    
    // Utility methods
    bool is_at_end() const;
    std::string current_token_value() const;
    
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    std::unique_ptr<Statement> parse();
    
    // Error handling
    class ParseError : public std::runtime_error {
    public:
        explicit ParseError(const std::string& message) : std::runtime_error(message) {}
    };
};

} // namespace sqldb

#endif // PARSER_H
