/**********************************************************************

	8259 PIC interface and emulation

**********************************************************************/

#ifndef PIC8259_H
#define PIC8259_H

#include "driver.h"

int pic8259_init(int count);

WRITE_HANDLER ( pic8259_0_w );
READ_HANDLER ( pic8259_0_r );
WRITE_HANDLER ( pic8259_1_w );
READ_HANDLER ( pic8259_1_r );

void pic8259_0_issue_irq(int irq);
void pic8259_1_issue_irq(int irq);

int pic8259_0_irq_pending(int irq);
int pic8259_1_irq_pending(int irq);

#endif /* PIC8259_H */
