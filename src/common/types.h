#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace sqldb {

// Token types for SQL parsing
enum class TokenType {
    // Literals
    IDENTIFIER,
    INTEGER_LITERAL,
    STRING_LITERAL,
    BOOLEAN_LITERAL,
    
    // Keywords
    CREATE,
    DROP,
    TABLE,
    INSERT,
    INTO,
    SELECT,
    FROM,
    WHERE,
    VALUES,
    
    // Data types
    INTEGER,
    VARCHAR,
    BOOLEAN,
    
    // Constraints
    PRIMARY,
    KEY,
    NOT,
    NULL_KEYWORD,
    
    // Operators
    EQUALS,
    NOT_EQUALS,
    LESS_THAN,
    GREATER_THAN,
    LESS_EQUAL,
    GREATER_EQUAL,
    
    // Punctuation
    SEMICOLON,
    COMMA,
    LEFT_PAREN,
    RIGHT_PAREN,
    ASTERISK,
    
    // Special
    END_OF_FILE,
    UNKNOWN
};

// SQL data types
enum class DataType {
    INTEGER,
    VARCHAR,
    BOOLEAN
};

// Column constraints
enum class ConstraintType {
    PRIMARY_KEY,
    NOT_NULL
};

// Value type for storing different data types
using Value = std::variant<int, std::string, bool>;

// Token structure
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t, const std::string& v, int l = 1, int c = 1)
        : type(t), value(v), line(l), column(c) {}
};

// Column definition
struct Column {
    std::string name;
    DataType type;
    int varchar_length;  // Only used for VARCHAR
    bool is_primary_key;
    bool is_not_null;
    
    Column(const std::string& n, DataType t, int len = 0, bool pk = false, bool nn = false)
        : name(n), type(t), varchar_length(len), is_primary_key(pk), is_not_null(nn) {}
};

// Table schema
struct TableSchema {
    std::string name;
    std::vector<Column> columns;
    
    TableSchema(const std::string& n) : name(n) {}
};

// Row data
using Row = std::vector<Value>;

// WHERE clause condition
struct WhereCondition {
    std::string column_name;
    TokenType operator_type;
    Value value;
    
    WhereCondition(const std::string& col, TokenType op, const Value& val)
        : column_name(col), operator_type(op), value(val) {}
};

// SQL Statement types
enum class StatementType {
    CREATE_TABLE,
    DROP_TABLE,
    INSERT,
    SELECT
};

// Base SQL statement
struct Statement {
    StatementType type;
    virtual ~Statement() = default;
};

// CREATE TABLE statement
struct CreateTableStatement : public Statement {
    std::string table_name;
    std::vector<Column> columns;
    
    CreateTableStatement() { type = StatementType::CREATE_TABLE; }
};

// INSERT statement
struct InsertStatement : public Statement {
    std::string table_name;
    std::vector<Value> values;
    
    InsertStatement() { type = StatementType::INSERT; }
};

// DROP TABLE statement
struct DropTableStatement : public Statement {
    std::string table_name;
    
    DropTableStatement() { type = StatementType::DROP_TABLE; }
};

// SELECT statement
struct SelectStatement : public Statement {
    std::string table_name;
    bool select_all;
    std::unique_ptr<WhereCondition> where_condition;
    
    SelectStatement() : select_all(true), where_condition(nullptr) { 
        type = StatementType::SELECT; 
    }
};

} // namespace sqldb

#endif // TYPES_H
