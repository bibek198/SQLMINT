#include "table.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>

namespace sqldb {

TableStorage::TableStorage(const std::string& table_name, MetadataManager* metadata_mgr) 
    : table_name(table_name), metadata_manager(metadata_mgr) {
    if (!metadata_mgr) {
        throw std::runtime_error("MetadataManager cannot be null");
    }
    
    file_path = metadata_manager->get_table_file_path(table_name);
    ensure_table_file();
}

void TableStorage::ensure_table_file() {
    // Create file if it doesn't exist
    std::ifstream test(file_path);
    if (!test.is_open()) {
        std::ofstream create_file(file_path);
        if (!create_file.is_open()) {
            throw std::runtime_error("Cannot create table file: " + file_path);
        }
        create_file << "# Table data for " << table_name << "\n";
    }
}

std::string TableStorage::serialize_value(const Value& value, DataType type) {
    std::ostringstream oss;
    
    switch (type) {
        case DataType::INTEGER:
            oss << std::get<int>(value);
            break;
        case DataType::VARCHAR: {
            const std::string& str = std::get<std::string>(value);
            // Escape special characters
            std::string escaped;
            for (char c : str) {
                if (c == '|' || c == '\\' || c == '\n' || c == '\r') {
                    escaped += '\\';
                    switch (c) {
                        case '|': escaped += '|'; break;
                        case '\\': escaped += '\\'; break;
                        case '\n': escaped += 'n'; break;
                        case '\r': escaped += 'r'; break;
                    }
                } else {
                    escaped += c;
                }
            }
            oss << escaped;
            break;
        }
        case DataType::BOOLEAN:
            oss << (std::get<bool>(value) ? "1" : "0");
            break;
    }
    
    return oss.str();
}

Value TableStorage::deserialize_value(const std::string& value_str, DataType type) {
    switch (type) {
        case DataType::INTEGER:
            return Value(std::stoi(value_str));
        case DataType::VARCHAR: {
            // Unescape special characters
            std::string unescaped;
            for (size_t i = 0; i < value_str.length(); i++) {
                if (value_str[i] == '\\' && i + 1 < value_str.length()) {
                    char next = value_str[i + 1];
                    switch (next) {
                        case '|': unescaped += '|'; break;
                        case '\\': unescaped += '\\'; break;
                        case 'n': unescaped += '\n'; break;
                        case 'r': unescaped += '\r'; break;
                        default: unescaped += next; break;
                    }
                    i++; // Skip next character
                } else {
                    unescaped += value_str[i];
                }
            }
            return Value(unescaped);
        }
        case DataType::BOOLEAN:
            return Value(value_str == "1");
        default:
            throw std::runtime_error("Unknown data type for deserialization");
    }
}

std::string TableStorage::serialize_row(const Row& row) {
    const std::vector<Column> columns = metadata_manager->get_columns(table_name);
    if (row.size() != columns.size()) {
        throw std::runtime_error("Row size doesn't match table schema");
    }
    
    std::ostringstream oss;
    for (size_t i = 0; i < row.size(); i++) {
        if (i > 0) {
            oss << "|";
        }
        oss << serialize_value(row[i], columns[i].type);
    }
    
    return oss.str();
}

Row TableStorage::deserialize_row(const std::string& row_str) {
    const std::vector<Column> columns = metadata_manager->get_columns(table_name);
    
    std::vector<std::string> value_strings;
    std::string current_value;
    bool escaped = false;
    
    // Parse pipe-separated values with escape handling
    for (char c : row_str) {
        if (escaped) {
            current_value += c;
            escaped = false;
        } else if (c == '\\') {
            current_value += c;
            escaped = true;
        } else if (c == '|') {
            value_strings.push_back(current_value);
            current_value.clear();
        } else {
            current_value += c;
        }
    }
    
    if (!current_value.empty() || !value_strings.empty()) {
        value_strings.push_back(current_value);
    }
    
    if (value_strings.size() != columns.size()) {
        throw std::runtime_error("Row data doesn't match table schema");
    }
    
    Row row;
    for (size_t i = 0; i < value_strings.size(); i++) {
        row.push_back(deserialize_value(value_strings[i], columns[i].type));
    }
    
    return row;
}

void TableStorage::insert_row(const std::vector<Value>& values) {
    // Validate the insert
    metadata_manager->validate_insert_values(table_name, values);
    
    // Serialize and write to file
    std::string row_data = serialize_row(values);
    
    std::ofstream file(file_path, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open table file for writing: " + file_path);
    }
    
    file << row_data << "\n";
    file.close();
}

std::vector<Row> TableStorage::select_all() {
    std::vector<Row> rows;
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open table file for reading: " + file_path);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }
        
        try {
            Row row = deserialize_row(line);
            rows.push_back(row);
        } catch (const std::exception& e) {
            // Skip malformed rows, log error if needed
            continue;
        }
    }
    
    return rows;
}

std::vector<Row> TableStorage::select_where(const WhereCondition& condition) {
    // Validate the WHERE condition
    metadata_manager->validate_where_condition(table_name, condition);
    
    std::vector<Row> all_rows = select_all();
    std::vector<Row> filtered_rows;
    
    for (const Row& row : all_rows) {
        if (evaluate_condition(row, condition)) {
            filtered_rows.push_back(row);
        }
    }
    
    return filtered_rows;
}

bool TableStorage::evaluate_condition(const Row& row, const WhereCondition& condition) {
    int column_index = metadata_manager->get_column_index(table_name, condition.column_name);
    if (column_index < 0 || column_index >= static_cast<int>(row.size())) {
        return false;
    }
    
    const Value& row_value = row[column_index];
    return compare_values(row_value, condition.value, condition.operator_type);
}

bool TableStorage::compare_values(const Value& left, const Value& right, TokenType op) {
    // Type compatibility should already be validated
    
    try {
        switch (op) {
            case TokenType::EQUALS:
                return left == right;
            case TokenType::NOT_EQUALS:
                return left != right;
            case TokenType::LESS_THAN:
                return left < right;
            case TokenType::GREATER_THAN:
                return left > right;
            case TokenType::LESS_EQUAL:
                return left <= right;
            case TokenType::GREATER_EQUAL:
                return left >= right;
            default:
                return false;
        }
    } catch (...) {
        // Comparison failed (e.g., type mismatch)
        return false;
    }
}

size_t TableStorage::get_row_count() {
    return select_all().size();
}

void TableStorage::clear_table() {
    std::ofstream file(file_path, std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot clear table file: " + file_path);
    }
    file << "# Table data for " << table_name << "\n";
}

bool TableStorage::table_file_exists() const {
    std::ifstream file(file_path);
    return file.is_open();
}

void TableStorage::delete_table_file() {
    std::error_code ec;
    std::filesystem::remove(file_path, ec);
    // Ignore errors if file doesn't exist
}

} // namespace sqldb
