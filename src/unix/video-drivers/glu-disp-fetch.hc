	__gluGetString = (const GLubyte *(CALLBACK *)( GLenum name ))
		GET_GL_PROCADDRESS ("gluGetString");


    __gluErrorString = ( const GLubyte * (CALLBACK *) ( GLenum) )
    	GET_GL_PROCADDRESS("gluErrorString");

    __gluLookAt = ( void (CALLBACK *) ( GLdouble, GLdouble, GLdouble,
                                	GLdouble, GLdouble, GLdouble,
                                	GLdouble, GLdouble, GLdouble) )
    	GET_GL_PROCADDRESS("gluLookAt");

