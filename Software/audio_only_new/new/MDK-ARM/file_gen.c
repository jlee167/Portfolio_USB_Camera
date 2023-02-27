#include "file_gen.h"


volatile int curr_rec_idx;
	
	
void set_rec_idx(int idx) {
	curr_rec_idx = idx;
}


int get_rec_idx(void) {
	return curr_rec_idx;
}


int is_rec_idx_set(void) {
	if (curr_rec_idx)
		return 1;
	else
		return 0;
}