#ifndef __SVGAINPUT_H
#define __SVGAINPUT_H

int svga_input_init(void (*release_func)(void), void (*aqcuire_func)(void));
void svga_input_exit(void);

#endif
