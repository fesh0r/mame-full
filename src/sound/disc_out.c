/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@esplexo.co.uk)                       */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DSO_OUTPUT - Sinewave generator source code                          */
/*                                                                      */
/************************************************************************/

struct dso_output_context
{
	INT16 output;
};

/************************************************************************/
/*                                                                      */
/* Usage of node_description values for step function                   */
/*                                                                      */
/* input[0]    - Output value                                           */
/* input[1]    - Volume setting (static)                                */
/* input[2]    - NOT USED                                               */
/* input[3]    - NOT USED                                               */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dso_output_step(struct node_description *node)
{
	/* We ALWAYS work in stereo here, let the stream update decide if its mono/stereo output */
	struct dso_output_context *context;
	context=(struct dso_output_context*)node->context;
	/* Clamp outputs */
	if(node->input[0]<-32768) context->output=-32768;  else if(node->input[0]>32767) context->output=32767;  else context->output=(INT16)node->input[0];
	return 0;
}

int dso_output_init(struct node_description *node)
{
	discrete_log("dso_output_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dso_output_context)))==NULL)
	{
		discrete_log("dso_output_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dso_output_context));
	}

	return 0;
}



