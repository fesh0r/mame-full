/***************************************************************************
    
	IBM AT Compatibles

***************************************************************************/

#ifndef AT_H
#define AT_H

DRIVER_INIT( atcga );
DRIVER_INIT( at386 );
DRIVER_INIT( at586 );

DRIVER_INIT( at_vga );
DRIVER_INIT( ps2m30286 );

MACHINE_RESET( at );
MACHINE_RESET( at_vga );

#endif /* AT_H */
