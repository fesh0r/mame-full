
extern GLXPixmap (CALLBACK * __glXCreateGLXPixmap)( Display *, XVisualInfo *, Pixmap);
extern XVisualInfo* (CALLBACK * __glXChooseVisual) (Display *, int , int *);
extern GLXContext (CALLBACK * __glXCreateContext) (Display *, XVisualInfo *, GLXContext, Bool);
extern void (CALLBACK * __glXDestroyContext) (Display *, GLXContext);
extern int (CALLBACK * __glXGetConfig) (Display *, XVisualInfo *, int, int *);
extern Bool (CALLBACK * __glXMakeCurrent) (Display *, GLXDrawable, GLXContext);
extern void (CALLBACK * __glXSwapBuffers) (Display *, GLXDrawable);

