#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <fstream> // File handling ke liye zaroori

const int COLUMN_USERNAME_SIZE = 32;
const int COLUMN_EMAIL_SIZE = 255;
const std::string DB_FILENAME = "apoorv.db"; // Hardcoded filename

struct Row
{
    int id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
};

enum PrepareResult
{
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT
};
enum StatementType
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
};
enum ExecuteResult
{
    EXECUTE_SUCCESS
};

struct Statement
{
    StatementType type;
    Row row_to_insert;
};

// ==========================================
// DISK PERSISTENCE LOGIC (Phase 4 Core)
// ==========================================

// Program shuru hote hi file se data RAM (vector) mein load karna
void db_open(std::vector<Row> &table)
{
    std::ifstream file(DB_FILENAME, std::ios::binary);

    if (!file)
    {
        std::cout << "No existing database found. Starting with an empty table." << std::endl;
        return;
    }

    Row temp_row;
    // File se ek-ek Row ka size padhna aur vector mein daalna
    while (file.read((char *)&temp_row, sizeof(Row)))
    {
        table.push_back(temp_row);
    }
    file.close();
    std::cout << "Successfully loaded " << table.size() << " rows from " << DB_FILENAME << std::endl;
}

// Program band hone se pehle RAM ka data wapas file mein save karna
void db_close(const std::vector<Row> &table)
{
    std::ofstream file(DB_FILENAME, std::ios::binary);

    for (const auto &row : table)
    {
        file.write((const char *)&row, sizeof(Row));
    }
    file.close();
    std::cout << "Database saved to disk. Goodbye!" << std::endl;
}

// ==========================================
// PARSER & EXECUTOR
// ==========================================

PrepareResult prepare_statement(std::string input, Statement &statement)
{
    if (input.substr(0, 6) == "insert")
    {
        statement.type = STATEMENT_INSERT;
        std::stringstream ss(input);
        std::string keyword, user, email;
        int id;
        ss >> keyword >> id >> user >> email;

        if (ss.fail())
            return PREPARE_SYNTAX_ERROR;

        statement.row_to_insert.id = id;
        strncpy(statement.row_to_insert.username, user.c_str(), COLUMN_USERNAME_SIZE);
        strncpy(statement.row_to_insert.email, email.c_str(), COLUMN_EMAIL_SIZE);
        return PREPARE_SUCCESS;
    }
    if (input == "select")
    {
        statement.type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement &statement, std::vector<Row> &table)
{
    if (statement.type == STATEMENT_INSERT)
    {
        table.push_back(statement.row_to_insert);
        std::cout << "Executed." << std::endl;
    }
    else
    {
        if (table.empty())
            std::cout << "Table is empty." << std::endl;
        for (const auto &row : table)
        {
            std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
        }
    }
}

// ==========================================
// MAIN REPL LOOP
// ==========================================

int main()
{
    std::string input_buffer;
    std::vector<Row> table;

    // Load existing data at startup
    db_open(table);

    std::cout << "--- TinyDB Phase 4 (Single-File Persistence) ---" << std::endl;

    while (true)
    {
        std::cout << "db > ";
        if (!std::getline(std::cin, input_buffer))
            break;

        if (input_buffer == ".exit")
        {
            db_close(table); // Save data before exiting
            break;
        }

        Statement statement;
        PrepareResult res = prepare_statement(input_buffer, statement);

        if (res == PREPARE_SUCCESS)
        {
            execute_statement(statement, table);
        }
        else if (res == PREPARE_SYNTAX_ERROR)
        {
            std::cout << "Syntax error. Use: insert <id> <user> <email>" << std::endl;
        }
        else
        {
            std::cout << "Unrecognized command." << std::endl;
        }
    }
    return 0;
}