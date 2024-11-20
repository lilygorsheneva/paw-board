#ifndef HAPTICS_H__
#define HAPTICS_H__

#include "constants.h"

typedef enum 
{
    HAPTICS_MODE_SINGLE,
    HAPTICS_MODE_FIVE,
} haptics_mode_t;

static const haptics_mode_t HAPTICS_MODE = HAPTICS_MODE_SINGLE;

void initialize_feedback(void);
void do_feedback(bool force_off, bool force_full);

#endif