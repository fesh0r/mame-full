    __glXCreateGLXPixmap = (GLXPixmap (CALLBACK *)( Display *, XVisualInfo *, Pixmap))
        GET_GL_PROCADDRESS("glXCreateGLXPixmap");

    __glXChooseVisual = (XVisualInfo* (CALLBACK *) (Display *, int , int *)) 
    	GET_GL_PROCADDRESS("glXChooseVisual");

    __glXCreateContext = (GLXContext (CALLBACK *) (Display *, XVisualInfo *, GLXContext, Bool))
    	GET_GL_PROCADDRESS("glXCreateContext");

    __glXDestroyContext = (void (CALLBACK *) (Display *, GLXContext))
    	GET_GL_PROCADDRESS("glXDestroyContext");

    __glXGetConfig = (int (CALLBACK *) (Display *, XVisualInfo *, int, int *)) 
    	GET_GL_PROCADDRESS("glXGetConfig");

    __glXMakeCurrent = (Bool (CALLBACK *) (Display *, GLXDrawable, GLXContext)) 
    	GET_GL_PROCADDRESS("glXMakeCurrent");

    __glXSwapBuffers = (void (CALLBACK *) (Display *, GLXDrawable)) 
    	GET_GL_PROCADDRESS("glXSwapBuffers");

