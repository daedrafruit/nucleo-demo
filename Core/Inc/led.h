#pragma once

#define WIDTH  8
#define HEIGHT 32
#define MAX_LED WIDTH*HEIGHT

void set_LED(int LED_num, int r, int g, int b);
void set_brightness(int brightness);
void send_LED();
int xy(int x, int y);
