#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>

const int COLUMN_USERNAME_SIZE = 32;
const int COLUMN_EMAIL_SIZE = 255;

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

struct Statement
{
    StatementType type;
    Row row_to_insert;
};

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
        for (auto &row : table)
        {
            std::cout << "(" << row.id << ", " << row.username << ", " << row.email << ")" << std::endl;
        }
    }
}

int main()
{
    std::string input;
    std::vector<Row> table;
    while (true)
    {
        std::cout << "db > ";
        std::getline(std::cin, input);
        if (input == ".exit")
            break;

        Statement statement;
        PrepareResult res = prepare_statement(input, statement);
        if (res == PREPARE_SUCCESS)
            execute_statement(statement, table);
        else
            std::cout << "Error processing command." << std::endl;
    }
    return 0;
}