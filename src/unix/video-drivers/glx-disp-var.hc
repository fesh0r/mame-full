
GLXPixmap (CALLBACK * __glXCreateGLXPixmap)( Display *, XVisualInfo *, Pixmap) = NULL;
XVisualInfo* (CALLBACK * __glXChooseVisual) (Display *, int , int *) = NULL;
GLXContext (CALLBACK * __glXCreateContext) (Display *, XVisualInfo *, GLXContext, Bool) = NULL;
void (CALLBACK * __glXDestroyContext) (Display *, GLXContext) = NULL;
int (CALLBACK * __glXGetConfig) (Display *, XVisualInfo *, int, int *) = NULL;
Bool (CALLBACK * __glXMakeCurrent) (Display *, GLXDrawable, GLXContext) = NULL;
void (CALLBACK * __glXSwapBuffers) (Display *, GLXDrawable) = NULL;

