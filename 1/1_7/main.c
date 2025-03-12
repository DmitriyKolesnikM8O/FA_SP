#include "utility.h"


int main(int argc, char* argv[]) {
	for (size_t j = 1;j < argc; ++j) {
		char * catalog_name = argv[j];
		error_msg errorMsg = processing_catalog(catalog_name);
		if(errorMsg.type){
			return print_error(errorMsg);
		}
	}
	return 0;
}