#ifndef METADATA_H
#define METADATA_H

#include "../common/types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace sqldb {

class MetadataManager {
private:
    std::string data_directory;
    std::string metadata_file;
    std::unordered_map<std::string, std::unique_ptr<TableSchema>> tables;
    
    // File I/O helpers
    void ensure_data_directory();
    void load_metadata();
    void save_metadata();
    
    // Serialization helpers
    std::string serialize_data_type(DataType type);
    DataType deserialize_data_type(const std::string& type_str);
    std::string serialize_column(const Column& column);
    Column deserialize_column(const std::string& column_str);
    
public:
    explicit MetadataManager(const std::string& data_dir = "data");
    ~MetadataManager();
    
    // Table management
    bool table_exists(const std::string& table_name) const;
    void create_table(const std::string& table_name, const std::vector<Column>& columns);
    void drop_table(const std::string& table_name);
    
    // Schema access
    const TableSchema* get_table_schema(const std::string& table_name) const;
    std::vector<std::string> get_table_names() const;
    
    // Column information
    const Column* get_column(const std::string& table_name, const std::string& column_name) const;
    std::vector<Column> get_columns(const std::string& table_name) const;
    int get_column_index(const std::string& table_name, const std::string& column_name) const;
    
    // Validation
    void validate_table_name(const std::string& table_name) const;
    void validate_insert_values(const std::string& table_name, const std::vector<Value>& values) const;
    void validate_where_condition(const std::string& table_name, const WhereCondition& condition) const;
    
    // Data directory
    std::string get_data_directory() const { return data_directory; }
    std::string get_table_file_path(const std::string& table_name) const;
};

} // namespace sqldb

#endif // METADATA_H
