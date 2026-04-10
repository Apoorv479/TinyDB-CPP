#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <fstream>
#include <algorithm>

// ==========================================
// 1. DATA STRUCTURES & ENUMS
// ==========================================
const int COLUMN_USERNAME_SIZE = 32;
const int COLUMN_EMAIL_SIZE = 255;

struct Row
{
    int id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
};

struct MathOperation
{
    double num1;
    char op;
    double num2;
};

//  Specific columns choosing
struct SelectConfig
{
    bool all = true; // Default all true
    bool id = false;
    bool username = false;
    bool email = false;
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
    STATEMENT_SELECT,
    STATEMENT_MATH,
    STATEMENT_AGGREGATE
};
enum AggregateType
{
    AGG_COUNT,
    AGG_MAX,
    AGG_MIN,
    AGG_AVG
};
enum ExecuteResult
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
};

struct Statement
{
    StatementType type;
    Row row_to_insert;
    MathOperation math;
    AggregateType agg_type;
    SelectConfig select_cols; // : Parser will tell what to print
};

// ==========================================
// 2. DISK PERSISTENCE (File-per-Table)
// ==========================================
void db_open(std::vector<Row> &table, std::string filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        std::ofstream create_empty_file(filename, std::ios::app);
        std::cout << "Created a new database file: " << filename << std::endl;
        return;
    }
    Row temp_row;
    while (file.read((char *)&temp_row, sizeof(Row)))
        table.push_back(temp_row);
    file.close();
    std::cout << "Opened " << filename << " (" << table.size() << " rows loaded)" << std::endl;
}

void db_close(const std::vector<Row> &table, std::string filename)
{
    std::ofstream file(filename, std::ios::binary);
    for (const auto &row : table)
        file.write((const char *)&row, sizeof(Row));
    file.close();
    std::cout << "Saved data safely to " << filename << std::endl;
}

// ==========================================
// 3. THE PARSER
// ==========================================
PrepareResult prepare_statement(std::string input, Statement &statement)
{
    if (input.substr(0, 6) == "insert")
    {
        statement.type = STATEMENT_INSERT;
        std::stringstream ss(input);
        std::string cmd, user, email;
        int id;

        ss >> cmd >> id >> user >> email;
        if (ss.fail())
            return PREPARE_SYNTAX_ERROR;

        statement.row_to_insert.id = id;
        strncpy(statement.row_to_insert.username, user.c_str(), COLUMN_USERNAME_SIZE - 1);
        statement.row_to_insert.username[COLUMN_USERNAME_SIZE - 1] = '\0';
        strncpy(statement.row_to_insert.email, email.c_str(), COLUMN_EMAIL_SIZE - 1);
        statement.row_to_insert.email[COLUMN_EMAIL_SIZE - 1] = '\0';
        return PREPARE_SUCCESS;
    }

    if (input.substr(0, 6) == "select")
    {
        // Aggregates Check
        if (input == "select count")
        {
            statement.type = STATEMENT_AGGREGATE;
            statement.agg_type = AGG_COUNT;
            return PREPARE_SUCCESS;
        }
        if (input == "select max")
        {
            statement.type = STATEMENT_AGGREGATE;
            statement.agg_type = AGG_MAX;
            return PREPARE_SUCCESS;
        }
        if (input == "select min")
        {
            statement.type = STATEMENT_AGGREGATE;
            statement.agg_type = AGG_MIN;
            return PREPARE_SUCCESS;
        }
        if (input == "select avg")
        {
            statement.type = STATEMENT_AGGREGATE;
            statement.agg_type = AGG_AVG;
            return PREPARE_SUCCESS;
        }

        // Math Operation Check
        if (input.length() > 7 && (input.find('+') != std::string::npos || input.find('-') != std::string::npos || input.find('*') != std::string::npos || input.find('/') != std::string::npos))
        {
            statement.type = STATEMENT_MATH;
            std::stringstream ss(input.substr(7));
            ss >> statement.math.num1 >> statement.math.op >> statement.math.num2;
            if (ss.fail())
                return PREPARE_SYNTAX_ERROR;
            return PREPARE_SUCCESS;
        }

        // NAYA: Column Projection Logic
        statement.type = STATEMENT_SELECT;
        if (input == "select" || input == "select *")
        {
            statement.select_cols.all = true; // all print
            return PREPARE_SUCCESS;
        }

        //  specific columns asked user (e.g., "select id email")
        statement.select_cols.all = false; // All closed
        std::stringstream ss(input.substr(6));
        std::string col;
        while (ss >> col)
        {
            if (col == "id")
                statement.select_cols.id = true;
            else if (col == "username")
                statement.select_cols.username = true;
            else if (col == "email")
                statement.select_cols.email = true;
            else
                return PREPARE_SYNTAX_ERROR; // column asked which doesn't exist
        }
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

// ==========================================
// 4. THE EXECUTORS
// ==========================================
void execute_aggregate(Statement &statement, std::vector<Row> &table)
{ /* Wahi purana logic */
    if (table.empty() && statement.agg_type != AGG_COUNT)
    {
        std::cout << "Error: Table is empty." << std::endl;
        return;
    }
    switch (statement.agg_type)
    {
    case AGG_COUNT:
        std::cout << "Count: " << table.size() << std::endl;
        break;
    case AGG_MAX:
    {
        int m = table[0].id;
        for (const auto &r : table)
            if (r.id > m)
                m = r.id;
        std::cout << "Max ID: " << m << std::endl;
        break;
    }
    case AGG_MIN:
    {
        int m = table[0].id;
        for (const auto &r : table)
            if (r.id < m)
                m = r.id;
        std::cout << "Min ID: " << m << std::endl;
        break;
    }
    case AGG_AVG:
    {
        double s = 0;
        for (const auto &r : table)
            s += r.id;
        std::cout << "Average ID: " << s / table.size() << std::endl;
        break;
    }
    }
}

//  Executor which  checks and then prints
ExecuteResult execute_select(Statement &statement, std::vector<Row> &table)
{
    if (table.empty())
    {
        std::cout << "Table is empty." << std::endl;
        return EXECUTE_SUCCESS;
    }

    for (const auto &r : table)
    {
        std::cout << "(";
        bool first = true;

        if (statement.select_cols.all || statement.select_cols.id)
        {
            std::cout << r.id;
            first = false;
        }
        if (statement.select_cols.all || statement.select_cols.username)
        {
            if (!first)
                std::cout << ", ";
            std::cout << r.username;
            first = false;
        }
        if (statement.select_cols.all || statement.select_cols.email)
        {
            if (!first)
                std::cout << ", ";
            std::cout << r.email;
        }
        std::cout << ")" << std::endl;
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement &statement, std::vector<Row> &table)
{
    if (statement.type == STATEMENT_INSERT)
    {
        table.push_back(statement.row_to_insert);
        std::cout << "Executed. Row added!" << std::endl;
    }
    else if (statement.type == STATEMENT_SELECT)
    {
        execute_select(statement, table); // Updated executor
    }
    else if (statement.type == STATEMENT_MATH)
    {
        double res = 0;
        if (statement.math.op == '+')
            res = statement.math.num1 + statement.math.num2;
        else if (statement.math.op == '-')
            res = statement.math.num1 - statement.math.num2;
        else if (statement.math.op == '*')
            res = statement.math.num1 * statement.math.num2;
        else if (statement.math.op == '/')
        {
            if (statement.math.num2 == 0)
            {
                std::cout << "Error: Divide by zero!" << std::endl;
                return EXECUTE_SUCCESS;
            }
            res = statement.math.num1 / statement.math.num2;
        }
        std::cout << "Result: " << res << std::endl;
    }
    else if (statement.type == STATEMENT_AGGREGATE)
    {
        execute_aggregate(statement, table);
    }
    return EXECUTE_SUCCESS;
}

// ==========================================
// 5. MAIN LOOP
// ==========================================
int main()
{
    std::string input_buffer;
    std::vector<Row> table;
    std::string current_db = "";

    std::cout << "--- TinyDB (Projection + Multi-Table + Math) ---" << std::endl;
    std::cout << "Type '.open <filename>' to start." << std::endl;

    while (true)
    {
        std::cout << "db > ";
        if (!std::getline(std::cin, input_buffer))
            break;

        if (input_buffer[0] == '.')
        {
            if (input_buffer.substr(0, 5) == ".open")
            {
                if (current_db != "")
                {
                    db_close(table, current_db);
                }
                if (input_buffer.length() > 6)
                {
                    current_db = input_buffer.substr(6);
                    table.clear();
                    db_open(table, current_db);
                }
                else
                {
                    std::cout << "Error: Provide a filename" << std::endl;
                }
                continue;
            }
            else if (input_buffer == ".exit")
            {
                if (current_db != "")
                {
                    db_close(table, current_db);
                }
                break;
            }
            else
            {
                std::cout << "Unrecognized meta command." << std::endl;
                continue;
            }
        }

        Statement statement;
        if (prepare_statement(input_buffer, statement) != PREPARE_SUCCESS)
        {
            std::cout << "Syntax Error. Only 'id', 'username', and 'email' are valid columns." << std::endl;
            continue;
        }

        if (current_db == "" && statement.type != STATEMENT_MATH)
        {
            std::cout << "Error: No database opened. Use '.open <filename>' first." << std::endl;
            continue;
        }

        execute_statement(statement, table);
    }
    return 0;
}