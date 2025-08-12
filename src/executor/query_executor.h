#ifndef QUERY_EXECUTOR_H
#define QUERY_EXECUTOR_H

#include "../common/types.h"
#include "../storage/metadata.h"
#include "../storage/table.h"
#include <memory>
#include <string>

namespace sqldb {

class QueryExecutor {
private:
    std::unique_ptr<MetadataManager> metadata_manager;
    
    // Execution methods
    std::string execute_create_table(const CreateTableStatement& stmt);
    std::string execute_drop_table(const DropTableStatement& stmt);
    std::string execute_insert(const InsertStatement& stmt);
    std::string execute_select(const SelectStatement& stmt);
    
    // Utility methods
    std::string format_results(const std::vector<Row>& rows, const std::vector<Column>& columns);
    std::string format_value(const Value& value);
    std::string get_data_type_string(DataType type, int varchar_length = 0);
    
public:
    explicit QueryExecutor(const std::string& data_directory = "data");
    ~QueryExecutor() = default;
    
    // Main execution method
    std::string execute(std::unique_ptr<Statement> statement);
    
    // Meta commands
    std::string list_tables();
    std::string show_help();
    
    // Utility
    MetadataManager* get_metadata_manager() const { return metadata_manager.get(); }
};

} // namespace sqldb

#endif // QUERY_EXECUTOR_H
