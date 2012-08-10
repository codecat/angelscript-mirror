/**
 * Functions for creating test scripts
 * @author Markus Lenger
 */
#ifndef AS_TEST_SCRIPT_FACTORY_H
#define AS_TEST_SCRIPT_FACTORY_H

#include <string>
#include <sstream>

void create_array_test(unsigned array_cnt, unsigned elem_cnt, std::string &result);
void create_symbol_test(unsigned symbol_cnt, std::string &result);

#endif
