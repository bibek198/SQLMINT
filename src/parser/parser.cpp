#include "parser.h"
#include <stdexcept>
#include <algorithm>

namespace sqldb {

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current_pos(0) {}

const Token& Parser::peek() const {
    if (current_pos >= tokens.size()) {
        static Token eof_token(TokenType::END_OF_FILE, "");
        return eof_token;
    }
    return tokens[current_pos];
}

const Token& Parser::advance() {
    if (current_pos < tokens.size()) {
        return tokens[current_pos++];
    }
    static Token eof_token(TokenType::END_OF_FILE, "");
    return eof_token;
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match_any(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (peek().type == type) {
            advance();
            return true;
        }
    }
    return false;
}

void Parser::expect(TokenType type, const std::string& error_message) {
    if (!match(type)) {
        throw ParseError(error_message + ", got " + Tokenizer::token_type_to_string(peek().type));
    }
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::END_OF_FILE;
}

std::string Parser::current_token_value() const {
    return peek().value;
}

std::unique_ptr<Statement> Parser::parse() {
    if (is_at_end()) {
        return nullptr;
    }
    
    switch (peek().type) {
        case TokenType::CREATE:
            return parse_create_table();
        case TokenType::DROP:
            return parse_drop_table();
        case TokenType::INSERT:
            return parse_insert();
        case TokenType::SELECT:
            return parse_select();
        default:
            throw ParseError("Expected SQL keyword");
    }
}

std::unique_ptr<CreateTableStatement> Parser::parse_create_table() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    expect(TokenType::CREATE, "Expected CREATE");
    expect(TokenType::TABLE, "Expected TABLE");
    
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected table name");
    }
    stmt->table_name = advance().value;
    
    expect(TokenType::LEFT_PAREN, "Expected '('");
    
    // Parse column definitions
    do {
        if (peek().type == TokenType::RIGHT_PAREN) {
            break;
        }
        
        Column column = parse_column_definition();
        stmt->columns.push_back(column);
        
    } while (match(TokenType::COMMA));
    
    expect(TokenType::RIGHT_PAREN, "Expected ')'");
    
    return stmt;
}

std::unique_ptr<DropTableStatement> Parser::parse_drop_table() {
    auto stmt = std::make_unique<DropTableStatement>();
    
    expect(TokenType::DROP, "Expected DROP");
    expect(TokenType::TABLE, "Expected TABLE");
    
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected table name");
    }
    stmt->table_name = advance().value;
    
    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parse_insert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    expect(TokenType::INSERT, "Expected INSERT");
    expect(TokenType::INTO, "Expected INTO");
    
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected table name");
    }
    stmt->table_name = advance().value;
    
    expect(TokenType::VALUES, "Expected VALUES");
    expect(TokenType::LEFT_PAREN, "Expected '('");
    
    // Parse values
    do {
        if (peek().type == TokenType::RIGHT_PAREN) {
            break;
        }
        
        Value value = parse_value();
        stmt->values.push_back(value);
        
    } while (match(TokenType::COMMA));
    
    expect(TokenType::RIGHT_PAREN, "Expected ')'");
    
    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parse_select() {
    auto stmt = std::make_unique<SelectStatement>();
    
    expect(TokenType::SELECT, "Expected SELECT");
    
    if (match(TokenType::ASTERISK)) {
        stmt->select_all = true;
    } else {
        throw ParseError("Only SELECT * is currently supported");
    }
    
    expect(TokenType::FROM, "Expected FROM");
    
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected table name");
    }
    stmt->table_name = advance().value;
    
    // Optional WHERE clause
    if (match(TokenType::WHERE)) {
        stmt->where_condition = parse_where_clause();
    }
    
    return stmt;
}

Column Parser::parse_column_definition() {
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected column name");
    }
    
    std::string column_name = advance().value;
    
    int varchar_length = 0;
    DataType data_type = parse_data_type(varchar_length);
    
    std::vector<ConstraintType> constraints = parse_constraints();
    
    bool is_primary_key = std::find(constraints.begin(), constraints.end(), 
                                   ConstraintType::PRIMARY_KEY) != constraints.end();
    bool is_not_null = std::find(constraints.begin(), constraints.end(), 
                                ConstraintType::NOT_NULL) != constraints.end();
    
    return Column(column_name, data_type, varchar_length, is_primary_key, is_not_null);
}

DataType Parser::parse_data_type(int& varchar_length) {
    if (match(TokenType::INTEGER)) {
        return DataType::INTEGER;
    } else if (match(TokenType::BOOLEAN)) {
        return DataType::BOOLEAN;
    } else if (match(TokenType::VARCHAR)) {
        expect(TokenType::LEFT_PAREN, "Expected '(' after VARCHAR");
        
        if (peek().type != TokenType::INTEGER_LITERAL) {
            throw ParseError("Expected VARCHAR length");
        }
        
        varchar_length = std::stoi(advance().value);
        expect(TokenType::RIGHT_PAREN, "Expected ')' after VARCHAR length");
        
        return DataType::VARCHAR;
    } else {
        throw ParseError("Expected data type");
    }
}

std::vector<ConstraintType> Parser::parse_constraints() {
    std::vector<ConstraintType> constraints;
    
    while (true) {
        if (match(TokenType::PRIMARY)) {
            expect(TokenType::KEY, "Expected KEY after PRIMARY");
            constraints.push_back(ConstraintType::PRIMARY_KEY);
        } else if (match(TokenType::NOT)) {
            expect(TokenType::NULL_KEYWORD, "Expected NULL after NOT");
            constraints.push_back(ConstraintType::NOT_NULL);
        } else {
            break;
        }
    }
    
    return constraints;
}

Value Parser::parse_value() {
    if (peek().type == TokenType::INTEGER_LITERAL) {
        int value = std::stoi(advance().value);
        return Value(value);
    } else if (peek().type == TokenType::STRING_LITERAL) {
        std::string value = advance().value;
        return Value(value);
    } else if (peek().type == TokenType::BOOLEAN_LITERAL) {
        std::string bool_str = advance().value;
        std::transform(bool_str.begin(), bool_str.end(), bool_str.begin(), ::toupper);
        bool value = (bool_str == "TRUE");
        return Value(value);
    } else {
        throw ParseError("Expected value");
    }
}

std::unique_ptr<WhereCondition> Parser::parse_where_clause() {
    if (peek().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected column name in WHERE clause");
    }
    
    std::string column_name = advance().value;
    
    TokenType operator_type;
    if (match_any({TokenType::EQUALS, TokenType::NOT_EQUALS, TokenType::LESS_THAN,
                   TokenType::GREATER_THAN, TokenType::LESS_EQUAL, TokenType::GREATER_EQUAL})) {
        operator_type = tokens[current_pos - 1].type;
    } else {
        throw ParseError("Expected comparison operator in WHERE clause");
    }
    
    Value value = parse_value();
    
    return std::make_unique<WhereCondition>(column_name, operator_type, value);
}

} // namespace sqldb
