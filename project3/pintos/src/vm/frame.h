#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "threads/vaddr.h"
#include "threads/init.h"
#include "threads/synch.h"

void frame_table_init(size_t num_of_user_frame);

void frame_table_free();

bool evict_single_frame();

