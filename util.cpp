#include "util.h"

bool u_strcmp( const char* str1, const char* str2 ) {
	int i = 0;
	while( (str1[i] != '\0') && (str2[i] != '\0') ) {
		if( str1[i] == str2[i] ) {
			i++;
			continue;
		}
		else {
			return false;
		}
	}
	// If both strings are at their end
	if( str1[i] == str2[i] ) {
		return true;
	}
	return false;
}

std::vector<char> u_readFile( const std::string& fileName ) {
	std::ifstream file( fileName, std::ios::ate | std::ios::binary );

	if( !file.is_open() ) {
		throw std::runtime_error( "Failed to open file" );
	}

	size_t fileSize = ( size_t )file.tellg();
	std::vector<char> buffer( fileSize );

	file.seekg( 0 );
	file.read( buffer.data(), fileSize );
	file.close();

	return buffer;
}