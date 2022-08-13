#ifndef UTIL_H
#define UTIL_H

#include <fstream>
#include <vector>

bool u_strcmp( const char* str1, const char* str2 );
std::vector<char> u_readFile( const std::string& fileName );

#endif /* UTIL_H */
