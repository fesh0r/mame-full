#ifndef I8255_H
#define I8255_H

typedef struct I8255_INTERFACE
{
	int (*porta_r)(void);
	int (*portb_r)(void);
	int (*portc_r)(void);
	void	(*porta_w)(int);
	void	(*portb_w)(int);
	void	(*portc_w)(int);
} I8255_INTERFACE;

extern void	i8255_init(I8255_INTERFACE *);
extern void	i8255_porta_w(int data);
extern void	i8255_portb_w(int data);
extern void	i8255_portc_w(int data);
extern void	i8255_control_port_w(int data);
extern int	i8255_porta_r(void);
extern int	i8255_portb_r(void);
extern int	i8255_portc_r(void);
extern void	i8255_set_porta(int data);
extern void	i8255_set_portb(int data);
extern void	i8255_set_portc(int data);

typedef struct I8255
{
	/* data that CPU sees when it reads/writes to the port */
        unsigned char   porta_data;
        unsigned char    portb_data;
        unsigned char    portc_data;

	/* control port data */
        unsigned char    control_port_data;
} I8255;



#endif

