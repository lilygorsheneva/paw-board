#ifndef HAPTICS_H__
#define HAPTICS_H__

#include "constants.h"
#include "encoding.h"


void initialize_feedback(void);
void do_feedback(encoder_flags_t flags);

#endif