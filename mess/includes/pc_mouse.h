
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { TYPE_MICROSOFT_MOUSE, TYPE_MOUSE_SYSTEMS } PC_MOUSE_PROTOCOL;

void pc_mouse_handshake_in(int n, int data);

// set base for input port
void pc_mouse_set_input_base(int base);
void	pc_mouse_set_protocol(PC_MOUSE_PROTOCOL protocol);
void pc_mouse_set_serial_port(int uart_index);
void	pc_mouse_initialise(void);

#define INPUT_MOUSE_SYSTEMS \
	PORT_START	/* IN12 */ \
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Mouse Left Button", KEYCODE_Q, JOYCODE_1_BUTTON1) \
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Mouse Middle Button", KEYCODE_W, JOYCODE_1_BUTTON2) \
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Mouse Right Button", KEYCODE_E, JOYCODE_1_BUTTON3) \
\
	PORT_START /* Mouse - X AXIS */ \
	PORT_ANALOGX( 0xfff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE ) \
\
	PORT_START /* Mouse - Y AXIS */ \
	PORT_ANALOGX( 0xfff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE ) \

#define INPUT_MICROSOFT_MOUSE \
	PORT_START	/* IN12 */ \
	PORT_BITX( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Mouse Button Left",	CODE_NONE,		JOYCODE_1_BUTTON1) \
	PORT_BITX( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD,	"Mouse Button Right",	CODE_NONE,		JOYCODE_1_BUTTON2) \
\
	PORT_START /* IN13 mouse X */ \
	PORT_ANALOGX(0xfff,0,IPT_TRACKBALL_X,100,0,0,0xfff,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT) \
\
	PORT_START /* IN14 mouse Y */ \
	PORT_ANALOGX(0xfff,0,IPT_TRACKBALL_Y,100,0,0,0xfff,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN) \

#define PC_NO_MOUSE \
        PORT_START      /* IN12 */\
        PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )\
        PORT_START      /* IN13 */\
        PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )\
        PORT_START      /* IN14 */\
        PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )


#ifdef __cplusplus
}
#endif
