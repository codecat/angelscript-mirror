/**
 * Functions for creating test scripts
 * @author Markus Lenger
 */

#include "script_factory.h"

/**
 * Create a test script that contains array_cnt arrays with elem_cnt elements each
 */
void create_array_test(unsigned array_cnt, unsigned elem_cnt, std::string &script)
{
    std::stringstream script_buffer;
    for (unsigned i = 0; i < array_cnt; i++)
    {
        script_buffer << "int[] array_" << i << " = {";
        if (elem_cnt > 0)
        {
            script_buffer << 0;
        }
        for (unsigned j = 1; j < elem_cnt; j++)
        {
            script_buffer << ", " << j;
        }
        script_buffer << "};";
    }
    script_buffer << std::endl << "int main() { print (\"elem 999 = \" + array_0[999] + \"\\n\"); return 0; }";
    script = script_buffer.str();
}

/**
 * Create a test script that contains symbols_cnt global integer constants
 */
void create_symbol_test(unsigned symbol_cnt, std::string &script)
{
    std::stringstream script_buffer;
    for (unsigned i = 0; i < symbol_cnt; i++)
    {
        script_buffer << "const int const_" << i << " = " << i << ";" << std::endl;
    }
    script = script_buffer.str();
}
