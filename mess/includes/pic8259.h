/**********************************************************************

	8259 PIC interface and emulation

**********************************************************************/

#ifndef PIC8259_H
#define PIC8259_H

#include "driver.h"


int pic8259_init(int count);
void pic8259_reset(void);

READ8_HANDLER ( pic8259_0_r );
READ8_HANDLER ( pic8259_1_r );
WRITE8_HANDLER ( pic8259_0_w );
WRITE8_HANDLER ( pic8259_1_w );

READ32_HANDLER ( pic8259_32_0_r );
READ32_HANDLER ( pic8259_32_1_r );
WRITE32_HANDLER ( pic8259_32_0_w );
WRITE32_HANDLER ( pic8259_32_1_w );

void pic8259_0_issue_irq(int irq);
void pic8259_1_issue_irq(int irq);

int pic8259_0_irq_pending(int irq);
int pic8259_1_irq_pending(int irq);

#endif /* PIC8259_H */
