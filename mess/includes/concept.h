/*
	Corvus Concept driver

	Raphael Nabet, 2003
*/

enum
{
	rom0_base = 0x10000
};


READ16_HANDLER(concept_io_r);
WRITE16_HANDLER(concept_io_w);
MACHINE_INIT(concept);
VIDEO_START(concept);
VIDEO_UPDATE(concept);
