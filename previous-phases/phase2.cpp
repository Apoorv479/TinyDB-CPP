#include <iostream>
#include <string>

enum PrepareResult
{
    PREPARE_SUCCESS,
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
};

PrepareResult prepare_statement(std::string input, Statement &statement)
{
    if (input.substr(0, 6) == "insert")
    {
        statement.type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if (input == "select")
    {
        statement.type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement &statement)
{
    switch (statement.type)
    {
    case STATEMENT_INSERT:
        std::cout << "This is where we would do an insert." << std::endl;
        break;
    case STATEMENT_SELECT:
        std::cout << "This is where we would do a select." << std::endl;
        break;
    }
}

int main()
{
    std::string input;
    while (true)
    {
        std::cout << "db > ";
        std::getline(std::cin, input);

        if (input == ".exit")
            break;

        Statement statement;
        switch (prepare_statement(input, statement))
        {
        case PREPARE_SUCCESS:
            execute_statement(statement);
            break;
        case PREPARE_UNRECOGNIZED_STATEMENT:
            std::cout << "Unrecognized keyword at start of '" << input << "'." << std::endl;
            continue;
        }
    }
    return 0;
}