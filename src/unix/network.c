#include "xmame.h"

#ifdef MAME_NET
enum {MASTER=1,SLAVE};
enum {IDENTIFY, SYNC};
#define NAME_LENGTH 256
#define PORT_DATA 9000

static int netkeymap = 0;
static int players = 0;
static char *mastername = NULL;
#endif

struct rc_option network_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
#ifdef MAME_NET
   { "Network Related", NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "master",		NULL,			rc_int,		&players,
     NULL,		1,			4,		NULL,
     "Enable master mode. Set number of players" },
   { "slave",		NULL,			rc_string,	&mastername,
     NULL,		0,			0,		NULL,
     "Enable slave mode. Set master hostname" },
   { "netmapkey",	NULL,			rc_bool,	&netkeymap,
     "0",		0,			0,		NULL,
     "When enabled all players use the player 1 keys. For use with *real* multiplayer games" },
#endif
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

#ifdef MAME_NET
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef svgalib
#include <vgakeyboard.h>
#endif
#include "driver.h"

static int socks[4];
static struct sockaddr_in names[4];
static int player;
static unsigned char keymap[128]; /* table to map network keys to player 1 */
static int timeout = 60;

static int init_master_socket(void)
{
	struct hostent *hp;
	char hname[NAME_LENGTH];

        fprintf(stderr_file, "XMame in network Master Mode\nWaiting for %d players.\n", players-1);
	gethostname(hname, 256);

	/* socket creation */
	if ((socks[0] = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		fprintf(stderr_file, "init master : Can't initialise socket\n");
		return(OSD_NOT_OK);
	}

	/* Assign domain and port number */
	memset(&names[0], 0, sizeof(names[0]));
	names[0].sin_family = AF_INET;
	names[0].sin_port = PORT_DATA;
	
	/* Assign IP address */
	if ((hp = gethostbyname(hname)) == NULL)
	{
		fprintf(stderr_file, "init master : gethostbyname error\n");
		return(OSD_NOT_OK);
	}
	memcpy(&(names[0].sin_addr.s_addr), hp->h_addr, hp->h_length);

	/* bind socket */
	if (bind(socks[0], (struct sockaddr *)&names[0], sizeof(names[0])) == -1)
	{
		fprintf(stderr_file, "init master : Bind failure.\n");
		return(OSD_NOT_OK);
	}

	return(OSD_OK);
}

static int init_slave_sockets(void)
{
        struct hostent *hp;
	char hname[NAME_LENGTH];

        fprintf(stderr_file, "Slave Mode; Registering to Master %s\n", mastername);
        
        /* socket creation */
        if ((socks[1] = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
                fprintf(stderr_file, "init slave : Can't initialise socket\n");
                return(OSD_NOT_OK);
        }

        /* Assign domain and port number */
        memset(&names[1], 0, sizeof(names[1]));
        names[1].sin_family = AF_INET;
        names[1].sin_port = PORT_DATA;

        /* Assign IP address */
        if ((hp = gethostbyname(mastername)) == NULL)
        {
                fprintf(stderr_file, "init slave : gethostbyname error\n");
                return(OSD_NOT_OK);
        }
        memcpy(&(names[1].sin_addr.s_addr), hp->h_addr, hp->h_length);

        gethostname(hname, 256);

        /* socket creation */
        if ((socks[0] = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
                fprintf(stderr_file, "init slave : Can't initialise socket\n");
                return(OSD_NOT_OK);
        }

        /* Assign domain and port number */
        memset(&names[0], 0, sizeof(names[0]));
        names[0].sin_family = AF_INET;
        names[0].sin_port = PORT_DATA+1;

        /* Assign IP address */
        if ((hp = gethostbyname(hname)) == NULL)
        {
                fprintf(stderr_file, "init slave : gethostbyname error\n");
                return(OSD_NOT_OK);
        }
        memcpy(&(names[0].sin_addr.s_addr), hp->h_addr, hp->h_length);

        /* bind socket */
        if (bind(socks[0], (struct sockaddr *)&names[0], sizeof(names[0])) == -1)
        {
                fprintf(stderr_file, "init slave : Bind failure.\n");
                return(OSD_NOT_OK);
        }

	return(OSD_OK);
}

static int receive_msg(void *msg, int size)
{
        unsigned int lg = 0;
        fd_set rfds;
        struct timeval tv;
        
        /* watch socks[0] to see if it has any input */
        FD_ZERO(&rfds);
        FD_SET(socks[0], &rfds);
        
        /* Wait up to timeout seconds. */
        tv.tv_sec  = timeout;
        tv.tv_usec = 0;

        if (select(socks[0] + 1, &rfds, NULL, NULL, &tv)==0)
        {
        	fprintf(stderr_file, "Error: timeout (%d secs) while receiving message.\n", timeout);
                return OSD_NOT_OK;
        }
        
        if (recvfrom(socks[0], msg, size, 0, NULL, &lg) == -1)
        {
        	fprintf(stderr_file, "Error: socket error receiving message.\n");
                return OSD_NOT_OK;
        }
        return OSD_OK;
}

static int send_msg(void *msg, int size)
{
	int i;

	switch(netstate)
	{
	case MASTER:
		for(i=1;i<players;i++)
		{
			if (sendto(socks[i], msg, size, 0, (struct sockaddr *)&names[i], sizeof(names[i])) == -1)
			{
				fprintf(stderr_file, "Error: socket error sending message.\n");
				return OSD_NOT_OK;
			}
		}
		break;
	case SLAVE:
		if (sendto(socks[1], msg, size, 0, (struct sockaddr *)&names[1], sizeof(names[1])) == -1)
		{
			fprintf(stderr_file, "Error: socket error sending message.\n");
			return OSD_NOT_OK;
		}
		break;
	}
	return OSD_OK;
}

static int add_slave(char *host, int *sock, struct sockaddr_in *name)
{
        struct hostent *hp;

        /* socket creation */
        if ((*sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
                fprintf(stderr_file, "Error: can't initialise socket\n");
                return(OSD_NOT_OK);
        }

        /* Assign domain and port number */
        memset(name, 0, sizeof(struct sockaddr_in));
        name->sin_family = AF_INET;
        name->sin_port   = PORT_DATA+1;

        /* Assign IP address */
        if ((hp = gethostbyname(host)) == NULL)
        {
                fprintf(stderr_file, "Error: gethostbyname error\n");
                return(OSD_NOT_OK);
        }
        memcpy(&name->sin_addr.s_addr, hp->h_addr, hp->h_length);

	return(OSD_OK);
}

static int register_to_master(void)
{
	char hname[NAME_LENGTH];

	gethostname(hname, NAME_LENGTH);
	if (sendto(socks[1], hname, NAME_LENGTH, 0, (struct sockaddr *)&names[1], sizeof(names[1])) == -1)
	{
		fprintf(stderr_file, "Error: socket error sending registration to master");
		return OSD_NOT_OK;
	}
	if (receive_msg(&player, sizeof(player)) != OSD_OK) return OSD_NOT_OK;
	fprintf(stderr_file, "Registered as player %d\n", player);
	return OSD_OK;
}

static int wait_registration(void)
{
	char hname[NAME_LENGTH]; /* slave host name */
	int i;

	for(i=1;i<players;i++)
	{
		player=i+1;
		if (receive_msg(hname, NAME_LENGTH) != OSD_OK)
		{
			fprintf(stderr_file, "Error: Can't receive registration from player %d\n", player);
			return OSD_NOT_OK;
		}
		if (add_slave(hname, &socks[i], &names[i])!=OSD_OK) return OSD_NOT_OK;
		if (sendto(socks[i], &player, sizeof(player), 0, (struct sockaddr *)&names[i], sizeof(names[i]))==-1)
		{
			fprintf(stderr_file, "Error: socket error sending registration to player %d\n", player);
			return OSD_NOT_OK;
		}
		fprintf(stderr_file, "%s registered successfully as player %d.\n", hname, player);
	}
	player = 1;
	return OSD_OK;
}

int default_key(const struct InputPort *in);

static int net_map_key(int keycode, int playermask)
{
  struct InputPort *in = Machine->input_ports, *start = Machine->input_ports;
  int port, found, event = 0;
 
  if (in->type == IPT_END) return keycode;   /* nothing to do */
 
    /* make sure the InputPort definition is correct */
  if (in->type != IPT_PORT)
    {
      if (errorlog) fprintf(errorlog,"Error in InputPort definition: expecting PORT_START\n");
      return keycode;
    }
  else start = ++in;
 
  found = 0;
  port = 0;
  while ((found ==0) && (in->type != IPT_END) && (port < MAX_INPUT_PORTS))
    {
      while (in->type != IPT_END && in->type != IPT_PORT)
        {
          if (default_key(in)==keycode)
            {
              event = in->type;
              found = 1;
              break;
            }
          in++;
        }
      if (found == 0)
      {
        port++;
        if (in->type == IPT_PORT) in++;
      }
    }
 
  if (found == 0) return keycode;
 
  in = start;
  port = 0;
  while (in->type != IPT_END && port < MAX_INPUT_PORTS)
    {
      while (in->type != IPT_END && in->type != IPT_PORT)
        {
          if ((in->type & IPF_PLAYERMASK)== playermask)
            if ((in->type & (~IPF_MASK)) == (event & (~IPF_MASK)))
              {
                return default_key(in);
              }
          in++;
        }
 
      port++;
      if (in->type == IPT_PORT) in++;
    }
 
  return keycode;
}

static void build_keymap(void)
{
  int i, keycode, playermask = 0;
  
  switch(player)
  {
    case 1:
      playermask = IPF_PLAYER1;
      break;
    case 2:
      playermask = IPF_PLAYER2;
      break;
    case 3:
      playermask = IPF_PLAYER3;
      break;
    case 4:
      playermask = IPF_PLAYER4;
      break;
  }
  
  memset(keymap,0,128*sizeof(unsigned char));
  
  for(i=0;i<128;i++)
  {
    keymap[net_map_key(i, playermask)] = i;
  }
}


/*
 * get key tables from slaves
 */
void osd_net_sync(void)
{
    int i,j;
    static char net_key[128];
    
    if (!key) return;
    
    if (netkeymap)
    {
      for(i=0; i<128; i++)
      {
        global_key[i] = local_key[keymap[i]];
      }
    }
    
    switch(netstate)
    {
      case MASTER:
        if(!netkeymap) memcpy(global_key, local_key, 128*sizeof(unsigned char));
        for(i=1;i<players;i++)
        {
                if (receive_msg(net_key, 128*sizeof(unsigned char)) != OSD_OK)
                	goto drop_to_single_player;
                for(j=0;j<=OSD_MAX_KEY;j++)
                	global_key[j] |= net_key[j];
        }
	if (send_msg(global_key, 128*sizeof(unsigned char)) != OSD_OK)
                goto drop_to_single_player;
	break;
      case SLAVE:
        if (netkeymap)
        {
		if (send_msg(global_key, 128*sizeof(unsigned char)) != OSD_OK)
                	goto drop_to_single_player;
	}
        else
        {
		if (send_msg(local_key, 128*sizeof(unsigned char)) != OSD_OK)
                	goto drop_to_single_player;
	}
	if (receive_msg(global_key, 128*sizeof(unsigned char))!=OSD_OK)
                goto drop_to_single_player;
	break;
    }
    /* after the first successfull sync it should be safe to lower the
       timeout to 5 seconds */
    timeout = 5;
    return;
    
drop_to_single_player:    
    fprintf(stderr_file, "Lost network connection, continuing in single player mode\n");
    osd_net_close();
    netstate=0;
    key=local_key;
}

/*
 * Close all opened sockets
 */
void osd_net_close(void)
{
	int i;

	switch(netstate)
	{
	    case MASTER:
		for(i=0;i<(players-1);i++) close(socks[i]);
		break;
	    case SLAVE:
		close(socks[0]);
		close(socks[1]);
		break;
	}
}

/*
 * Initialise network
 * - the master opens a socket and waits for slaves
 * - the slaves register to the master
 */
int osd_net_init(void)
{
	netstate = 0;
	
	if (players) netstate = MASTER;
	if (mastername)
	{
	   if(netstate==MASTER)
	   {
	      fprintf(stderr_file, "error: can't be Slave and Master\n");
	      return OSD_NOT_OK;
	   }
	   netstate = SLAVE;
	}
	
	switch(netstate)
	{
	    case 0:
	        return OSD_OK;
	    case MASTER:
		if (init_master_socket() != OSD_OK)
		    return OSD_NOT_OK;
		if (wait_registration() != OSD_OK)
		    return OSD_NOT_OK;
		break;
	    case SLAVE:
		if (init_slave_sockets() != OSD_OK)
		    return OSD_NOT_OK;
		if (register_to_master() != OSD_OK)
		    return OSD_NOT_OK;
		break;
	}
	if (netkeymap) build_keymap();
	/* mouse is not supported in network mode */
	use_mouse = FALSE;
	return OSD_OK;
}

#endif /* MAME_NET */
