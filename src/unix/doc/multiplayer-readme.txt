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
