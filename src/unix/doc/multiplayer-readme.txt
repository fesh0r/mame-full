	Multiplayer Network XMame N0.4 (Rewrite by Steve Freeland, caucasatron@yahoo.ca)
	------------------------------
	mame version 0.58.1

Usage (for an n-player game):
-----
Start a master:
    xmame.<display method> -master <n> <other options> <game name>
Start n - 1 slaves:
    xmame.<display method> -slave <master hostname> <other options> <game name>
Currently there can only be one slave per machine, although a slave
can share a machine with the master.
The slave can also use the -netmapkey option, which for true
multiplayer games (eg Gauntlet) should let you use player 1
controls (which you're likely to have configured to something convenient)
to control player <i> (whichever player number the slave is assigned
by the master).

Message format
--------------
Each UDP packet has a 4-byte header followed by a body.
The header has a 3-byte magic string "XNM" followed by a 1-byte message type.
The contents of the message body depends on the message type, and
there may in fact not be a body.

Message types:  See enum msg_type_t in src/unix/network.c in case
-------------   this doc is out of date
JOIN:  This is the first message the slaves send to the master for
handshaking.  The body contains a string identifying the slave
(<hostname>/<pid>), the slave's MAME build version and net protocol
version, and the name of the game being played.  If any of the latter
3 do not match the master's information, the master will refuse the
join.  The slave id string is currently only used for logging.

ACCEPT:  This is the master's reply to the slave if the protocol
versions and game name match.  The body contains is the player number
assigned to the slave (usually 2 to 4; the master is always player 1)
and the id strings and IP addresses for all the other slaves.

REFUSE:  This is the master's reply to the slave if the protocol
versions and/or game name do not match.  There is no body.

LOCAL_STATE:  This message is sent out each frame (when osd_net_sync() is
invoked) by each peer to all the other peers.  It contains a
sequence number (see Protocol below) and a byte order-normalized copy
of the slave's current input_port_values[] array.  Each peer uses
incoming messages of this type to compute the input global state.

QUIT:  Sent when a client or the master exits and allows the remaining
participants to handle the exit gracefully without having wait for a timeout.

ACKs and ACKable messages:  Certain message types are designated
"ACKable":  these are intermixed with the regular LOCAL_STATE messages
which make up the bulk of the traffic.  An ACKable is sent out by a
peer at the beginning of a frame *instead* of a LOCAL_STATE message.
The peer then waits for the recipient to send an ACK (acknowledgement)
message in response, at which point the LOCAL_STATE can be sent out.
Currently the following message types are "ACKable":  ACCEPT and QUIT.

Protocol
--------
Each frame, upon invocation of osd_net_sync(), each peer sends its
local input state to each of the other peers, then waits for the
incoming messages containing the local input state for each of the other
peers to arrive.  When all have been received the global input state
is computed, osd_net_sync() exits and emulation proceeds exactly as if
the input had been received through a local input device rather than
the network.
Since the protocol uses UDP messages rather than TCP (in order to
minimize the impact of latency) there is no guarantee that a message
sent will arrive at its destination, or how long it will take even if
it does arrive.  Therefore a peer may need to re-send a local input
state if conditions arise which indicate the previous message did not
reach its destination in time.

                               +--------+
			       | SYNCED |
			       +--------+
                                    |
== Frame n begins ==================+======================================
                                    |
                 +------------------+-----------------+
  Send ACKable n |		                      | Send state n
                 v                                    v
     +----------------------+              +----------------------+
     | ack_expected   = yes | rcv ACK n    | ack_expected   = no  |
     | state_expected = yes |------------->| state_expected = yes |
     +----------------------+ send STATE n +----------------------+
          |  ^   |   ^  |                      |  ^   |   ^  |
      rcv |  |   |   |  | rcv STATE        rcv |  |   |   |  | rcv STATE
  ACKable |  |   |   |  | n - 1        ACKable |  |   |   |  | n - 1
        n |  |   |   |  |                    n |  |   |   |  |
          v  |   |   |  v                      v  |   |   |  v 
 send ACK |  |   |   |  | send STATE  send ACK |  |   |   |  | send STATE
        n |  |   |   |  | n - 1              n |  |   |   |  | n - 1
          +--+   |   +--+                      +--+   |   +--+
                 |                                    |
     rcv STATE n |                                    |
                 v                                    |
     +----------------------+                         |
     | ack_expected   = yes |                         | rcv
     | state_expected = no  |                         | STATE
     +----------------------+           +--+          | n
                 |                      |  |          |
             rcv |         send STATE n |  |          |
             ACK |                      |  |          |
               n |                      ^  |          |
                 v          rcv STATE n |  |          |
            send |                      |  v          |
           STATE |       +---------------------+      |
               n |       | ack_expected = no   |      |
		 +------>| state_expected = no |<-----+
                         +---------------------+
                                    |
                                    | Other peers sync
                                    |
== Frame n ends ====================+======================================
                                    |                  
                                    v
                               +--------+              
	                       | SYNCED |              
			       +--------+

TODO
----
. Adjust for high latency -- coarser sync frequency?	
. Handle factors which may cause a variance in the initial state in the
  different emulated machines -- NVRAM and high scores.  Anything else?
. Sync analog controls
. Handle UI-originated events, eg pause, reset
. More flexible network port config?
. Talk to the core team about integrating the protocol-level stuff
  into the platform-independant network.c
. _inbound_sync() is getting a bit unmaintainable -- try to break it
  up as much as possible

Old readme:
-----------
	Multiplayer Network XMame 0.02
	------------------------------
	mame version 0.34b5

Contact: Eric Totel (totel@laas.fr)

Description
-----------
This adds/modifies some files to support network mame games.
!!!! Only X11 related files have been patched !!!!!
If someone wants to make the keys communication for svgalib, ggi ... he
is welcome ;-)

Disclaimer
----------
I have no special knowledge about xmame sources. This version was builded
quickly (one day), and just for fun. Make what you want with it !!!
This is probably the last update of this particular patch. This work will
probably merge with the netmame project.

New to version 0.02
-------------------
KEY MAPPING:
- key mapping from player 1 keys to player n keys (each time player n uses
  a player 1 key, this key is mapped to correct key for player n).
  Consequently, all players can use the player 1 keys to act upon the game.
  BEWARE this is not always a good thing. That is why it is only an option.
  Comments:
  - key mapping activation is of interest for *real* multiplayer games (such
    as gauntlet for example)
  - key mapping won't work with games where each player plays at his turn and
    where the same keys are used by all players.

Installation
------------
In this archive, you will find the patch for the xmame 0.33rc1.1 source tree.
Thus, you need the xmame archive, and must compile the whole thing.

1. Extract the xmame archive
2. Extract the xnetmame in the xmame directory
3. Compile as usual.

How to use it ?
---------------
1. launch 'xmame -master <number of players> ...'
2. launch 'xmame -slave <master host name> ...' on the machines needed
   to play the game.
3. Enjoy !

Examples:
1. on machine gandalf: xmame -master 2 gaunt2p
2. on machine bilbo:   xmame -netmapkey -slave gandalf gaunt2p

Was tested on
-------------
- Linux on a stand-alone machine
- Solaris (Sparstations 4/5/20, Ultra) on an intranet

Test results
------------
The time overheads induced by the communications are acceptable on an ethernet
network. It proved to worked on ISDN lines too.
I've not tried any modem game, and I probably won't ;-) But if someone obtains
results in this particular case, i'd like to have some infos.

To add
------
- I cannot test joystick support, so it probably does not work currently
  (only keycodes are sent via the network)
- mouse network support
- this little thing will probably merge with the netmame project

Technical comments
------------------
The goal is to synchronise many xmames running independantly on a
network. This is performed by message exchange between the programs.
The communications are based on udp communications. It will
probably move to tcp to enhance the reliability of communications.
It is necessary to ensure that no message is lost, so that the replica
can be considered to be consistant.

The synchronisation occurs at display update:
- all slaves send their current key table to master
- the master builds a common key table and builds the input ports values
- the master sends the input ports values to all slaves, so that all
  emulated games use the same entries.
  (same input => same output) <=> same behaviour

No deviation of behaviour was stated during the few tests. All replicas
seem to behave exactly the same.

Source comments
---------------
All modifications are inserted in #ifdef MAME_NET ... #endif /* MAME_NET */
sections.

The following OS-dependant functions were defined:

--- src/unix/keyboard.c
void osd_update_keys(char keys[OSD_MAX_KEY+1]);
  update key table using another process key table

void osd_build_global_keys(void);
  key tables exchange between replica

--- src/unix/network.c
int osd_send_msg(void *msg, int size);
  master : sends message to all slaves
  slave  : sends message to master

int osd_receive_msg(void *msg, int size);
  read a message received on reception socket

void osd_network_synchronise(void);
  synchronise the replicas by message exchange

void osd_cleanup_network(void);
  clean up network structures

void osd_net_init(void);      
  init network game
