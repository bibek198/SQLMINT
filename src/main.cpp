#include "common/types.h"
#include "parser/tokenizer.h"
#include "parser/parser.h"
#include "executor/query_executor.h"
#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <cctype>

namespace sqldb {

class SQLShell {
private:
    std::unique_ptr<QueryExecutor> executor;
    bool running;
    
    // Command processing
    bool is_meta_command(const std::string& input);
    std::string process_meta_command(const std::string& input);
    std::string process_sql_command(const std::string& input);
    
    // Input handling
    std::string read_command();
    std::string trim(const std::string& str);
    
    // Display
    void print_welcome();
    void print_prompt();
    
public:
    SQLShell();
    void run();
};

SQLShell::SQLShell() : running(true) {
    try {
        executor = std::make_unique<QueryExecutor>();
    } catch (const std::exception& e) {
        std::cerr << "Error initializing database: " << e.what() << std::endl;
        running = false;
    }
}

void SQLShell::print_welcome() {
    std::cout << "SQL Database Engine v1.0\n";
    std::cout << "========================\n";
    std::cout << "Type 'help' or '\\h' for help, '\\q' to quit.\n\n";
}

void SQLShell::print_prompt() {
    std::cout << "sqldb> ";
}

std::string SQLShell::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::string SQLShell::read_command() {
    std::string line;
    std::string command;
    
    while (true) {
        if (!std::getline(std::cin, line)) {
            // EOF or error
            running = false;
            return "";
        }
        
        line = trim(line);
        if (line.empty()) {
            if (command.empty()) {
                print_prompt();
                continue;
            } else {
                break;
            }
        }
        
        command += line;
        
        // Check if command is complete (ends with semicolon or is a meta command)
        if (line.back() == ';' || line[0] == '\\' || 
            line == "help" || line == "exit" || line == "quit") {
            break;
        }
        
        command += " ";
        std::cout << "    -> ";  // Continuation prompt
    }
    
    return command;
}

bool SQLShell::is_meta_command(const std::string& input) {
    if (input.empty()) {
        return false;
    }
    
    if (input[0] == '\\') {
        return true;
    }
    
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    // Remove trailing semicolon if present
    if (!lower_input.empty() && lower_input.back() == ';') {
        lower_input.pop_back();
    }
    
    return (lower_input == "help" || lower_input == "exit" || lower_input == "quit" || lower_input == "clear");
}

std::string SQLShell::process_meta_command(const std::string& input) {
    std::string cmd = input;
    if (cmd[0] == '\\') {
        cmd = cmd.substr(1);
    }
    
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    
    // Remove trailing semicolon if present
    if (!cmd.empty() && cmd.back() == ';') {
        cmd.pop_back();
    }
    
    if (cmd == "q" || cmd == "quit" || cmd == "exit") {
        running = false;
        return "Goodbye!";
    } else if (cmd == "l" || cmd == "list") {
        return executor->list_tables();
    } else if (cmd == "h" || cmd == "help") {
        return executor->show_help();
    } else if (cmd == "c" || cmd == "clear") {
        // Clear screen using ANSI escape sequences
        std::cout << "\033[2J\033[H" << std::flush;
        return "";
    } else {
        return "Unknown meta command: " + input;
    }
}

std::string SQLShell::process_sql_command(const std::string& input) {
    try {
        // Remove trailing semicolon if present
        std::string sql = input;
        if (!sql.empty() && sql.back() == ';') {
            sql.pop_back();
        }
        
        // Tokenize
        Tokenizer tokenizer(sql);
        std::vector<Token> tokens = tokenizer.tokenize();
        
        // Parse
        Parser parser(tokens);
        std::unique_ptr<Statement> statement = parser.parse();
        
        if (!statement) {
            return "Error: Failed to parse SQL statement";
        }
        
        // Execute
        return executor->execute(std::move(statement));
        
    } catch (const Parser::ParseError& e) {
        return std::string("Parse Error: ") + e.what();
    } catch (const std::exception& e) {
        return std::string("Error: ") + e.what();
    }
}

void SQLShell::run() {
    if (!running) {
        return;
    }
    
    print_welcome();
    
    while (running) {
        print_prompt();
        
        std::string input = read_command();
        if (!running) {
            break;
        }
        
        if (input.empty()) {
            continue;
        }
        
        std::string result;
        if (is_meta_command(input)) {
            result = process_meta_command(input);
        } else {
            result = process_sql_command(input);
        }
        
        if (!result.empty()) {
            std::cout << result << std::endl;
        }
        
        std::cout << std::endl;
    }
}

} // namespace sqldb

int main() {
    try {
        sqldb::SQLShell shell;
        shell.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}
