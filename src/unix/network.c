#include "xmame.h"

#ifdef MAME_NET

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "driver.h"

#define PROTOCOL_DEBUG 2

enum net_role {
    NONE,
    MASTER,
    SLAVE
};

static enum net_role _current_net_role = NONE;

#define NET_MAX_PLAYERS 4
#define MASTER_INPUT_PORT 9000
#define SLAVE_INPUT_PORT 9001
#define MAX_MSG_LEN 100

enum net_msg_type {
    JOIN,
    ACCEPT,
    REFUSE,
    ACK,
    LOCAL_STATE,
    QUIT
};

/* Bitmap indices for the master's ACCEPT Message */
#define CONFIG_BITMAP_PARALLELSYNC 0

#define MSG_MAGIC_HEADER "XNM"
#define PROTOCOL_VERSION "N0.5"
static char _truncated_build_version[10];

struct msg_info_t {
    enum net_msg_type msg_type;
    unsigned sequence;
    unsigned char msg[MAX_MSG_LEN];
    unsigned char *data_start;
    int data_len;
} _scratch_msg_info, _outbound_state_msg_info[2];
#define _current_outbound_state_msg_info \
        _outbound_state_msg_info[_current_frame_sequence % 2]
#define _previous_outbound_state_msg_info \
        _outbound_state_msg_info[(_current_frame_sequence - 1) % 2]

static unsigned short _saved_input_port_values_1[MAX_INPUT_PORTS];
static unsigned short _saved_input_port_values_2[MAX_INPUT_PORTS];
static unsigned short *_saved_input_port_values[2] = {
    _saved_input_port_values_1,
    _saved_input_port_values_2 };
#define _current_saved_input_port_values \
        _saved_input_port_values[_current_frame_sequence % 2]
#define _previous_saved_input_port_values \
        _saved_input_port_values[(_current_frame_sequence - 1) % 2]

static int _input_remap = 0;
static int _parallel_sync = 0;

#if PROTOCOL_DEBUG >= 1
unsigned _repeated_outbound_state_count = 0;
#endif

static int _current_player_count = 0;
struct peer_info_t {
    char name[MAX_MSG_LEN];
    int player_number;
    int active;
    struct sockaddr_in addr;
    int ack_expected;
    int state_expected;
    unsigned time_since_resend;
    struct msg_info_t last_outbound_ackable;
    struct msg_info_t early_inbound_msg_info;
} _peer_info[NET_MAX_PLAYERS];

static int _original_player_count = 0;

static int _state_exchange_active = 0;

/* For slave only */
const char *_master_hostname = NULL;
static unsigned char _player_number;
#define _master_info (_peer_info[0])

struct input_bit_id {
    unsigned port_index;
    unsigned short mask;
};
struct input_mapping {
    struct input_bit_id source;
    struct input_bit_id dest;
};
static struct input_mapping input_map[MAX_INPUT_PORTS * sizeof(unsigned short)];
unsigned input_mapping_count;

#endif

struct rc_option network_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
#ifdef MAME_NET
   { "Network Related", NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "master",		NULL,			rc_int,		&_original_player_count,
     NULL,		2,			NET_MAX_PLAYERS,NULL,
     "Enable master mode. Set number of players" },
   { "slave",		NULL,			rc_string,	&_master_hostname,
     NULL,		0,			0,		NULL,
     "Enable slave mode. Set master hostname" },
   { "netmapkey",	NULL,			rc_bool,	&_input_remap,
     "0",		0,			0,		NULL,
     "When enabled all players use the player 1 keys. For use with *real* multiplayer games" },
   { "parallelsync",    NULL,			rc_bool,	&_parallel_sync,
     "0",		0,			0,		NULL,
     "Perform network input sync in advance:  Causes ~ 16 ms input delay but more suitable for relatively slow machines" },
#endif
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

#ifdef MAME_NET

static int _output_socket;
static int _input_socket;

unsigned _current_frame_sequence = 0;

static int tv_subtract(struct timeval *tv1, struct timeval *tv2)
{
    int result;
    result = (tv1->tv_sec - tv2->tv_sec) * 1000000;
    result += (tv1->tv_usec - tv2->tv_usec);
    return result;
}

#if PROTOCOL_DEBUG >= 1
struct timeval _sync_start_time;
struct timeval _start_time;

unsigned _min_sync_time = 0;
unsigned _max_sync_time = 0;
unsigned _total_sync_time = 0;

static void
_inbound_sync(unsigned short input_port_deviations[MAX_INPUT_PORTS],
	      unsigned sync_sequence);

static void record_sync_start()
{
    gettimeofday(&_sync_start_time, NULL);
}

static void record_sync_end()
{
    unsigned sync_time;
    struct timeval sync_end_time;
    gettimeofday(&sync_end_time, NULL);
    sync_time = tv_subtract(&sync_end_time, &_sync_start_time) / 1000;
    if (_min_sync_time == 0 || (sync_time < _min_sync_time)) {
	_min_sync_time = sync_time;
    }
    if (_max_sync_time == 0 || (sync_time > _max_sync_time)) {
	_max_sync_time = sync_time;
    }
    _total_sync_time += sync_time;
}
#endif

static int _init_sockets(void)
{
    struct sockaddr_in input_addr;
    
    if ((_output_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	fprintf(stderr_file,
		"init slave : Can't initialise output socket (%s)\n",
		strerror(errno));
	return(OSD_NOT_OK);
    }

    if ((_input_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	fprintf(stderr_file,
		"init slave : Can't initialise input socket (%s)\n",
		strerror(errno));
	return(OSD_NOT_OK);
    }

    /* Assign domain and port number */
    /* Strictly speaking there's no reason for the master and slave ports
       to be different, but it does make it possible to run both on the
       same machine, which is useful for debuggin */
    input_addr.sin_family = AF_INET;
    input_addr.sin_addr.s_addr = INADDR_ANY;
    if (_current_net_role == MASTER) {
	input_addr.sin_port = htons(MASTER_INPUT_PORT);
    } else {
	input_addr.sin_port = htons(SLAVE_INPUT_PORT);
    }

    /* bind socket */
    if (bind(_input_socket, (struct sockaddr *)&input_addr, sizeof(input_addr)) == -1) {
	fprintf(stderr_file,
		"Input socket bind failure (%s).\n",
		strerror(errno));
	return(OSD_NOT_OK);
    }
    return(OSD_OK);
}

static int rcv_msg(int fd,
		   struct msg_info_t *msg_info, 
		   struct sockaddr_in *source_addr,
		   unsigned *timeout)
{
    int failure = 0;
    unsigned socklen = 0;
    fd_set scratch_fds;
    struct timeval rcv_start_tv, rcv_end_tv, timeout_tv;
    struct timeval *timeout_tv_p = NULL;
    int msg_len;

    if (*timeout > 0) {
	timeout_tv.tv_sec = *timeout / 1000;
	timeout_tv.tv_usec = (*timeout % 1000) * 1000;
	timeout_tv_p = &timeout_tv;
    }

    if (source_addr != 0) {
	socklen = sizeof(*source_addr);
    }

    gettimeofday(&rcv_start_tv, NULL);
    do {
	FD_ZERO(&scratch_fds);
	FD_SET(fd, &scratch_fds);
	    
	if (select(fd + 1, &scratch_fds, NULL, NULL, timeout_tv_p) == 0) {
	    *timeout = 0;
	    return OSD_NOT_OK;
	}
        
	msg_len = recvfrom(fd,
			   msg_info->msg,
			   sizeof(msg_info->msg),
			   0,
			   (struct sockaddr *)source_addr,
			   &socklen);
	if (msg_len < 0) {
	    fprintf(stderr_file,
		    "Error: socket error receiving message (%s).\n",
		    strerror(errno));
	    failure = 1;
	}
	/* Packets without the magic header are discarded without error;
	   they are assumed to be unrelated traffic */
    } while (msg_len < 4 || memcmp(msg_info->msg, MSG_MAGIC_HEADER, 3) != 0);
	
    if (! failure) {
	msg_info->msg_type = msg_info->msg[3];
	switch (msg_info->msg_type) {
	case JOIN:
	case ACCEPT:
	case REFUSE:
	    msg_info->data_start = msg_info->msg + 4;
	    msg_info->data_len = msg_len - 4;
	    break;
	case LOCAL_STATE:
	case QUIT:
	case ACK:
	    if (msg_len < 8) {
		fprintf(stderr_file,
			"Error: received truncated packet (size %i).\n",
			msg_len);
		failure = 1;
	    } else {
		msg_info->sequence =
		    (unsigned)ntohl(*((long int *)(msg_info->msg + 4)));
		msg_info->data_start = msg_info->msg + 8;
		msg_info->data_len = msg_len - 8;
	    }
	    break;
	}
    }
    gettimeofday(&rcv_end_tv, NULL);
    *timeout -= tv_subtract(&rcv_end_tv, &rcv_start_tv) / 1000;
    if (failure) {
	return OSD_NOT_OK;
    }
    return OSD_OK;
}

void _prime_msg(struct msg_info_t *msg_info)
{
    memcpy(msg_info->msg, MSG_MAGIC_HEADER, 3);
    msg_info->msg[3] = (unsigned char)msg_info->msg_type;
    switch (msg_info->msg_type) {
    case JOIN:
    case ACCEPT:
    case REFUSE:
	msg_info->data_start = msg_info->msg + 4;
	break;
    case QUIT:
    case ACK:
    case LOCAL_STATE:
	(*(long int *)(msg_info->msg + 4)) = ntohl(msg_info->sequence);
	msg_info->data_start = msg_info->msg + 8;
	break;
    }
    msg_info->data_len = 0;
}

int _write_to_msg(struct msg_info_t *msg_info,
		  const void *new_data,
		  unsigned new_data_len)
{
    if ((msg_info->data_start - msg_info->msg) + msg_info->data_len + new_data_len > MAX_MSG_LEN)
    {
	fprintf(stderr_file, "Error: message overflow\n");
	return OSD_NOT_OK;
    }
    memcpy(msg_info->data_start + msg_info->data_len, new_data, new_data_len);
    msg_info->data_len += new_data_len;
    return OSD_OK;
}

static int _send_msg(int sock_fd,
		     struct msg_info_t *msg_info,
		     struct sockaddr_in *dest)
{
    if (sendto(sock_fd,
	       msg_info->msg,
	       (msg_info->data_start - msg_info->msg) + msg_info->data_len,
	       0,
	       (struct sockaddr *)dest,
	       sizeof(*dest)) < 0)
    {
	fprintf(stderr_file, "Error: socket error sending message (%s).\n", strerror(errno));
	return OSD_NOT_OK;
    }
    return OSD_OK;
}

static void _copy_msg(struct msg_info_t *src_msg,
		      struct msg_info_t *dest_msg)
{
    memcpy(dest_msg, src_msg, sizeof(struct msg_info_t));
    if (dest_msg->data_start != NULL) {
	dest_msg->data_start =
	    dest_msg->msg + (src_msg->data_start - src_msg->msg);
    }
}

static int _register_to_master(void)
{
    struct hostent *master_hostent;
    char scratch[MAX_MSG_LEN];
    unsigned timeout;
    unsigned peer_index, peer_player_number;
    unsigned msg_data_left;
    unsigned char *msg_read_pointer;

    fprintf(stderr_file, "Slave Mode; Registering to Master %s\n", _master_hostname);
        
    /* Assign IP address */
    if ((master_hostent = gethostbyname(_master_hostname)) == NULL) {
	fprintf(stderr_file, "init slave : gethostbyname error\n");
	return(OSD_NOT_OK);
    }
	
    sprintf(_master_info.name, "Master: %s", _master_hostname);
    _master_info.player_number = 1;
    _master_info.active = 1;
    _master_info.addr.sin_family = AF_INET;
    memcpy(&(_master_info.addr.sin_addr.s_addr),
	   master_hostent->h_addr,
	   master_hostent->h_length);
    _master_info.addr.sin_port = htons(MASTER_INPUT_PORT);

    gethostname(scratch, MAX_MSG_LEN);

    _scratch_msg_info.msg_type = JOIN;
    _prime_msg(&_scratch_msg_info);
    _write_to_msg(&_scratch_msg_info, scratch, strlen(scratch));
    sprintf(scratch, "/%d", getpid());
    _write_to_msg(&_scratch_msg_info, scratch, strlen(scratch) + 1);
    _write_to_msg(&_scratch_msg_info,
		  _truncated_build_version,
		  strlen(_truncated_build_version) + 1);
    _write_to_msg(&_scratch_msg_info, PROTOCOL_VERSION, strlen(PROTOCOL_VERSION) + 1);
    _write_to_msg(&_scratch_msg_info,
		  Machine->gamedrv->name,
		  strlen(Machine->gamedrv->name) + 1);

    do {
	_send_msg(_output_socket, &_scratch_msg_info, &_master_info.addr);
    } while (timeout = 3000,
	     rcv_msg(_input_socket, &_scratch_msg_info, NULL, &timeout) != OSD_OK);

    if (_scratch_msg_info.msg_type == REFUSE) {
	fprintf(stderr_file, "Master \"%s\" refused registration\n", _master_hostname);
	if (_scratch_msg_info.data_len > 0) {
	    fprintf(stderr_file, "Reason given:  %.50s\n", _scratch_msg_info.data_start);
	} else {
	    fprintf(stderr_file, "No reason given\n");
	}
	return OSD_NOT_OK;
    }
    else if (_scratch_msg_info.msg_type != ACCEPT) {
	fprintf(stderr_file,
		"Master \"%s\" returned bogus message type %d\n",
		_master_hostname,
		_scratch_msg_info.msg_type);
	return OSD_NOT_OK;
    }

    _player_number = _scratch_msg_info.data_start[0];
    _parallel_sync =
	(_scratch_msg_info.data_start[1] & (1 << CONFIG_BITMAP_PARALLELSYNC));

    _current_player_count = 2;
    peer_index = 1;
    peer_player_number = 1;
    msg_read_pointer = _scratch_msg_info.data_start + 2;
    msg_data_left = _scratch_msg_info.data_len - 2;
    while (msg_data_left > 0) {
	peer_player_number += 1;
	if (peer_player_number == _player_number) {
	    peer_player_number += 1;
	}
	_peer_info[peer_index].player_number = peer_player_number;
	strncpy(_peer_info[peer_index].name,
		msg_read_pointer,
		msg_data_left);
	msg_data_left -= (strlen(msg_read_pointer) + 1);
	msg_read_pointer += (strlen(msg_read_pointer) + 1);
	_peer_info[_current_player_count].active = 1;

	if (msg_data_left == 0) {
	    fprintf(stderr_file,
		    "Master returned malformed accept message:"
		    " no IP address for player %d\n",
		    peer_player_number);
	    return OSD_NOT_OK;
	}
	if (! inet_aton(msg_read_pointer,
			&(_peer_info[peer_index].addr.sin_addr)))
	{
	    fprintf(stderr_file,
		    "Master returned malformed accept message:"
		    " bogus IP address \"%s\" for player %d\n",
		    msg_read_pointer,
		    peer_player_number);
	    return OSD_NOT_OK;
	}
	msg_data_left -= (strlen(msg_read_pointer) + 1);
	msg_read_pointer += (strlen(msg_read_pointer) + 1);

	_peer_info[peer_index].early_inbound_msg_info.sequence =
	    (unsigned)-1;

	_current_player_count += 1;
	peer_index += 1;
    }
    _original_player_count = _current_player_count;
    _scratch_msg_info.msg_type = ACK;
    _scratch_msg_info.sequence = 0;
    _prime_msg(&_scratch_msg_info);
    _send_msg(_output_socket, &_scratch_msg_info, &(_master_info.addr));

    fprintf(stderr_file,
	    "Registration as player %d confirmed\n",
	    (unsigned)_player_number);
    fprintf(stderr_file,
	    "Protocol config: parallelsync %s\n",
	    (_parallel_sync ? "enabled" : "disabled"));
#if PROTOCOL_DEBUG >= 1
    gettimeofday(&_start_time, NULL);
#endif
    return OSD_OK;
}


static unsigned char _peer_index_for_addr(struct in_addr *addr)
{
    unsigned char i = 0;
    while (i < _original_player_count - 1) {
	if (memcmp(addr, &(_peer_info[i].addr.sin_addr), sizeof(*addr)) == 0) {
	    break;
	}
	i += 1;
    }
    if (i == _original_player_count - 1) {
	return (unsigned char)-1;
    } else {
	return i;
    }
}

#if 0
static unsigned char _find_free_peer_index()
{
    unsigned char i = 0;
    while (i < _original_player_count - 1 && _peer_info[i].active) {
	i += 1;
    }
    if (i == _original_player_count - 1) {
	return (unsigned char)-1;
    } else {
	return i;
    }
}
#endif

static int _await_slave_registrations(void)
{
    unsigned char slave_index;
    struct sockaddr_in source_addr;
    unsigned timeout;
    unsigned char scratch_msg_data[MAX_MSG_LEN];
    const char * comparison_string[] = { _truncated_build_version,
					 PROTOCOL_VERSION,
					 Machine->gamedrv->name,
					 NULL };
    const char * comparison_string_name[] = { "build version",
					      "protocol version",
					      "game name" };
    unsigned i = 0;

    fprintf(stderr_file,
	    "XMame in network Master Mode\n"
	    "Waiting for %d more players.\n",
	    _original_player_count - 1);

    _current_player_count = 1; /* Count only master to being with */

    for (slave_index = 0;
	 slave_index < _original_player_count - 1;
	 slave_index++)
    {
	_peer_info[slave_index].active = 0;
	_peer_info[slave_index].early_inbound_msg_info.sequence = (unsigned)-1;
    }
    while (_current_player_count < _original_player_count) {
	unsigned msg_data_left;
	unsigned char *msg_read_pointer;

	if (timeout = 0,
	    rcv_msg(_input_socket,
		    &_scratch_msg_info,
		    &source_addr,
		    &timeout) == OSD_NOT_OK)
	{
	    fprintf(stderr_file,
		    "Error: Registration from slave failed (%s)\n",
		    strerror(errno));
	    continue;
	}
	if (_scratch_msg_info.msg_type != JOIN) {
	    fprintf(stderr_file,
		    "Error: Unexpected message type (%d)"
		    " while awaiting registrations\n",
		    _scratch_msg_info.msg_type);
	    continue;
	}
	    
	slave_index = _peer_index_for_addr(&source_addr.sin_addr);

	if (slave_index == (unsigned char )-1) {
	    slave_index = _current_player_count - 1;
	    _peer_info[slave_index].addr.sin_family = AF_INET;
	    memcpy(&(_peer_info[slave_index].addr.sin_addr),
		   &(source_addr.sin_addr),
		   sizeof(_peer_info[slave_index].addr.sin_addr));
	    _peer_info[slave_index].addr.sin_port = htons(SLAVE_INPUT_PORT);
	} else {
	    /* When a known slave re-registers (due to timeout most often),
	       the master must double-check its handshake parameters
	       (MAME/net version, game name) because it's possible they've
	       changed -- this happens if it's a new process on the same
	       host, which the master has know way of detecting */
	    _peer_info[slave_index].active = 0; 
	}

	msg_data_left = _scratch_msg_info.data_len;
	msg_read_pointer = _scratch_msg_info.data_start;
	    
	strncpy(_peer_info[slave_index].name, msg_read_pointer, msg_data_left);
	msg_data_left -= (strlen(_peer_info[slave_index].name) + 1);
	msg_read_pointer += (strlen(_peer_info[slave_index].name) + 1);

	while (comparison_string[i] != NULL) {
	    if (strncmp(msg_read_pointer,
			comparison_string[i],
			msg_data_left) != 0)
	    {
		_scratch_msg_info.msg_type = REFUSE;
		_prime_msg(&_scratch_msg_info);
		sprintf(scratch_msg_data,
			"Wrong %s; need %s not %.10s",
			comparison_string_name[i],
			comparison_string[i],
			msg_read_pointer);
		_write_to_msg(&_scratch_msg_info,
			      scratch_msg_data,
			      strlen(scratch_msg_data));
		_send_msg(_output_socket,
			  &_scratch_msg_info,
			  &(_peer_info[slave_index].addr));
		break;
	    }
	    msg_data_left -= (strlen(comparison_string[i]) + 1);
	    msg_read_pointer += strlen(comparison_string[i]) + 1;
	    i += 1;
	}
	if (comparison_string[i] == NULL) {
	    _peer_info[slave_index].active = 1;
	    if (slave_index == _current_player_count - 1) {
		_current_player_count += 1;
	    }
	} 
    }

    /* At this point the master has received enough registrations to
       start the game, so it sends out accept messages */
    for (slave_index = 0;
	 slave_index < _current_player_count - 1;
	 slave_index++)
    {
	_peer_info[slave_index].last_outbound_ackable.msg_type = ACCEPT;
	_peer_info[slave_index].last_outbound_ackable.sequence = 0;
	_prime_msg(&(_peer_info[slave_index].last_outbound_ackable));
	scratch_msg_data[0] = slave_index + 2;
	scratch_msg_data[1] = (_parallel_sync << CONFIG_BITMAP_PARALLELSYNC);
	_write_to_msg(&(_peer_info[slave_index].last_outbound_ackable),
		      scratch_msg_data,
		      2);
	for (i = 0; i < _current_player_count - 1; i++) {
	    /* The accept message sent out to each slave includes the IP
	       address for every *other* slave */
	    if (i != slave_index) {
		char *ip_string = inet_ntoa(_peer_info[i].addr.sin_addr);
		_write_to_msg(&(_peer_info[slave_index].last_outbound_ackable),
			      _peer_info[i].name,
			      strlen(_peer_info[i].name) + 1);
		_write_to_msg(&(_peer_info[slave_index].last_outbound_ackable),
			      ip_string,
			      strlen(ip_string) + 1);
	    }
	}
	    
	if (_send_msg(_output_socket,
		      &(_peer_info[slave_index].last_outbound_ackable),
		      &(_peer_info[slave_index].addr)) != OSD_OK)
	{
	    /* At this point it's too late to back out and start accepting
	       registrations again, but if there's a real problem with the
	       slave the sync timeout will cut it out and the game
	       should proceed with one fewer player */
	    fprintf(stderr_file,
		    "Error: socket error sending registration"
		    " to slave %d (%s)\n",
		    slave_index + 2,
		    strerror(errno));
	} else {
	    fprintf(stderr_file,
		    "%s registration accepted as player %d.\n",
		    _peer_info[slave_index].name,
		    slave_index + 2);
	    _peer_info[slave_index].active = 1;
	}

	_peer_info[slave_index].ack_expected = 1;
	_peer_info[slave_index].state_expected = 0;
    }
    _inbound_sync(NULL, 0);
#if PROTOCOL_DEBUG >= 1
    gettimeofday(&_start_time, NULL);
#endif
    return OSD_OK;
}

static struct input_map_reference_t {
    UINT32 playermask;
    UINT32 start;
    UINT32 coin;
} input_map_reference[] = { { IPF_PLAYER2, IPT_START2, IPT_COIN2 },
			    { IPF_PLAYER3, IPT_START3, IPT_COIN3 },
			    { IPF_PLAYER4, IPT_START4, IPT_COIN4 } };

static int _map_input_port(unsigned unmapped_bit_index,
			  struct input_bit_id *mapped_bit_id)
{
    unsigned candidate_port_index, candidate_bit_index;
    UINT32 unmapped_type = Machine->gamedrv->input_ports[unmapped_bit_index].type;
    UINT32 target_type;

    /* Map player n input bit to player 1, all others are no-op */
    if (_player_number == 1) {
	return 0;
    }
    if ((unmapped_type & IPF_PLAYERMASK) ==
	    input_map_reference[_player_number - 2].playermask)
    {
	target_type = ((unmapped_type & ~IPF_PLAYERMASK) | IPF_PLAYER1);
    }
    else if (unmapped_type == input_map_reference[_player_number - 2].start) {
	target_type = IPT_START1;
    }
    else if (unmapped_type == input_map_reference[_player_number - 2].coin) {
	target_type = IPT_COIN1;
    } else {
	return 0;
    }

    if (Machine->gamedrv->input_ports[0].type != IPT_PORT) {
        fprintf(stderr_file,
	        "Unable to build key map:  "
	        "Port definitions do not begin with IPT_PORT\n");        
        return 0;
    }
    for (candidate_bit_index = 1, candidate_port_index = 0;
         candidate_port_index < MAX_INPUT_PORTS &&
	     Machine->gamedrv->input_ports[candidate_bit_index].type != IPT_END;
         candidate_bit_index += 1)
    {
	if (Machine->gamedrv->input_ports[candidate_bit_index].type == IPT_PORT) {
	    candidate_port_index += 1;
	    continue;
	}
	if (Machine->gamedrv->input_ports[candidate_bit_index].type == target_type) {
	    mapped_bit_id->port_index = candidate_port_index;
	    mapped_bit_id->mask = Machine->gamedrv->input_ports[candidate_bit_index].mask;
	    return 1;
	}
    }
    /* Loop only exits if a mapped version of
       the input bit could not be found */
    return 0;
}

static int _build_net_keymap(void)
{
    unsigned unmapped_bit_index, unmapped_port_index;
    unsigned previous_mapping_index;

    input_mapping_count = 0;
    if (Machine->gamedrv->input_ports[0].type != IPT_PORT) {
        fprintf(stderr_file,
	        "Unable to build key map:  "
	        "Port definitions do not begin with IPT_PORT\n");        
        return 0;
    }
    for (unmapped_bit_index = 1, unmapped_port_index = 0;
         unmapped_port_index < MAX_INPUT_PORTS &&
	     Machine->gamedrv->input_ports[unmapped_bit_index].type != IPT_END;
         unmapped_bit_index += 1)
    {
	if (Machine->gamedrv->input_ports[unmapped_bit_index].type == IPT_PORT) {
	    unmapped_port_index += 1;
	    continue;
	}
	/* Check for duplicates */
	for (previous_mapping_index = 0;
	     previous_mapping_index < input_mapping_count;
	     previous_mapping_index++)
	{
	    if ((input_map[previous_mapping_index].source.port_index == unmapped_port_index) &&
		(input_map[previous_mapping_index].source.mask == 
		     Machine->gamedrv->input_ports[unmapped_bit_index].mask))
	    {
		goto skip_duplicate;
	    }
	}
	if (_map_input_port(unmapped_bit_index, &(input_map[input_mapping_count].dest))) {
	    input_map[input_mapping_count].source.port_index = unmapped_port_index;
	    input_map[input_mapping_count].source.mask =
		Machine->gamedrv->input_ports[unmapped_bit_index].mask;
	    input_mapping_count += 1;
	}
    skip_duplicate:
    }
    return 1;
}

static unsigned short _scratch_input_state[MAX_INPUT_PORTS];

void _remap_input_state(unsigned short input_state[MAX_INPUT_PORTS])
{
    unsigned mapping_index;
    for (mapping_index = 0;
	 mapping_index < input_mapping_count;
	 mapping_index++)
    {
	unsigned short source_mask = input_map[mapping_index].source.mask;
	unsigned short source_bit =
	    input_state[input_map[mapping_index].source.port_index] & source_mask;
	unsigned short dest_mask = input_map[mapping_index].dest.mask;
	unsigned short dest_bit = 
	    input_state[input_map[mapping_index].dest.port_index] & dest_mask;

	if (source_bit == 0) {
	    input_state[input_map[mapping_index].dest.port_index] &= ~dest_mask;
	} else {
	    input_state[input_map[mapping_index].dest.port_index] |= dest_mask;
	}
	if (dest_bit == 0) {
	    input_state[input_map[mapping_index].source.port_index] &= ~source_mask;
	} else {
	    input_state[input_map[mapping_index].source.port_index] |= source_mask;
	}
    }
}

void _outbound_sync(unsigned short input_port_values[MAX_INPUT_PORTS],
		    unsigned sync_sequence)
{
    unsigned i;
    unsigned peer_index;

    _current_outbound_state_msg_info.msg_type = LOCAL_STATE;
    _current_outbound_state_msg_info.sequence =	sync_sequence;
    _prime_msg(&(_current_outbound_state_msg_info));

    for (i = 0; i < MAX_INPUT_PORTS; i++) {
	unsigned short scratch = htons(input_port_values[i]);
	_write_to_msg(&(_current_outbound_state_msg_info),
		      &scratch,
		      sizeof(scratch));
    }
    
#if PROTOCOL_DEBUG >= 3
    fprintf(stderr_file,
	    "Sending state %d to peers\n",
	    _current_outbound_state_msg_info.sequence);
#endif
    for (peer_index = 0;
	 peer_index < _original_player_count - 1;
	 peer_index++)
    {
	if (_peer_info[peer_index].active) {
	    _send_msg(_output_socket,
		      &(_current_outbound_state_msg_info),
		      &(_peer_info[peer_index].addr));
	}
	_peer_info[peer_index].state_expected = 1;
    }
}

#define GIVEUP_TIMEOUT (15 * 1000)
#define RETRY_TIMEOUT 50

static void _handle_duplicate_message(struct msg_info_t *duplicate_msg_info,
				      struct peer_info_t *source_peer)
				      
{
    /* Peer seems to have have missed previous state */
#if PROTOCOL_DEBUG >= 2
    fprintf(stderr_file,
	    "Received dupe message type %d sequence %d from peer \"%s\"\n",
	    duplicate_msg_info->msg_type,
	    duplicate_msg_info->sequence,
	    source_peer->name);
#endif
    if (source_peer->time_since_resend >= RETRY_TIMEOUT) {
#if PROTOCOL_DEBUG >= 2
	fprintf(stderr_file,
		"... re-sending state sequence %d\n",
		_previous_outbound_state_msg_info.sequence);
#endif
	_send_msg(_output_socket,
		  &(_previous_outbound_state_msg_info),
		  &(source_peer->addr));
	source_peer->time_since_resend = 0;
#if PROTOCOL_DEBUG >= 1
	_repeated_outbound_state_count += 1;
#endif
    }
}

static void _handle_early_message(struct msg_info_t *early_msg_info,
				  struct peer_info_t *source_peer)
{
    /* A peer is already at the next frame and sending its
       state:  Copy it into a temporary space (unless this has already
       been done) and process it when ready */
    if (source_peer->early_inbound_msg_info.sequence != early_msg_info->sequence)
    {
	_copy_msg(early_msg_info,
		  &(source_peer->early_inbound_msg_info));
    }
}

/* Returns 1 if the message brings the peer completely up to date for
   the current sync sequence -- ie, neither ACKable nor state expected
   any longer */
static int
_process_inbound_message(struct msg_info_t *msg_to_process,
			 struct peer_info_t *source_peer,
			 unsigned sync_sequence,
			 unsigned short input_port_deviations[MAX_INPUT_PORTS])
{
    int result = 0;
    if (source_peer->state_expected) {
	/* All ACKables handled here */
	if (msg_to_process->msg_type == QUIT) {
	    /* ACKables with a sequence number in the future could be
	       stored and used later to save a message here and there.
	       This may or may not be worth the extra complexity in the
	       code; I'm not implementing it for now. */
	    if (msg_to_process->sequence == sync_sequence) {
		if (source_peer->active) {
		    fprintf(stderr_file,
			    "Peer \"%s\" quit\n",
			    source_peer->name);
		    source_peer->active = 0;
		    _current_player_count -= 1;
		    result = 1;
		}
		_scratch_msg_info.msg_type = ACK;
		_scratch_msg_info.sequence = sync_sequence;
		_prime_msg(&_scratch_msg_info);
		_send_msg(_output_socket,
			  &_scratch_msg_info,
			  &(source_peer->addr));
		if (_current_player_count == 1) {
		    osd_net_close();
		}
	    }
	}
	else if (msg_to_process->msg_type == LOCAL_STATE) {
#if PROTOCOL_DEBUG >= 3
	    fprintf(stderr_file,
		    "Received state %d from slave \"%s\" (expecting %d)\n",
		    msg_to_process->sequence,
		    source_peer->name,
		    sync_sequence);
#endif
	    if (msg_to_process->sequence == sync_sequence) {
		unsigned i;
		memcpy(_scratch_input_state,
		       _scratch_msg_info.data_start,
		       _scratch_msg_info.data_len);
		for (i = 0; i < MAX_INPUT_PORTS; i++) {
		    /* mask in the default deviations from each peer */
		    input_port_deviations[i] |= ntohs(_scratch_input_state[i]);
		}
		source_peer->state_expected = 0;
		if (! source_peer->ack_expected) {
		    result = 1;
		}
	    }
	    else if (msg_to_process->sequence == sync_sequence - 1) {
		_handle_duplicate_message(msg_to_process,
					  source_peer);
	    }
	    else if (msg_to_process->sequence == sync_sequence + 1) {
		_handle_early_message(msg_to_process, source_peer);
	    } else {
		/* This must be an old, late-delivered packet, or else
		   there's something wrong with the protocol
		   implementation.   TODO:  Add sanity check? */
	    }
	} /* end else if (msg_to_process->msg_type == LOCAL_STATE) */
    } /* end if (source_peer->state_expected) */
    else if (source_peer->ack_expected) {
	if (msg_to_process->msg_type == ACK &&
	    msg_to_process->sequence == sync_sequence)
	{
	    source_peer->ack_expected = 0;
	    if (_state_exchange_active > 0) {
		/* Now that the peer in question has supplied the requisite
		   ACK, it can be sent its copy of the local state. */
		_send_msg(_output_socket,
			  &(_current_outbound_state_msg_info),
			  &(source_peer->addr));
	    }
	    if (! source_peer->state_expected) {
		result = 1;
	    }
	}
    } else {
	/* Neither ACK nor STATE expected, therefore it is assumed
	       that this peer has sent whatever it needed to and only
	       other peers are holding up _inbound_sync(). */
	if (msg_to_process->sequence == _current_frame_sequence + 1 &&
	    (msg_to_process->msg_type == LOCAL_STATE ||
	     msg_to_process->msg_type == QUIT))
	{
	    _handle_early_message(msg_to_process, source_peer);
		
	}
	else if (msg_to_process->msg_type == LOCAL_STATE &&
		 msg_to_process->sequence == _current_frame_sequence)
	{
	    _handle_duplicate_message(msg_to_process, source_peer);
	} else {
	    /* Assume late or out-of-order message */
	}
    }
    return result;
}

/* This function listens to the network and waits for LOCAL_STATE and ACK
   messages.  It exits when all of the peers have satisfactorily reported,
   or a timeout elapses.
*/
static void
_inbound_sync(unsigned short input_port_deviations[MAX_INPUT_PORTS],
	      unsigned sync_sequence)
{
    unsigned i;
    unsigned peers_awaited = 0;
    /* This function has this amount of time to complete.  Any peers that
       haven't synced up when the time is up are dropped permanently. */
    unsigned rcv_result;
    unsigned time_used;
    unsigned time_before_retry = RETRY_TIMEOUT;
    unsigned time_before_giveup = GIVEUP_TIMEOUT;

    struct sockaddr_in source_addr;

    /* 1. Initialize time_since_resend for each peer, process early
       messages and count peers without early messages */
    for (i = 0; i < _original_player_count - 1; i++) {
	if (_peer_info[i].active &&
	    (_peer_info[i].ack_expected || _peer_info[i].state_expected))
	{
	    if (_peer_info[i].early_inbound_msg_info.sequence != sync_sequence ||
		! _process_inbound_message(
		      &(_peer_info[i].early_inbound_msg_info),
		      &(_peer_info[i]),
		      sync_sequence,
		      input_port_deviations))
	    {
		peers_awaited += 1;
		_peer_info[i].time_since_resend = 0;
	    }
	}
    }

    /* 2. For peers without early messages, process new messages from
       network */
    while (peers_awaited > 0 && time_before_giveup > 0) {
	time_used = time_before_retry;
	rcv_result = rcv_msg(_input_socket,
			     &_scratch_msg_info,
			     &source_addr,
			     &time_before_retry);
	time_used -= time_before_retry;
	
	/* I'd like to find an alternative to this loop, which seems
	   wasteful.  Previously I had a last_resend_time instead of
	   time_since_resend, which didn't need to be updated after each
	   call to rcv_msg, but after some (quite necessary) breaking up
	   of this function, the member (huh huh uh huh) needs to be
	   reset two frames down, in _handle_duplicate_message(), and I
	   don't want to add a "current time" paramter to both
	   functions so I can go back to that setup. */
	for (i = 0; i < _original_player_count - 1; i++) {
	    if (_peer_info[i].active) {
		_peer_info[i].time_since_resend += time_used;
	    }
	}

	if (rcv_result != OSD_OK) {
	    if (time_before_retry == 0) {
		/* Resend messages to whichever peers are dawdling */
		for (i = 0; i < _original_player_count - 1; i++) {
		    if (_peer_info[i].active && 
			(_peer_info[i].time_since_resend >= RETRY_TIMEOUT))
		    {
			if (_peer_info[i].ack_expected) {
#if PROTOCOL_DEBUG >= 2
			    fprintf(stderr_file,
				    "Peer \"%s\" timed out; "
				    "re-sending ACKable type %d, seq %d\n",
				    _peer_info[i].name,
				    _peer_info[i].last_outbound_ackable.msg_type,
				    _peer_info[i].last_outbound_ackable.sequence);
#endif
			    _send_msg(_output_socket,
				      &(_peer_info[i].last_outbound_ackable),
				      &(_peer_info[i].addr));
			    _peer_info[i].time_since_resend = 0;
			}
			else if (_peer_info[i].state_expected) {
#if PROTOCOL_DEBUG >= 2
			    fprintf(stderr_file,
				    "Peer \"%s\" timed out; "
				    "re-sending state sequence %d\n",
				    _peer_info[i].name,
				    _current_outbound_state_msg_info.sequence);
#endif
			    _send_msg(_output_socket,
				      &(_current_outbound_state_msg_info),
				      &(_peer_info[i].addr));
			    _peer_info[i].time_since_resend = 0;
#if PROTOCOL_DEBUG >= 1
			    _repeated_outbound_state_count += 1;
#endif
			}
		    }
		}
		time_before_retry = RETRY_TIMEOUT;
	    } /* end if (time_before_retry == 0) */
	} else /* Successfully received message before timeout; process it */ {
	    unsigned char peer_index =
		_peer_index_for_addr(&source_addr.sin_addr);

	    if (peer_index == (unsigned char)-1) {
		fprintf(stderr_file,
			"Error : Received packet from unknown peer\n");
	    }
	    else if (! _peer_info[peer_index].active &&
		     _scratch_msg_info.msg_type != QUIT)
	    {
		/* QUIT messages from inactive peers are ok,
		   but not other messages */
		fprintf(stderr_file,
			"Error: Received message (type %d)"
			" from inactive peer \"%s\\n",
			_scratch_msg_info.msg_type,
			_peer_info[peer_index].name);
	    } else {
		if (_process_inbound_message(&_scratch_msg_info,
					     &(_peer_info[peer_index]),
					     sync_sequence,
					     input_port_deviations))
		{
		    peers_awaited -= 1;
		}
	    }
	}
	if (time_before_giveup > time_used) {
	    time_before_giveup -= time_used;
	} else {
	    time_before_giveup = 0;
	}
    }

    /* 3. At this point all slaves have reported their states,
       or have timed out */
    /* TODO:  Dropping peers is susceptible to desync -- fix */
    if (peers_awaited > 0) {
	for (i = 0; i < _original_player_count - 1; i++)
	{
	    if (_peer_info[i].active &&
		(_peer_info[i].ack_expected || _peer_info[i].state_expected))
	    {
		fprintf(stderr_file,
			"No messages from peer \"%s\" in too long; disconnecting it\n",
			_peer_info[i].name);
		_peer_info[i].active = 0;
		_current_player_count -= 1;
		if (_current_player_count == 1) {
		    osd_net_close();
		}
	    }
	}
    }
}

#if PROTOCOL_DEBUG >= 3
static void
_log_port_array(unsigned short *port_values, unsigned port_count)
{
    unsigned i;
    for (i = 0; i < port_count; i++) {
	fprintf(stderr_file,
		"%x%s",
		port_values[i],
		(i < port_count - 1) ? " " : "\n");
    }
}
#endif

void osd_net_sync(unsigned short input_port_values[MAX_INPUT_PORTS],
                  unsigned short input_port_defaults[MAX_INPUT_PORTS])
{
    unsigned i;
    if (_current_net_role != NONE) {
	_current_frame_sequence += 1;
#if PROTOCOL_DEBUG >= 1
	record_sync_start();
#endif
	if (_input_remap) {
	    _remap_input_state(input_port_values);
	    _remap_input_state(input_port_defaults);
	}
	/* Change the values into deviations from default values */
	for (i = 0; i < MAX_INPUT_PORTS; i++) {
	    input_port_values[i] ^= input_port_defaults[i];
	}
	_outbound_sync(input_port_values, _current_frame_sequence);
#if PROTOCOL_DEBUG >= 3
	fprintf(stderr_file,
		"Sync %d outbound/saved deviations: ",
		_current_frame_sequence);
	_log_port_array(input_port_values, MAX_INPUT_PORTS);
#endif
	if (_parallel_sync) {
	    memcpy(_current_saved_input_port_values,
		   input_port_values,
		   MAX_INPUT_PORTS * sizeof(input_port_values[0]));
	    if (_current_frame_sequence > 1) {
		memcpy(input_port_values,
		       _previous_saved_input_port_values,
		       MAX_INPUT_PORTS * sizeof(input_port_values[0]));
#if PROTOCOL_DEBUG >= 3
	fprintf(stderr_file,
		"Sync %d retrieved deviations: ",
		_current_frame_sequence - 1);
	_log_port_array(input_port_values, MAX_INPUT_PORTS);
#endif
		_inbound_sync(input_port_values, _current_frame_sequence - 1);
	    } else {
		/* Just in case */
		memcpy(input_port_values,
		       input_port_defaults,
		       MAX_INPUT_PORTS * sizeof(input_port_values[0]));
	    }
	} else {
	    _inbound_sync(input_port_values, _current_frame_sequence);
	}
	/* Change deviations from default deviations back into actual values */
	for (i = 0; i < MAX_INPUT_PORTS; i++) {
	    input_port_values[i] ^= input_port_defaults[i];
	}
#if PROTOCOL_DEBUG >= 1
	record_sync_end();
#if PROTOCOL_DEBUG >= 3
	fprintf(stderr_file,
		"Frame %d final values: ",
		_current_frame_sequence);
	_log_port_array(input_port_values, MAX_INPUT_PORTS);
#endif
#endif
    } /* else this shouldn't even be called */
}
    
/*
 * Initialise network
 * - the master opens a socket and waits for slaves
 * - the slaves register to the master
 */
int osd_net_init(void)
{
    char *build_version_end;

    if (_original_player_count > 0) {
	_current_net_role = MASTER;
    }

    if (_master_hostname != NULL) {
	if (_current_net_role == MASTER) {
	    fprintf(stderr_file, "error: can't be Slave and Master\n");
	    return OSD_NOT_OK;
	}
	_current_net_role = SLAVE;
    }
	
    /* Warning:  Cutting out the date from the build_version string
       unfortunately relies on its format staying the same, which it
       most likely won't for any length of time.  This used in
       the handshake to match MAME versions -- is it worth it? */
    build_version_end = strchr(build_version, ' ');
    if (build_version_end == NULL) {
	strcpy(_truncated_build_version, build_version);
    } else {
	strncpy(_truncated_build_version,
		build_version,
		(build_version_end - build_version));
    }

    if (_current_net_role != NONE) {
	if (_init_sockets() != OSD_OK)
	    return OSD_NOT_OK;
	if (_current_net_role == MASTER) {
	    if (_await_slave_registrations() != OSD_OK)
		return OSD_NOT_OK;
	} else {
	    if (_register_to_master() != OSD_OK)
		return OSD_NOT_OK;
	    if (_input_remap)
		if (! _build_net_keymap()) {
		    _input_remap = 0;
		}
	}
    }
    return OSD_OK;
}

int osd_net_active(void)
{
    return (_current_net_role != NONE);
}

#define MILLISECONDS_PART(ms) (ms % 1000)
#define SECONDS_PART(ms) ((ms / 1000) - (ms / (60 * 1000) * 60))
#define MINUTES_PART(ms) (ms / (60 * 1000))

/*
 * Close all opened sockets
 */
void osd_net_close(void)
{
    unsigned peer_index = 0;
    if (_current_net_role != NONE) {
	if (_current_player_count > 1) {
	    _current_frame_sequence += 1;
	    _state_exchange_active = 0;
	    for (peer_index = 0;
		 peer_index < _original_player_count - 1;
		 peer_index++)
	    {
		if (_peer_info[peer_index].active) {
		    _peer_info[peer_index].last_outbound_ackable.msg_type =
			QUIT;
		    _peer_info[peer_index].last_outbound_ackable.sequence =
			_current_frame_sequence;
		    _prime_msg(&(_peer_info[peer_index].last_outbound_ackable));
		    _send_msg(_output_socket,
			      &(_peer_info[peer_index].last_outbound_ackable),
			      &(_peer_info[peer_index].addr));
		    _peer_info[peer_index].ack_expected = 1;
		}
	    }
	    _inbound_sync(NULL, _current_frame_sequence);
	}
	close(_input_socket);
	close(_output_socket);
    }
#if PROTOCOL_DEBUG >= 1
    if (_current_net_role != NONE) {
	struct timeval end_time;
	unsigned total_time;
	gettimeofday(&end_time, NULL);

	total_time = tv_subtract(&end_time, &_start_time) / 1000;
	fprintf(stderr_file, "Protocol stats:\n");
	fprintf(stderr_file, "Current frame sequence:  %d\n",
		_current_frame_sequence);
	fprintf(stderr_file,
		"Outbound state repeats:  %d\n",
		_repeated_outbound_state_count);

	fprintf(stderr_file,
		"Total sync time:  %02dm%02d.%ds (%02dm%02d.%ds real time elapsed)\n",
		MINUTES_PART(_total_sync_time),
		SECONDS_PART(_total_sync_time),
		MILLISECONDS_PART(_total_sync_time),
		MINUTES_PART(total_time),
		SECONDS_PART(total_time),
		MILLISECONDS_PART(total_time));

	fprintf(stderr_file,
		"Average sync time:  %d ms\n",
		_total_sync_time / _current_frame_sequence);
	fprintf(stderr_file, "Min sync time:  %d ms\n", _min_sync_time);
	fprintf(stderr_file, "Max sync time:  %d ms\n", _max_sync_time);
    }
#endif
    _current_net_role = NONE;
}

#endif /* MAME_NET */
