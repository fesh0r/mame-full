#ifndef INPUTX_H
#define INPUTX_H

void inputx_init(void);
void inputx_update(unsigned short *ports);

int inputx_can_post(void);
int inputx_can_post_key(int ch);
void inputx_post(const char *text);

#endif /* INPUTX_H */
