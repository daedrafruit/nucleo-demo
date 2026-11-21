#pragma once

#include <stdint.h>

typedef struct {
  uint32_t time;
  uint8_t lane;
} input_event_t;

uint8_t poll_input_event(input_event_t *e);
void load_input_buffer(uint32_t chart_time);
void init_input_buffer();


