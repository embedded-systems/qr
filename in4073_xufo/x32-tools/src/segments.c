/* see segments.h for more information */
#include "segments.h"

void get_segment_offsets(int base_offset, int code_size, int lit_size, 
	int data_size, int bss_size, int* start_code, int* start_lit,
	int* start_data, int* start_bss) {

		/* the default segment layout:
					<empty space, denoted by base_offset>
					CODE SEGMENT
					LITERAL SEGMENT
					DATA SEGMENT
					BSS SEGMENT
		
					All placed right after eachother
		*/
	
		*start_code = base_offset;
		*start_lit = *start_code + code_size;
		*start_data = *start_lit + lit_size;
		*start_bss = *start_data + data_size;
	
}
