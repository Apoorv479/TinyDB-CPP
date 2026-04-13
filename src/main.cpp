#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <fstream>
#include <algorithm>

enum DataType
{
    TYPE_INT,
    TYPE_TEXT
};

struct Column
{
    char name[32];
    DataType type;
    int size;
};

struct Table
{
    std::string name;
    std::vector<Column> schema;
    int row_size = 0;
    std::vector<std::vector<char>> rows;

    void calculate_row_size()
    {
        row_size = 0;
        for (const auto &col : schema)
            row_size += col.size;
    }

    int get_column_offset(int col_index)
    {
        int offset = 0;
        for (int i = 0; i < col_index; i++)
            offset += schema[i].size;
        return offset;
    }
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
    STATEMENT_AGGREGATE,
    STATEMENT_CREATE
};

struct Statement
{
    StatementType type;
    std::vector<std::string> insert_values;
    std::string agg_func;
    std::string agg_col;
    std::vector<int> projection_indices;
    double n1, n2;
    char op;
};

//  Whitespace Trimming Function
std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first)
        return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

std::string safe_substr(std::string str, int pos)
{
    if (pos >= str.length())
        return "";
    return str.substr(pos);
}

// PERSISTENCE & PARSER

void db_open(Table &table, std::string filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        table.name = filename;
        std::cout << "New database: " << filename << std::endl;
        return;
    }
    table.name = filename;
    table.schema.clear();
    table.rows.clear();
    int col_count;
    file.read((char *)&col_count, sizeof(int));
    for (int i = 0; i < col_count; i++)
    {
        Column col;
        file.read((char *)&col, sizeof(Column));
        table.schema.push_back(col);
    }
    table.calculate_row_size();
    while (true)
    {
        std::vector<char> row(table.row_size);
        if (!file.read(row.data(), table.row_size))
            break;
        table.rows.push_back(row);
    }
    file.close();
    std::cout << "Opened " << filename << " (" << table.rows.size() << " rows loaded)" << std::endl;
}

void db_close(Table &table)
{
    if (table.name == "" || table.schema.empty())
        return;
    std::ofstream file(table.name, std::ios::binary);
    int col_count = table.schema.size();
    file.write((char *)&col_count, sizeof(int));
    for (auto &col : table.schema)
        file.write((char *)&col, sizeof(Column));
    for (auto &row : table.rows)
        file.write(row.data(), table.row_size);
    file.close();
}

PrepareResult prepare_statement(std::string input, Statement &statement, Table &table)
{
    if (input.substr(0, 12) == "create table")
    {
        statement.type = STATEMENT_CREATE;
        std::stringstream ss(safe_substr(input, 13));
        std::string col_info;
        table.schema.clear();
        while (std::getline(ss, col_info, ','))
        {
            std::stringstream css(col_info);
            std::string name, type;
            css >> name >> type;
            Column c;
            strncpy(c.name, name.c_str(), 31);
            if (type == "int")
            {
                c.type = TYPE_INT;
                c.size = 4;
            }
            else
            {
                c.type = TYPE_TEXT;
                c.size = 255;
            }
            table.schema.push_back(c);
        }
        table.calculate_row_size();
        return PREPARE_SUCCESS;
    }

    if (input.substr(0, 6) == "insert")
    {
        statement.type = STATEMENT_INSERT;
        std::stringstream ss(safe_substr(input, 7));
        std::string val;
        while (ss >> val)
            statement.insert_values.push_back(val);
        if (statement.insert_values.size() != table.schema.size())
            return PREPARE_SYNTAX_ERROR;
        return PREPARE_SUCCESS;
    }

    if (input.substr(0, 6) == "select")
    {
        std::string after_select = trim(safe_substr(input, 7));

        if (after_select == "" || after_select == "*")
        {
            statement.type = STATEMENT_SELECT;
            for (int i = 0; i < table.schema.size(); i++)
                statement.projection_indices.push_back(i);
            return PREPARE_SUCCESS;
        }

        if (input.find_first_of("+-*/") != std::string::npos)
        {
            statement.type = STATEMENT_MATH;
            std::stringstream mss(safe_substr(input, 7));
            mss >> statement.n1 >> statement.op >> statement.n2;
            return PREPARE_SUCCESS;
        }

        std::stringstream ss(after_select);
        std::string first;
        ss >> first;
        if (first == "count" || first == "max" || first == "min" || first == "avg")
        {
            statement.type = STATEMENT_AGGREGATE;
            statement.agg_func = first;
            ss >> statement.agg_col;
            return PREPARE_SUCCESS;
        }

        statement.type = STATEMENT_SELECT;
        std::stringstream pss(after_select);
        std::string col;
        while (pss >> col)
        {
            bool found = false;
            for (int i = 0; i < table.schema.size(); i++)
            {
                if (table.schema[i].name == col)
                {
                    statement.projection_indices.push_back(i);
                    found = true;
                    break;
                }
            }
            if (!found)
                return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

// EXECUTORS

void execute_statement(Statement &statement, Table &table)
{
    if (statement.type == STATEMENT_CREATE)
    {
        std::cout << "Schema defined (" << table.schema.size() << " columns)." << std::endl;
    }
    else if (statement.type == STATEMENT_INSERT)
    {
        std::vector<char> row(table.row_size);
        int offset = 0;
        for (int i = 0; i < table.schema.size(); i++)
        {
            if (table.schema[i].type == TYPE_INT)
            {
                int val = std::stoi(statement.insert_values[i]);
                memcpy(row.data() + offset, &val, 4);
            }
            else
            {
                char text[255] = {0};
                strncpy(text, statement.insert_values[i].c_str(), 254);
                memcpy(row.data() + offset, text, 255);
            }
            offset += table.schema[i].size;
        }
        table.rows.push_back(row);
        std::cout << "Row added." << std::endl;
    }
    else if (statement.type == STATEMENT_SELECT)
    {
        if (table.rows.empty())
        {
            std::cout << "Table is empty." << std::endl;
            return;
        }
        for (auto &row : table.rows)
        {
            std::cout << "(";
            for (int i = 0; i < statement.projection_indices.size(); i++)
            {
                int idx = statement.projection_indices[i];
                int offset = table.get_column_offset(idx);
                if (table.schema[idx].type == TYPE_INT)
                {
                    int val;
                    memcpy(&val, row.data() + offset, 4);
                    std::cout << val;
                }
                else
                {
                    char text[255];
                    memcpy(text, row.data() + offset, 255);
                    std::cout << text;
                }
                if (i < statement.projection_indices.size() - 1)
                    std::cout << ", ";
            }
            std::cout << ")" << std::endl;
        }
    }
    else if (statement.type == STATEMENT_MATH)
    {
        double r = 0;
        if (statement.op == '+')
            r = statement.n1 + statement.n2;
        else if (statement.op == '-')
            r = statement.n1 - statement.n2;
        else if (statement.op == '*')
            r = statement.n1 * statement.n2;
        else if (statement.op == '/')
            r = (statement.n2 != 0) ? statement.n1 / statement.n2 : 0;
        std::cout << "Math Result: " << r << std::endl;
    }
    else if (statement.type == STATEMENT_AGGREGATE)
    {
        if (table.rows.empty())
        {
            std::cout << "Empty table." << std::endl;
            return;
        }
        if (statement.agg_func == "count")
        {
            std::cout << "Count: " << table.rows.size() << std::endl;
            return;
        }
        int col_idx = -1;
        for (int i = 0; i < table.schema.size(); i++)
            if (table.schema[i].name == statement.agg_col)
                col_idx = i;
        if (col_idx == -1)
        {
            std::cout << "Error: Column not found." << std::endl;
            return;
        }

        int offset = table.get_column_offset(col_idx);
        double res = 0;
        bool first = true;
        for (auto &r : table.rows)
        {
            int val;
            memcpy(&val, r.data() + offset, 4);
            if (first)
            {
                res = val;
                first = false;
            }
            else
            {
                if (statement.agg_func == "max")
                    res = std::max((double)val, res);
                if (statement.agg_func == "min")
                    res = std::min((double)val, res);
                if (statement.agg_func == "avg")
                    res += val;
            }
        }
        if (statement.agg_func == "avg")
            res /= table.rows.size();
        std::cout << statement.agg_func << ": " << res << std::endl;
    }
}

int main()
{
    std::string input;
    Table table;
    std::cout << " TinyDB  " << std::endl;
    while (true)
    {
        std::cout << "db > ";
        if (!std::getline(std::cin, input))
            break;

        input = trim(input);
        if (input == "")
            continue;
        if (input == ".exit")
        {
            if (table.name != "")
                db_close(table);
            break;
        }

        if (input.substr(0, 5) == ".open")
        {
            if (table.name != "")
                db_close(table);
            db_open(table, trim(input.substr(6)));
            continue;
        }

        Statement stmt;
        if (prepare_statement(input, stmt, table) == PREPARE_SUCCESS)
        {
            execute_statement(stmt, table);
        }
        else
        {
            std::cout << "Error: Command invalid or schema mismatch." << std::endl;
        }
    }
    return 0;
}
