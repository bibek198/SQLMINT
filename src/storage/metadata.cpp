#include "metadata.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>

namespace sqldb {

MetadataManager::MetadataManager(const std::string& data_dir) 
    : data_directory(data_dir), metadata_file(data_dir + "/metadata.db") {
    ensure_data_directory();
    load_metadata();
}

MetadataManager::~MetadataManager() {
    save_metadata();
}

void MetadataManager::ensure_data_directory() {
    std::error_code ec;
    std::filesystem::create_directories(data_directory, ec);
    if (ec) {
        throw std::runtime_error("Failed to create data directory: " + ec.message());
    }
}

void MetadataManager::load_metadata() {
    std::ifstream file(metadata_file);
    if (!file.is_open()) {
        // File doesn't exist yet, that's okay
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }
        
        // Parse table definition line
        // Format: TABLE:table_name:column_count
        if (line.substr(0, 6) == "TABLE:") {
            std::istringstream iss(line);
            std::string token;
            std::getline(iss, token, ':'); // Skip "TABLE"
            
            std::string table_name;
            std::getline(iss, table_name, ':');
            
            std::string count_str;
            std::getline(iss, count_str);
            int column_count = std::stoi(count_str);
            
            auto schema = std::make_unique<TableSchema>(table_name);
            
            // Read column definitions
            for (int i = 0; i < column_count; i++) {
                if (!std::getline(file, line)) {
                    throw std::runtime_error("Incomplete table definition in metadata");
                }
                schema->columns.push_back(deserialize_column(line));
            }
            
            tables[table_name] = std::move(schema);
        }
    }
}

void MetadataManager::save_metadata() {
    std::ofstream file(metadata_file);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open metadata file for writing");
    }
    
    file << "# SQL Database Engine Metadata\n";
    file << "# Format: TABLE:name:column_count followed by column definitions\n\n";
    
    for (const auto& [table_name, schema] : tables) {
        file << "TABLE:" << table_name << ":" << schema->columns.size() << "\n";
        
        for (const auto& column : schema->columns) {
            file << serialize_column(column) << "\n";
        }
        
        file << "\n";
    }
}

std::string MetadataManager::serialize_data_type(DataType type) {
    switch (type) {
        case DataType::INTEGER: return "INTEGER";
        case DataType::VARCHAR: return "VARCHAR";
        case DataType::BOOLEAN: return "BOOLEAN";
        default: return "UNKNOWN";
    }
}

DataType MetadataManager::deserialize_data_type(const std::string& type_str) {
    if (type_str == "INTEGER") return DataType::INTEGER;
    if (type_str == "VARCHAR") return DataType::VARCHAR;
    if (type_str == "BOOLEAN") return DataType::BOOLEAN;
    throw std::runtime_error("Unknown data type: " + type_str);
}

std::string MetadataManager::serialize_column(const Column& column) {
    std::ostringstream oss;
    oss << "COLUMN:" << column.name << ":" << serialize_data_type(column.type);
    
    if (column.type == DataType::VARCHAR) {
        oss << ":" << column.varchar_length;
    } else {
        oss << ":0";
    }
    
    oss << ":" << (column.is_primary_key ? "1" : "0");
    oss << ":" << (column.is_not_null ? "1" : "0");
    
    return oss.str();
}

Column MetadataManager::deserialize_column(const std::string& column_str) {
    std::istringstream iss(column_str);
    std::string token;
    
    std::getline(iss, token, ':'); // Skip "COLUMN"
    
    std::string name;
    std::getline(iss, name, ':');
    
    std::string type_str;
    std::getline(iss, type_str, ':');
    DataType type = deserialize_data_type(type_str);
    
    std::string length_str;
    std::getline(iss, length_str, ':');
    int varchar_length = std::stoi(length_str);
    
    std::string pk_str;
    std::getline(iss, pk_str, ':');
    bool is_primary_key = (pk_str == "1");
    
    std::string nn_str;
    std::getline(iss, nn_str);
    bool is_not_null = (nn_str == "1");
    
    return Column(name, type, varchar_length, is_primary_key, is_not_null);
}

bool MetadataManager::table_exists(const std::string& table_name) const {
    return tables.find(table_name) != tables.end();
}

void MetadataManager::create_table(const std::string& table_name, const std::vector<Column>& columns) {
    if (table_exists(table_name)) {
        throw std::runtime_error("Table '" + table_name + "' already exists");
    }
    
    // Validate table name
    if (table_name.empty()) {
        throw std::runtime_error("Table name cannot be empty");
    }
    
    // Validate columns
    if (columns.empty()) {
        throw std::runtime_error("Table must have at least one column");
    }
    
    // Check for duplicate column names
    std::vector<std::string> column_names;
    int primary_key_count = 0;
    
    for (const auto& column : columns) {
        if (column.name.empty()) {
            throw std::runtime_error("Column name cannot be empty");
        }
        
        if (std::find(column_names.begin(), column_names.end(), column.name) != column_names.end()) {
            throw std::runtime_error("Duplicate column name: " + column.name);
        }
        
        column_names.push_back(column.name);
        
        if (column.is_primary_key) {
            primary_key_count++;
        }
        
        // Validate VARCHAR length
        if (column.type == DataType::VARCHAR && column.varchar_length <= 0) {
            throw std::runtime_error("VARCHAR length must be positive for column: " + column.name);
        }
    }
    
    // Check primary key constraint
    if (primary_key_count > 1) {
        throw std::runtime_error("Table can have at most one primary key");
    }
    
    auto schema = std::make_unique<TableSchema>(table_name);
    schema->columns = columns;
    tables[table_name] = std::move(schema);
    
    save_metadata();
}

void MetadataManager::drop_table(const std::string& table_name) {
    if (!table_exists(table_name)) {
        throw std::runtime_error("Table '" + table_name + "' does not exist");
    }
    
    tables.erase(table_name);
    save_metadata();
    
    // Also delete the table data file
    std::string table_file = get_table_file_path(table_name);
    std::error_code ec;
    std::filesystem::remove(table_file, ec);
    // Ignore errors if file doesn't exist
}

const TableSchema* MetadataManager::get_table_schema(const std::string& table_name) const {
    auto it = tables.find(table_name);
    return (it != tables.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> MetadataManager::get_table_names() const {
    std::vector<std::string> names;
    for (const auto& [name, schema] : tables) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

const Column* MetadataManager::get_column(const std::string& table_name, const std::string& column_name) const {
    const TableSchema* schema = get_table_schema(table_name);
    if (!schema) {
        return nullptr;
    }
    
    for (const auto& column : schema->columns) {
        if (column.name == column_name) {
            return &column;
        }
    }
    
    return nullptr;
}

std::vector<Column> MetadataManager::get_columns(const std::string& table_name) const {
    const TableSchema* schema = get_table_schema(table_name);
    if (!schema) {
        return {};
    }
    
    return schema->columns;
}

int MetadataManager::get_column_index(const std::string& table_name, const std::string& column_name) const {
    const TableSchema* schema = get_table_schema(table_name);
    if (!schema) {
        return -1;
    }
    
    for (size_t i = 0; i < schema->columns.size(); i++) {
        if (schema->columns[i].name == column_name) {
            return static_cast<int>(i);
        }
    }
    
    return -1;
}

void MetadataManager::validate_table_name(const std::string& table_name) const {
    if (!table_exists(table_name)) {
        throw std::runtime_error("Table '" + table_name + "' does not exist");
    }
}

void MetadataManager::validate_insert_values(const std::string& table_name, const std::vector<Value>& values) const {
    const TableSchema* schema = get_table_schema(table_name);
    if (!schema) {
        throw std::runtime_error("Table '" + table_name + "' does not exist");
    }
    
    if (values.size() != schema->columns.size()) {
        throw std::runtime_error("INSERT has " + std::to_string(values.size()) + 
                                 " values, expected " + std::to_string(schema->columns.size()));
    }
    
    // Type validation
    for (size_t i = 0; i < values.size(); i++) {
        const Column& column = schema->columns[i];
        const Value& value = values[i];
        
        // Check data type compatibility
        bool type_match = false;
        switch (column.type) {
            case DataType::INTEGER:
                type_match = std::holds_alternative<int>(value);
                break;
            case DataType::VARCHAR:
                if (std::holds_alternative<std::string>(value)) {
                    const std::string& str_value = std::get<std::string>(value);
                    if (static_cast<int>(str_value.length()) > column.varchar_length) {
                        throw std::runtime_error("String too long for column '" + column.name + 
                                                "', max length is " + std::to_string(column.varchar_length));
                    }
                    type_match = true;
                }
                break;
            case DataType::BOOLEAN:
                type_match = std::holds_alternative<bool>(value);
                break;
        }
        
        if (!type_match) {
            throw std::runtime_error("Type mismatch for column '" + column.name + "'");
        }
    }
}

void MetadataManager::validate_where_condition(const std::string& table_name, const WhereCondition& condition) const {
    const Column* column = get_column(table_name, condition.column_name);
    if (!column) {
        throw std::runtime_error("Column '" + condition.column_name + "' does not exist in table '" + table_name + "'");
    }
    
    // Type compatibility check
    bool type_match = false;
    switch (column->type) {
        case DataType::INTEGER:
            type_match = std::holds_alternative<int>(condition.value);
            break;
        case DataType::VARCHAR:
            type_match = std::holds_alternative<std::string>(condition.value);
            break;
        case DataType::BOOLEAN:
            type_match = std::holds_alternative<bool>(condition.value);
            break;
    }
    
    if (!type_match) {
        throw std::runtime_error("Type mismatch for column '" + condition.column_name + "'");
    }
}

std::string MetadataManager::get_table_file_path(const std::string& table_name) const {
    return data_directory + "/" + table_name + ".tbl";
}

} // namespace sqldb
