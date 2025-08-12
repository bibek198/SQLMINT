#include "query_executor.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace sqldb {

QueryExecutor::QueryExecutor(const std::string& data_directory) {
    metadata_manager = std::make_unique<MetadataManager>(data_directory);
}

std::string QueryExecutor::execute(std::unique_ptr<Statement> statement) {
    if (!statement) {
        return "Error: Null statement";
    }
    
    try {
        switch (statement->type) {
            case StatementType::CREATE_TABLE:
                return execute_create_table(*static_cast<CreateTableStatement*>(statement.get()));
            case StatementType::DROP_TABLE:
                return execute_drop_table(*static_cast<DropTableStatement*>(statement.get()));
            case StatementType::INSERT:
                return execute_insert(*static_cast<InsertStatement*>(statement.get()));
            case StatementType::SELECT:
                return execute_select(*static_cast<SelectStatement*>(statement.get()));
            default:
                return "Error: Unknown statement type";
        }
    } catch (const std::exception& e) {
        return std::string("Error: ") + e.what();
    }
}

std::string QueryExecutor::execute_create_table(const CreateTableStatement& stmt) {
    metadata_manager->create_table(stmt.table_name, stmt.columns);
    return "Table '" + stmt.table_name + "' created successfully.";
}

std::string QueryExecutor::execute_drop_table(const DropTableStatement& stmt) {
    // Validate table exists before dropping
    metadata_manager->validate_table_name(stmt.table_name);
    
    // Drop the table
    metadata_manager->drop_table(stmt.table_name);
    
    return "Table '" + stmt.table_name + "' dropped successfully.";
}

std::string QueryExecutor::execute_insert(const InsertStatement& stmt) {
    // Validate table exists
    metadata_manager->validate_table_name(stmt.table_name);
    
    // Create table storage and insert row
    TableStorage table_storage(stmt.table_name, metadata_manager.get());
    table_storage.insert_row(stmt.values);
    
    return "1 row inserted into '" + stmt.table_name + "'.";
}

std::string QueryExecutor::execute_select(const SelectStatement& stmt) {
    // Validate table exists
    metadata_manager->validate_table_name(stmt.table_name);
    
    // Get table schema
    const std::vector<Column> columns = metadata_manager->get_columns(stmt.table_name);
    
    // Create table storage and execute query
    TableStorage table_storage(stmt.table_name, metadata_manager.get());
    std::vector<Row> rows;
    
    if (stmt.where_condition) {
        rows = table_storage.select_where(*stmt.where_condition);
    } else {
        rows = table_storage.select_all();
    }
    
    return format_results(rows, columns);
}

std::string QueryExecutor::format_results(const std::vector<Row>& rows, const std::vector<Column>& columns) {
    if (columns.empty()) {
        return "No columns defined.";
    }
    
    // Calculate column widths
    std::vector<size_t> widths(columns.size());
    
    // Initialize with header widths
    for (size_t i = 0; i < columns.size(); i++) {
        widths[i] = columns[i].name.length();
    }
    
    // Calculate max width for each column based on data
    for (const Row& row : rows) {
        for (size_t i = 0; i < row.size() && i < columns.size(); i++) {
            std::string formatted = format_value(row[i]);
            widths[i] = std::max(widths[i], formatted.length());
        }
    }
    
    // Ensure minimum width
    for (size_t& width : widths) {
        width = std::max(width, size_t(10));
    }
    
    std::ostringstream result;
    
    // Print header
    result << "|";
    for (size_t i = 0; i < columns.size(); i++) {
        result << " " << std::left << std::setw(widths[i]) << columns[i].name << " |";
    }
    result << "\n";
    
    // Print separator
    result << "+";
    for (size_t i = 0; i < columns.size(); i++) {
        result << std::string(widths[i] + 2, '-') << "+";
    }
    result << "\n";
    
    // Print data rows
    for (const Row& row : rows) {
        result << "|";
        for (size_t i = 0; i < columns.size(); i++) {
            std::string value_str;
            if (i < row.size()) {
                value_str = format_value(row[i]);
            }
            result << " " << std::left << std::setw(widths[i]) << value_str << " |";
        }
        result << "\n";
    }
    
    result << rows.size() << " rows returned.";
    return result.str();
}

std::string QueryExecutor::format_value(const Value& value) {
    if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    return "";
}

std::string QueryExecutor::get_data_type_string(DataType type, int varchar_length) {
    switch (type) {
        case DataType::INTEGER:
            return "INTEGER";
        case DataType::VARCHAR:
            return "VARCHAR(" + std::to_string(varchar_length) + ")";
        case DataType::BOOLEAN:
            return "BOOLEAN";
        default:
            return "UNKNOWN";
    }
}

std::string QueryExecutor::list_tables() {
    std::vector<std::string> table_names = metadata_manager->get_table_names();
    
    if (table_names.empty()) {
        return "No tables found.";
    }
    
    std::ostringstream result;
    result << "Tables:\n";
    result << "=======\n";
    
    for (const std::string& table_name : table_names) {
        result << "  " << table_name << "\n";
        
        const std::vector<Column> columns = metadata_manager->get_columns(table_name);
        result << "    Columns:\n";
        
        for (const Column& column : columns) {
            result << "      " << column.name << " " 
                   << get_data_type_string(column.type, column.varchar_length);
            
            if (column.is_primary_key) {
                result << " PRIMARY KEY";
            }
            if (column.is_not_null) {
                result << " NOT NULL";
            }
            
            result << "\n";
        }
        
        result << "\n";
    }
    
    return result.str();
}

std::string QueryExecutor::show_help() {
    return R"(SQL Database Engine - Help
=========================

Supported SQL Commands:
-----------------------

CREATE TABLE table_name (
    column_name data_type [constraints],
    ...
);

DROP TABLE table_name;

Data Types:
  INTEGER        - 32-bit signed integers
  VARCHAR(n)     - Variable-length strings (max n characters)
  BOOLEAN        - True/false values

Constraints:
  PRIMARY KEY    - Designates primary key (max one per table)
  NOT NULL       - Column cannot be null

INSERT INTO table_name VALUES (value1, value2, ...);

SELECT * FROM table_name [WHERE column operator value];

Operators:
  =, !=, <>, <, >, <=, >=

Meta Commands:
--------------
\l, \list      - List all tables and their schemas
\h, help       - Show this help message
\c, clear      - Clear the terminal screen
\q, exit, quit - Exit the application

Examples:
---------
CREATE TABLE users (id INTEGER PRIMARY KEY, name VARCHAR(50), active BOOLEAN);
INSERT INTO users VALUES (1, 'Alice', true);
SELECT * FROM users WHERE id = 1;
DROP TABLE users;
)";
}

} // namespace sqldb
