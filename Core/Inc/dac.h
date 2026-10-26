#pragma once

#include <stdint.h>
void play_audio(uint8_t offset);
void audio_update();
void audio_init();
void audio_set_rate(uint32_t freq);
