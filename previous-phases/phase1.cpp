#include <iostream>
#include <string>

int main()
{
    std::string input_buffer;

    std::cout << " TinyDB Phase 1 (REPL) " << std::endl;

    while (true)
    {
        std::cout << "db > ";
        if (!std::getline(std::cin, input_buffer))
            break;

        // Meta Commands
        if (input_buffer == ".exit")
        {
            break;
        }
        else if (input_buffer[0] == '.')
        {
            std::cout << "Unrecognized command: " << input_buffer << std::endl;
        }
        else
        {
            std::cout << "I don't know how to handle: " << input_buffer << std::endl;
        }
    }
    return 0;
}