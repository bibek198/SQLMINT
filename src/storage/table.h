#ifndef TABLE_H
#define TABLE_H

#include "../common/types.h"
#include "metadata.h"
#include <string>
#include <vector>
#include <fstream>

namespace sqldb {

class TableStorage {
private:
    std::string table_name;
    std::string file_path;
    MetadataManager* metadata_manager;
    
    // File I/O helpers
    void ensure_table_file();
    std::string serialize_value(const Value& value, DataType type);
    Value deserialize_value(const std::string& value_str, DataType type);
    std::string serialize_row(const Row& row);
    Row deserialize_row(const std::string& row_str);
    
    // Query helpers
    bool evaluate_condition(const Row& row, const WhereCondition& condition);
    bool compare_values(const Value& left, const Value& right, TokenType op);
    
public:
    TableStorage(const std::string& table_name, MetadataManager* metadata_mgr);
    
    // Data operations
    void insert_row(const std::vector<Value>& values);
    std::vector<Row> select_all();
    std::vector<Row> select_where(const WhereCondition& condition);
    
    // Utility
    size_t get_row_count();
    void clear_table();
    
    // File operations
    bool table_file_exists() const;
    void delete_table_file();
};

} // namespace sqldb

#endif // TABLE_H
