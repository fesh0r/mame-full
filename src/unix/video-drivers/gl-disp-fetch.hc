	__glGetString = (const GLubyte *(CALLBACK *)( GLenum name ))
		GET_GL_PROCADDRESS ("glGetString");

/**
 * C2J Parser Version 1.6 Beta
 * Jausoft - Sven Goethel Software Development
 * Reading from file: gl-proto-auto.orig.h . . .
 * Destination-Class: aa ! 
 */

	__glClearIndex = (void (CALLBACK *)(GLfloat))
	  GET_GL_PROCADDRESS ("glClearIndex");

	__glClearColor = (void (CALLBACK *)(GLclampf, GLclampf, GLclampf, GLclampf))
	  GET_GL_PROCADDRESS ("glClearColor");

	__glClear = (void (CALLBACK *)(GLbitfield))
	  GET_GL_PROCADDRESS ("glClear");

	__glIndexMask = (void (CALLBACK *)(GLuint))
	  GET_GL_PROCADDRESS ("glIndexMask");

	__glColorMask = (void (CALLBACK *)(GLboolean, GLboolean, GLboolean, GLboolean))
	  GET_GL_PROCADDRESS ("glColorMask");

	__glAlphaFunc = (void (CALLBACK *)(GLenum, GLclampf))
	  GET_GL_PROCADDRESS ("glAlphaFunc");

	__glBlendFunc = (void (CALLBACK *)(GLenum, GLenum))
	  GET_GL_PROCADDRESS ("glBlendFunc");

	__glLogicOp = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glLogicOp");

	__glCullFace = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glCullFace");

	__glFrontFace = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glFrontFace");

	__glPointSize = (void (CALLBACK *)(GLfloat))
	  GET_GL_PROCADDRESS ("glPointSize");

	__glLineWidth = (void (CALLBACK *)(GLfloat))
	  GET_GL_PROCADDRESS ("glLineWidth");

	__glLineStipple = (void (CALLBACK *)(GLint, GLushort))
	  GET_GL_PROCADDRESS ("glLineStipple");

	__glPolygonMode = (void (CALLBACK *)(GLenum, GLenum))
	  GET_GL_PROCADDRESS ("glPolygonMode");

	__glPolygonOffset = (void (CALLBACK *)(GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glPolygonOffset");

	__glPolygonStipple = (void (CALLBACK *)(const GLubyte *))
	  GET_GL_PROCADDRESS ("glPolygonStipple");

	__glGetPolygonStipple = (void (CALLBACK *)(GLubyte *))
	  GET_GL_PROCADDRESS ("glGetPolygonStipple");

	__glEdgeFlag = (void (CALLBACK *)(GLboolean))
	  GET_GL_PROCADDRESS ("glEdgeFlag");

	__glEdgeFlagv = (void (CALLBACK *)(const GLboolean *))
	  GET_GL_PROCADDRESS ("glEdgeFlagv");

	__glScissor = (void (CALLBACK *)(GLint, GLint, GLsizei, GLsizei))
	  GET_GL_PROCADDRESS ("glScissor");

	__glClipPlane = (void (CALLBACK *)(GLenum, const GLdouble *))
	  GET_GL_PROCADDRESS ("glClipPlane");

	__glGetClipPlane = (void (CALLBACK *)(GLenum, GLdouble *))
	  GET_GL_PROCADDRESS ("glGetClipPlane");

	__glDrawBuffer = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glDrawBuffer");

	__glReadBuffer = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glReadBuffer");

	__glEnable = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glEnable");

	__glDisable = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glDisable");

	__glIsEnabled = (GLboolean (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glIsEnabled");

	__glEnableClientState = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glEnableClientState");

	__glDisableClientState = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glDisableClientState");

	__glGetBooleanv = (void (CALLBACK *)(GLenum, GLboolean *))
	  GET_GL_PROCADDRESS ("glGetBooleanv");

	__glGetDoublev = (void (CALLBACK *)(GLenum, GLdouble *))
	  GET_GL_PROCADDRESS ("glGetDoublev");

	__glGetFloatv = (void (CALLBACK *)(GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetFloatv");

	__glGetIntegerv = (void (CALLBACK *)(GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetIntegerv");

	__glPushAttrib = (void (CALLBACK *)(GLbitfield))
	  GET_GL_PROCADDRESS ("glPushAttrib");

	__glPopAttrib = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glPopAttrib");

	__glPushClientAttrib = (void (CALLBACK *)(GLbitfield))
	  GET_GL_PROCADDRESS ("glPushClientAttrib");

	__glPopClientAttrib = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glPopClientAttrib");

	__glRenderMode = (GLint (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glRenderMode");

	__glGetError = (GLenum (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glGetError");

	__glFinish = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glFinish");

	__glFlush = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glFlush");

	__glHint = (void (CALLBACK *)(GLenum, GLenum))
	  GET_GL_PROCADDRESS ("glHint");

	__glClearDepth = (void (CALLBACK *)(GLclampd))
	  GET_GL_PROCADDRESS ("glClearDepth");

	__glDepthFunc = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glDepthFunc");

	__glDepthMask = (void (CALLBACK *)(GLboolean))
	  GET_GL_PROCADDRESS ("glDepthMask");

	__glDepthRange = (void (CALLBACK *)(GLclampd, GLclampd))
	  GET_GL_PROCADDRESS ("glDepthRange");

	__glClearAccum = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glClearAccum");

	__glAccum = (void (CALLBACK *)(GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glAccum");

	__glMatrixMode = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glMatrixMode");

	__glOrtho = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glOrtho");

	__glFrustum = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glFrustum");

	__glViewport = (void (CALLBACK *)(GLint, GLint, GLsizei, GLsizei))
	  GET_GL_PROCADDRESS ("glViewport");

	__glPushMatrix = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glPushMatrix");

	__glPopMatrix = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glPopMatrix");

	__glLoadIdentity = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glLoadIdentity");

	__glLoadMatrixd = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glLoadMatrixd");

	__glLoadMatrixf = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glLoadMatrixf");

	__glMultMatrixd = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glMultMatrixd");

	__glMultMatrixf = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glMultMatrixf");

	__glRotated = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glRotated");

	__glRotatef = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glRotatef");

	__glScaled = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glScaled");

	__glScalef = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glScalef");

	__glTranslated = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glTranslated");

	__glTranslatef = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glTranslatef");

	__glIsList = (GLboolean (CALLBACK *)(GLuint))
	  GET_GL_PROCADDRESS ("glIsList");

	__glDeleteLists = (void (CALLBACK *)(GLuint, GLsizei))
	  GET_GL_PROCADDRESS ("glDeleteLists");

	__glGenLists = (GLuint (CALLBACK *)(GLsizei))
	  GET_GL_PROCADDRESS ("glGenLists");

	__glNewList = (void (CALLBACK *)(GLuint, GLenum))
	  GET_GL_PROCADDRESS ("glNewList");

	__glEndList = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glEndList");

	__glCallList = (void (CALLBACK *)(GLuint))
	  GET_GL_PROCADDRESS ("glCallList");

	__glCallLists = (void (CALLBACK *)(GLsizei, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glCallLists");

	__glListBase = (void (CALLBACK *)(GLuint))
	  GET_GL_PROCADDRESS ("glListBase");

	__glBegin = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glBegin");

	__glEnd = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glEnd");

	__glVertex2d = (void (CALLBACK *)(GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glVertex2d");

	__glVertex2f = (void (CALLBACK *)(GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glVertex2f");

	__glVertex2i = (void (CALLBACK *)(GLint, GLint))
	  GET_GL_PROCADDRESS ("glVertex2i");

	__glVertex2s = (void (CALLBACK *)(GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glVertex2s");

	__glVertex3d = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glVertex3d");

	__glVertex3f = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glVertex3f");

	__glVertex3i = (void (CALLBACK *)(GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glVertex3i");

	__glVertex3s = (void (CALLBACK *)(GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glVertex3s");

	__glVertex4d = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glVertex4d");

	__glVertex4f = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glVertex4f");

	__glVertex4i = (void (CALLBACK *)(GLint, GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glVertex4i");

	__glVertex4s = (void (CALLBACK *)(GLshort, GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glVertex4s");

	__glVertex2dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glVertex2dv");

	__glVertex2fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glVertex2fv");

	__glVertex2iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glVertex2iv");

	__glVertex2sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glVertex2sv");

	__glVertex3dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glVertex3dv");

	__glVertex3fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glVertex3fv");

	__glVertex3iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glVertex3iv");

	__glVertex3sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glVertex3sv");

	__glVertex4dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glVertex4dv");

	__glVertex4fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glVertex4fv");

	__glVertex4iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glVertex4iv");

	__glVertex4sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glVertex4sv");

	__glNormal3b = (void (CALLBACK *)(GLbyte, GLbyte, GLbyte))
	  GET_GL_PROCADDRESS ("glNormal3b");

	__glNormal3d = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glNormal3d");

	__glNormal3f = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glNormal3f");

	__glNormal3i = (void (CALLBACK *)(GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glNormal3i");

	__glNormal3s = (void (CALLBACK *)(GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glNormal3s");

	__glNormal3bv = (void (CALLBACK *)(const GLbyte *))
	  GET_GL_PROCADDRESS ("glNormal3bv");

	__glNormal3dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glNormal3dv");

	__glNormal3fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glNormal3fv");

	__glNormal3iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glNormal3iv");

	__glNormal3sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glNormal3sv");

	__glIndexd = (void (CALLBACK *)(GLdouble))
	  GET_GL_PROCADDRESS ("glIndexd");

	__glIndexf = (void (CALLBACK *)(GLfloat))
	  GET_GL_PROCADDRESS ("glIndexf");

	__glIndexi = (void (CALLBACK *)(GLint))
	  GET_GL_PROCADDRESS ("glIndexi");

	__glIndexs = (void (CALLBACK *)(GLshort))
	  GET_GL_PROCADDRESS ("glIndexs");

	__glIndexub = (void (CALLBACK *)(GLubyte))
	  GET_GL_PROCADDRESS ("glIndexub");

	__glIndexdv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glIndexdv");

	__glIndexfv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glIndexfv");

	__glIndexiv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glIndexiv");

	__glIndexsv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glIndexsv");

	__glIndexubv = (void (CALLBACK *)(const GLubyte *))
	  GET_GL_PROCADDRESS ("glIndexubv");

	__glColor3b = (void (CALLBACK *)(GLbyte, GLbyte, GLbyte))
	  GET_GL_PROCADDRESS ("glColor3b");

	__glColor3d = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glColor3d");

	__glColor3f = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glColor3f");

	__glColor3i = (void (CALLBACK *)(GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glColor3i");

	__glColor3s = (void (CALLBACK *)(GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glColor3s");

	__glColor3ub = (void (CALLBACK *)(GLubyte, GLubyte, GLubyte))
	  GET_GL_PROCADDRESS ("glColor3ub");

	__glColor3ui = (void (CALLBACK *)(GLuint, GLuint, GLuint))
	  GET_GL_PROCADDRESS ("glColor3ui");

	__glColor3us = (void (CALLBACK *)(GLushort, GLushort, GLushort))
	  GET_GL_PROCADDRESS ("glColor3us");

	__glColor4b = (void (CALLBACK *)(GLbyte, GLbyte, GLbyte, GLbyte))
	  GET_GL_PROCADDRESS ("glColor4b");

	__glColor4d = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glColor4d");

	__glColor4f = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glColor4f");

	__glColor4i = (void (CALLBACK *)(GLint, GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glColor4i");

	__glColor4s = (void (CALLBACK *)(GLshort, GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glColor4s");

	__glColor4ub = (void (CALLBACK *)(GLubyte, GLubyte, GLubyte, GLubyte))
	  GET_GL_PROCADDRESS ("glColor4ub");

	__glColor4ui = (void (CALLBACK *)(GLuint, GLuint, GLuint, GLuint))
	  GET_GL_PROCADDRESS ("glColor4ui");

	__glColor4us = (void (CALLBACK *)(GLushort, GLushort, GLushort, GLushort))
	  GET_GL_PROCADDRESS ("glColor4us");

	__glColor3bv = (void (CALLBACK *)(const GLbyte *))
	  GET_GL_PROCADDRESS ("glColor3bv");

	__glColor3dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glColor3dv");

	__glColor3fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glColor3fv");

	__glColor3iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glColor3iv");

	__glColor3sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glColor3sv");

	__glColor3ubv = (void (CALLBACK *)(const GLubyte *))
	  GET_GL_PROCADDRESS ("glColor3ubv");

	__glColor3uiv = (void (CALLBACK *)(const GLuint *))
	  GET_GL_PROCADDRESS ("glColor3uiv");

	__glColor3usv = (void (CALLBACK *)(const GLushort *))
	  GET_GL_PROCADDRESS ("glColor3usv");

	__glColor4bv = (void (CALLBACK *)(const GLbyte *))
	  GET_GL_PROCADDRESS ("glColor4bv");

	__glColor4dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glColor4dv");

	__glColor4fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glColor4fv");

	__glColor4iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glColor4iv");

	__glColor4sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glColor4sv");

	__glColor4ubv = (void (CALLBACK *)(const GLubyte *))
	  GET_GL_PROCADDRESS ("glColor4ubv");

	__glColor4uiv = (void (CALLBACK *)(const GLuint *))
	  GET_GL_PROCADDRESS ("glColor4uiv");

	__glColor4usv = (void (CALLBACK *)(const GLushort *))
	  GET_GL_PROCADDRESS ("glColor4usv");

	__glTexCoord1d = (void (CALLBACK *)(GLdouble))
	  GET_GL_PROCADDRESS ("glTexCoord1d");

	__glTexCoord1f = (void (CALLBACK *)(GLfloat))
	  GET_GL_PROCADDRESS ("glTexCoord1f");

	__glTexCoord1i = (void (CALLBACK *)(GLint))
	  GET_GL_PROCADDRESS ("glTexCoord1i");

	__glTexCoord1s = (void (CALLBACK *)(GLshort))
	  GET_GL_PROCADDRESS ("glTexCoord1s");

	__glTexCoord2d = (void (CALLBACK *)(GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glTexCoord2d");

	__glTexCoord2f = (void (CALLBACK *)(GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glTexCoord2f");

	__glTexCoord2i = (void (CALLBACK *)(GLint, GLint))
	  GET_GL_PROCADDRESS ("glTexCoord2i");

	__glTexCoord2s = (void (CALLBACK *)(GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glTexCoord2s");

	__glTexCoord3d = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glTexCoord3d");

	__glTexCoord3f = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glTexCoord3f");

	__glTexCoord3i = (void (CALLBACK *)(GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glTexCoord3i");

	__glTexCoord3s = (void (CALLBACK *)(GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glTexCoord3s");

	__glTexCoord4d = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glTexCoord4d");

	__glTexCoord4f = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glTexCoord4f");

	__glTexCoord4i = (void (CALLBACK *)(GLint, GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glTexCoord4i");

	__glTexCoord4s = (void (CALLBACK *)(GLshort, GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glTexCoord4s");

	__glTexCoord1dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glTexCoord1dv");

	__glTexCoord1fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glTexCoord1fv");

	__glTexCoord1iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glTexCoord1iv");

	__glTexCoord1sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glTexCoord1sv");

	__glTexCoord2dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glTexCoord2dv");

	__glTexCoord2fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glTexCoord2fv");

	__glTexCoord2iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glTexCoord2iv");

	__glTexCoord2sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glTexCoord2sv");

	__glTexCoord3dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glTexCoord3dv");

	__glTexCoord3fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glTexCoord3fv");

	__glTexCoord3iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glTexCoord3iv");

	__glTexCoord3sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glTexCoord3sv");

	__glTexCoord4dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glTexCoord4dv");

	__glTexCoord4fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glTexCoord4fv");

	__glTexCoord4iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glTexCoord4iv");

	__glTexCoord4sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glTexCoord4sv");

	__glRasterPos2d = (void (CALLBACK *)(GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glRasterPos2d");

	__glRasterPos2f = (void (CALLBACK *)(GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glRasterPos2f");

	__glRasterPos2i = (void (CALLBACK *)(GLint, GLint))
	  GET_GL_PROCADDRESS ("glRasterPos2i");

	__glRasterPos2s = (void (CALLBACK *)(GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glRasterPos2s");

	__glRasterPos3d = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glRasterPos3d");

	__glRasterPos3f = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glRasterPos3f");

	__glRasterPos3i = (void (CALLBACK *)(GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glRasterPos3i");

	__glRasterPos3s = (void (CALLBACK *)(GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glRasterPos3s");

	__glRasterPos4d = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glRasterPos4d");

	__glRasterPos4f = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glRasterPos4f");

	__glRasterPos4i = (void (CALLBACK *)(GLint, GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glRasterPos4i");

	__glRasterPos4s = (void (CALLBACK *)(GLshort, GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glRasterPos4s");

	__glRasterPos2dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glRasterPos2dv");

	__glRasterPos2fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glRasterPos2fv");

	__glRasterPos2iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glRasterPos2iv");

	__glRasterPos2sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glRasterPos2sv");

	__glRasterPos3dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glRasterPos3dv");

	__glRasterPos3fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glRasterPos3fv");

	__glRasterPos3iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glRasterPos3iv");

	__glRasterPos3sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glRasterPos3sv");

	__glRasterPos4dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glRasterPos4dv");

	__glRasterPos4fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glRasterPos4fv");

	__glRasterPos4iv = (void (CALLBACK *)(const GLint *))
	  GET_GL_PROCADDRESS ("glRasterPos4iv");

	__glRasterPos4sv = (void (CALLBACK *)(const GLshort *))
	  GET_GL_PROCADDRESS ("glRasterPos4sv");

	__glRectd = (void (CALLBACK *)(GLdouble, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glRectd");

	__glRectf = (void (CALLBACK *)(GLfloat, GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glRectf");

	__glRecti = (void (CALLBACK *)(GLint, GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glRecti");

	__glRects = (void (CALLBACK *)(GLshort, GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glRects");

	__glRectdv = (void (CALLBACK *)(const GLdouble *, const GLdouble *))
	  GET_GL_PROCADDRESS ("glRectdv");

	__glRectfv = (void (CALLBACK *)(const GLfloat *, const GLfloat *))
	  GET_GL_PROCADDRESS ("glRectfv");

	__glRectiv = (void (CALLBACK *)(const GLint *, const GLint *))
	  GET_GL_PROCADDRESS ("glRectiv");

	__glRectsv = (void (CALLBACK *)(const GLshort *, const GLshort *))
	  GET_GL_PROCADDRESS ("glRectsv");

	__glVertexPointer = (void (CALLBACK *)(GLint, GLenum, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glVertexPointer");

	__glNormalPointer = (void (CALLBACK *)(GLenum, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glNormalPointer");

	__glColorPointer = (void (CALLBACK *)(GLint, GLenum, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glColorPointer");

	__glIndexPointer = (void (CALLBACK *)(GLenum, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glIndexPointer");

	__glTexCoordPointer = (void (CALLBACK *)(GLint, GLenum, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexCoordPointer");

	__glEdgeFlagPointer = (void (CALLBACK *)(GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glEdgeFlagPointer");

	__glGetPointerv = (void (CALLBACK *)(GLenum, void **))
	  GET_GL_PROCADDRESS ("glGetPointerv");

	__glArrayElement = (void (CALLBACK *)(GLint))
	  GET_GL_PROCADDRESS ("glArrayElement");

	__glDrawArrays = (void (CALLBACK *)(GLenum, GLint, GLsizei))
	  GET_GL_PROCADDRESS ("glDrawArrays");

	__glDrawElements = (void (CALLBACK *)(GLenum, GLsizei, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glDrawElements");

	__glInterleavedArrays = (void (CALLBACK *)(GLenum, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glInterleavedArrays");

	__glShadeModel = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glShadeModel");

	__glLightf = (void (CALLBACK *)(GLenum, GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glLightf");

	__glLighti = (void (CALLBACK *)(GLenum, GLenum, GLint))
	  GET_GL_PROCADDRESS ("glLighti");

	__glLightfv = (void (CALLBACK *)(GLenum, GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glLightfv");

	__glLightiv = (void (CALLBACK *)(GLenum, GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glLightiv");

	__glGetLightfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetLightfv");

	__glGetLightiv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetLightiv");

	__glLightModelf = (void (CALLBACK *)(GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glLightModelf");

	__glLightModeli = (void (CALLBACK *)(GLenum, GLint))
	  GET_GL_PROCADDRESS ("glLightModeli");

	__glLightModelfv = (void (CALLBACK *)(GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glLightModelfv");

	__glLightModeliv = (void (CALLBACK *)(GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glLightModeliv");

	__glMaterialf = (void (CALLBACK *)(GLenum, GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glMaterialf");

	__glMateriali = (void (CALLBACK *)(GLenum, GLenum, GLint))
	  GET_GL_PROCADDRESS ("glMateriali");

	__glMaterialfv = (void (CALLBACK *)(GLenum, GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glMaterialfv");

	__glMaterialiv = (void (CALLBACK *)(GLenum, GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glMaterialiv");

	__glGetMaterialfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetMaterialfv");

	__glGetMaterialiv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetMaterialiv");

	__glColorMaterial = (void (CALLBACK *)(GLenum, GLenum))
	  GET_GL_PROCADDRESS ("glColorMaterial");

	__glPixelZoom = (void (CALLBACK *)(GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glPixelZoom");

	__glPixelStoref = (void (CALLBACK *)(GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glPixelStoref");

	__glPixelStorei = (void (CALLBACK *)(GLenum, GLint))
	  GET_GL_PROCADDRESS ("glPixelStorei");

	__glPixelTransferf = (void (CALLBACK *)(GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glPixelTransferf");

	__glPixelTransferi = (void (CALLBACK *)(GLenum, GLint))
	  GET_GL_PROCADDRESS ("glPixelTransferi");

	__glPixelMapfv = (void (CALLBACK *)(GLenum, GLint, const GLfloat *))
	  GET_GL_PROCADDRESS ("glPixelMapfv");

	__glPixelMapuiv = (void (CALLBACK *)(GLenum, GLint, const GLuint *))
	  GET_GL_PROCADDRESS ("glPixelMapuiv");

	__glPixelMapusv = (void (CALLBACK *)(GLenum, GLint, const GLushort *))
	  GET_GL_PROCADDRESS ("glPixelMapusv");

	__glGetPixelMapfv = (void (CALLBACK *)(GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetPixelMapfv");

	__glGetPixelMapuiv = (void (CALLBACK *)(GLenum, GLuint *))
	  GET_GL_PROCADDRESS ("glGetPixelMapuiv");

	__glGetPixelMapusv = (void (CALLBACK *)(GLenum, GLushort *))
	  GET_GL_PROCADDRESS ("glGetPixelMapusv");

	__glBitmap = (void (CALLBACK *)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *))
	  GET_GL_PROCADDRESS ("glBitmap");

	__glReadPixels = (void (CALLBACK *)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *))
	  GET_GL_PROCADDRESS ("glReadPixels");

	__glDrawPixels = (void (CALLBACK *)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glDrawPixels");

	__glCopyPixels = (void (CALLBACK *)(GLint, GLint, GLsizei, GLsizei, GLenum))
	  GET_GL_PROCADDRESS ("glCopyPixels");

	__glStencilFunc = (void (CALLBACK *)(GLenum, GLint, GLuint))
	  GET_GL_PROCADDRESS ("glStencilFunc");

	__glStencilMask = (void (CALLBACK *)(GLuint))
	  GET_GL_PROCADDRESS ("glStencilMask");

	__glStencilOp = (void (CALLBACK *)(GLenum, GLenum, GLenum))
	  GET_GL_PROCADDRESS ("glStencilOp");

	__glClearStencil = (void (CALLBACK *)(GLint))
	  GET_GL_PROCADDRESS ("glClearStencil");

	__glTexGend = (void (CALLBACK *)(GLenum, GLenum, GLdouble))
	  GET_GL_PROCADDRESS ("glTexGend");

	__glTexGenf = (void (CALLBACK *)(GLenum, GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glTexGenf");

	__glTexGeni = (void (CALLBACK *)(GLenum, GLenum, GLint))
	  GET_GL_PROCADDRESS ("glTexGeni");

	__glTexGendv = (void (CALLBACK *)(GLenum, GLenum, const GLdouble *))
	  GET_GL_PROCADDRESS ("glTexGendv");

	__glTexGenfv = (void (CALLBACK *)(GLenum, GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glTexGenfv");

	__glTexGeniv = (void (CALLBACK *)(GLenum, GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glTexGeniv");

	__glGetTexGendv = (void (CALLBACK *)(GLenum, GLenum, GLdouble *))
	  GET_GL_PROCADDRESS ("glGetTexGendv");

	__glGetTexGenfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetTexGenfv");

	__glGetTexGeniv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetTexGeniv");

	__glTexEnvf = (void (CALLBACK *)(GLenum, GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glTexEnvf");

	__glTexEnvi = (void (CALLBACK *)(GLenum, GLenum, GLint))
	  GET_GL_PROCADDRESS ("glTexEnvi");

	__glTexEnvfv = (void (CALLBACK *)(GLenum, GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glTexEnvfv");

	__glTexEnviv = (void (CALLBACK *)(GLenum, GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glTexEnviv");

	__glGetTexEnvfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetTexEnvfv");

	__glGetTexEnviv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetTexEnviv");

	__glTexParameterf = (void (CALLBACK *)(GLenum, GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glTexParameterf");

	__glTexParameteri = (void (CALLBACK *)(GLenum, GLenum, GLint))
	  GET_GL_PROCADDRESS ("glTexParameteri");

	__glTexParameterfv = (void (CALLBACK *)(GLenum, GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glTexParameterfv");

	__glTexParameteriv = (void (CALLBACK *)(GLenum, GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glTexParameteriv");

	__glGetTexParameterfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetTexParameterfv");

	__glGetTexParameteriv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetTexParameteriv");

	__glGetTexLevelParameterfv = (void (CALLBACK *)(GLenum, GLint, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetTexLevelParameterfv");

	__glGetTexLevelParameteriv = (void (CALLBACK *)(GLenum, GLint, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetTexLevelParameteriv");

	__glTexImage1D = (void (CALLBACK *)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexImage1D");

	__glTexImage2D = (void (CALLBACK *)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexImage2D");

	__glGetTexImage = (void (CALLBACK *)(GLenum, GLint, GLenum, GLenum, GLvoid *))
	  GET_GL_PROCADDRESS ("glGetTexImage");

	__glGenTextures = (void (CALLBACK *)(GLsizei, GLuint *))
	  GET_GL_PROCADDRESS ("glGenTextures");

	__glDeleteTextures = (void (CALLBACK *)(GLsizei, const GLuint *))
	  GET_GL_PROCADDRESS ("glDeleteTextures");

	__glBindTexture = (void (CALLBACK *)(GLenum, GLuint))
	  GET_GL_PROCADDRESS ("glBindTexture");

	__glPrioritizeTextures = (void (CALLBACK *)(GLsizei, const GLuint *, const GLclampf *))
	  GET_GL_PROCADDRESS ("glPrioritizeTextures");

	__glAreTexturesResident = (GLboolean (CALLBACK *)(GLsizei, const GLuint *, GLboolean *))
	  GET_GL_PROCADDRESS ("glAreTexturesResident");

	__glIsTexture = (GLboolean (CALLBACK *)(GLuint))
	  GET_GL_PROCADDRESS ("glIsTexture");

	__glTexSubImage1D = (void (CALLBACK *)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexSubImage1D");

	__glTexSubImage2D = (void (CALLBACK *)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexSubImage2D");

	__glCopyTexImage1D = (void (CALLBACK *)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint))
	  GET_GL_PROCADDRESS ("glCopyTexImage1D");

	__glCopyTexImage2D = (void (CALLBACK *)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint))
	  GET_GL_PROCADDRESS ("glCopyTexImage2D");

	__glCopyTexSubImage1D = (void (CALLBACK *)(GLenum, GLint, GLint, GLint, GLint, GLsizei))
	  GET_GL_PROCADDRESS ("glCopyTexSubImage1D");

	__glCopyTexSubImage2D = (void (CALLBACK *)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei))
	  GET_GL_PROCADDRESS ("glCopyTexSubImage2D");

	__glMap1d = (void (CALLBACK *)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *))
	  GET_GL_PROCADDRESS ("glMap1d");

	__glMap1f = (void (CALLBACK *)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *))
	  GET_GL_PROCADDRESS ("glMap1f");

	__glMap2d = (void (CALLBACK *)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *))
	  GET_GL_PROCADDRESS ("glMap2d");

	__glMap2f = (void (CALLBACK *)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *))
	  GET_GL_PROCADDRESS ("glMap2f");

	__glGetMapdv = (void (CALLBACK *)(GLenum, GLenum, GLdouble *))
	  GET_GL_PROCADDRESS ("glGetMapdv");

	__glGetMapfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetMapfv");

	__glGetMapiv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetMapiv");

	__glEvalCoord1d = (void (CALLBACK *)(GLdouble))
	  GET_GL_PROCADDRESS ("glEvalCoord1d");

	__glEvalCoord1f = (void (CALLBACK *)(GLfloat))
	  GET_GL_PROCADDRESS ("glEvalCoord1f");

	__glEvalCoord1dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glEvalCoord1dv");

	__glEvalCoord1fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glEvalCoord1fv");

	__glEvalCoord2d = (void (CALLBACK *)(GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glEvalCoord2d");

	__glEvalCoord2f = (void (CALLBACK *)(GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glEvalCoord2f");

	__glEvalCoord2dv = (void (CALLBACK *)(const GLdouble *))
	  GET_GL_PROCADDRESS ("glEvalCoord2dv");

	__glEvalCoord2fv = (void (CALLBACK *)(const GLfloat *))
	  GET_GL_PROCADDRESS ("glEvalCoord2fv");

	__glMapGrid1d = (void (CALLBACK *)(GLint, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glMapGrid1d");

	__glMapGrid1f = (void (CALLBACK *)(GLint, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glMapGrid1f");

	__glMapGrid2d = (void (CALLBACK *)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glMapGrid2d");

	__glMapGrid2f = (void (CALLBACK *)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glMapGrid2f");

	__glEvalPoint1 = (void (CALLBACK *)(GLint))
	  GET_GL_PROCADDRESS ("glEvalPoint1");

	__glEvalPoint2 = (void (CALLBACK *)(GLint, GLint))
	  GET_GL_PROCADDRESS ("glEvalPoint2");

	__glEvalMesh1 = (void (CALLBACK *)(GLenum, GLint, GLint))
	  GET_GL_PROCADDRESS ("glEvalMesh1");

	__glEvalMesh2 = (void (CALLBACK *)(GLenum, GLint, GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glEvalMesh2");

	__glFogf = (void (CALLBACK *)(GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glFogf");

	__glFogi = (void (CALLBACK *)(GLenum, GLint))
	  GET_GL_PROCADDRESS ("glFogi");

	__glFogfv = (void (CALLBACK *)(GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glFogfv");

	__glFogiv = (void (CALLBACK *)(GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glFogiv");

	__glFeedbackBuffer = (void (CALLBACK *)(GLsizei, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glFeedbackBuffer");

	__glPassThrough = (void (CALLBACK *)(GLfloat))
	  GET_GL_PROCADDRESS ("glPassThrough");

	__glSelectBuffer = (void (CALLBACK *)(GLsizei, GLuint *))
	  GET_GL_PROCADDRESS ("glSelectBuffer");

	__glInitNames = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glInitNames");

	__glLoadName = (void (CALLBACK *)(GLuint))
	  GET_GL_PROCADDRESS ("glLoadName");

	__glPushName = (void (CALLBACK *)(GLuint))
	  GET_GL_PROCADDRESS ("glPushName");

	__glPopName = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glPopName");

	__glDrawRangeElements = (void (CALLBACK *)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glDrawRangeElements");

	__glTexImage3D = (void (CALLBACK *)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexImage3D");

	__glTexSubImage3D = (void (CALLBACK *)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexSubImage3D");

	__glCopyTexSubImage3D = (void (CALLBACK *)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei))
	  GET_GL_PROCADDRESS ("glCopyTexSubImage3D");

	__glColorTable = (void (CALLBACK *)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glColorTable");

	__glColorSubTable = (void (CALLBACK *)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glColorSubTable");

	__glColorTableParameteriv = (void (CALLBACK *)(GLenum, GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glColorTableParameteriv");

	__glColorTableParameterfv = (void (CALLBACK *)(GLenum, GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glColorTableParameterfv");

	__glCopyColorSubTable = (void (CALLBACK *)(GLenum, GLsizei, GLint, GLint, GLsizei))
	  GET_GL_PROCADDRESS ("glCopyColorSubTable");

	__glCopyColorTable = (void (CALLBACK *)(GLenum, GLenum, GLint, GLint, GLsizei))
	  GET_GL_PROCADDRESS ("glCopyColorTable");

	__glGetColorTable = (void (CALLBACK *)(GLenum, GLenum, GLenum, GLvoid *))
	  GET_GL_PROCADDRESS ("glGetColorTable");

	__glGetColorTableParameterfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetColorTableParameterfv");

	__glGetColorTableParameteriv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetColorTableParameteriv");

	__glBlendEquation = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glBlendEquation");

	__glBlendColor = (void (CALLBACK *)(GLclampf, GLclampf, GLclampf, GLclampf))
	  GET_GL_PROCADDRESS ("glBlendColor");

	__glHistogram = (void (CALLBACK *)(GLenum, GLsizei, GLenum, GLboolean))
	  GET_GL_PROCADDRESS ("glHistogram");

	__glResetHistogram = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glResetHistogram");

	__glGetHistogram = (void (CALLBACK *)(GLenum, GLboolean, GLenum, GLenum, GLvoid *))
	  GET_GL_PROCADDRESS ("glGetHistogram");

	__glGetHistogramParameterfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetHistogramParameterfv");

	__glGetHistogramParameteriv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetHistogramParameteriv");

	__glMinmax = (void (CALLBACK *)(GLenum, GLenum, GLboolean))
	  GET_GL_PROCADDRESS ("glMinmax");

	__glResetMinmax = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glResetMinmax");

	__glGetMinmax = (void (CALLBACK *)(GLenum, GLboolean, GLenum, GLenum, GLvoid *))
	  GET_GL_PROCADDRESS ("glGetMinmax");

	__glGetMinmaxParameterfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetMinmaxParameterfv");

	__glGetMinmaxParameteriv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetMinmaxParameteriv");

	__glConvolutionFilter1D = (void (CALLBACK *)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glConvolutionFilter1D");

	__glConvolutionFilter2D = (void (CALLBACK *)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glConvolutionFilter2D");

	__glConvolutionParameterf = (void (CALLBACK *)(GLenum, GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glConvolutionParameterf");

	__glConvolutionParameterfv = (void (CALLBACK *)(GLenum, GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glConvolutionParameterfv");

	__glConvolutionParameteri = (void (CALLBACK *)(GLenum, GLenum, GLint))
	  GET_GL_PROCADDRESS ("glConvolutionParameteri");

	__glConvolutionParameteriv = (void (CALLBACK *)(GLenum, GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glConvolutionParameteriv");

	__glCopyConvolutionFilter1D = (void (CALLBACK *)(GLenum, GLenum, GLint, GLint, GLsizei))
	  GET_GL_PROCADDRESS ("glCopyConvolutionFilter1D");

	__glCopyConvolutionFilter2D = (void (CALLBACK *)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei))
	  GET_GL_PROCADDRESS ("glCopyConvolutionFilter2D");

	__glGetConvolutionFilter = (void (CALLBACK *)(GLenum, GLenum, GLenum, GLvoid *))
	  GET_GL_PROCADDRESS ("glGetConvolutionFilter");

	__glGetConvolutionParameterfv = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetConvolutionParameterfv");

	__glGetConvolutionParameteriv = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetConvolutionParameteriv");

	__glSeparableFilter2D = (void (CALLBACK *)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *))
	  GET_GL_PROCADDRESS ("glSeparableFilter2D");

	__glGetSeparableFilter = (void (CALLBACK *)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *))
	  GET_GL_PROCADDRESS ("glGetSeparableFilter");

	__glBlendColorEXT = (void (CALLBACK *)(GLclampf, GLclampf, GLclampf, GLclampf))
	  GET_GL_PROCADDRESS ("glBlendColorEXT");

	__glPolygonOffsetEXT = (void (CALLBACK *)(GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glPolygonOffsetEXT");

	__glTexImage3DEXT = (void (CALLBACK *)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexImage3DEXT");

	__glTexSubImage3DEXT = (void (CALLBACK *)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexSubImage3DEXT");

	__glCopyTexSubImage3DEXT = (void (CALLBACK *)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei))
	  GET_GL_PROCADDRESS ("glCopyTexSubImage3DEXT");

	__glGenTexturesEXT = (void (CALLBACK *)(GLsizei, GLuint *))
	  GET_GL_PROCADDRESS ("glGenTexturesEXT");

	__glDeleteTexturesEXT = (void (CALLBACK *)(GLsizei, const GLuint *))
	  GET_GL_PROCADDRESS ("glDeleteTexturesEXT");

	__glBindTextureEXT = (void (CALLBACK *)(GLenum, GLuint))
	  GET_GL_PROCADDRESS ("glBindTextureEXT");

	__glPrioritizeTexturesEXT = (void (CALLBACK *)(GLsizei, const GLuint *, const GLclampf *))
	  GET_GL_PROCADDRESS ("glPrioritizeTexturesEXT");

	__glAreTexturesResidentEXT = (GLboolean (CALLBACK *)(GLsizei, const GLuint *, GLboolean *))
	  GET_GL_PROCADDRESS ("glAreTexturesResidentEXT");

	__glIsTextureEXT = (GLboolean (CALLBACK *)(GLuint))
	  GET_GL_PROCADDRESS ("glIsTextureEXT");

	__glVertexPointerEXT = (void (CALLBACK *)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glVertexPointerEXT");

	__glNormalPointerEXT = (void (CALLBACK *)(GLenum, GLsizei, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glNormalPointerEXT");

	__glColorPointerEXT = (void (CALLBACK *)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glColorPointerEXT");

	__glIndexPointerEXT = (void (CALLBACK *)(GLenum, GLsizei, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glIndexPointerEXT");

	__glTexCoordPointerEXT = (void (CALLBACK *)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *))
	  GET_GL_PROCADDRESS ("glTexCoordPointerEXT");

	__glEdgeFlagPointerEXT = (void (CALLBACK *)(GLsizei, GLsizei, const GLboolean *))
	  GET_GL_PROCADDRESS ("glEdgeFlagPointerEXT");

	__glGetPointervEXT = (void (CALLBACK *)(GLenum, void **))
	  GET_GL_PROCADDRESS ("glGetPointervEXT");

	__glArrayElementEXT = (void (CALLBACK *)(GLint))
	  GET_GL_PROCADDRESS ("glArrayElementEXT");

	__glDrawArraysEXT = (void (CALLBACK *)(GLenum, GLint, GLsizei))
	  GET_GL_PROCADDRESS ("glDrawArraysEXT");

	__glBlendEquationEXT = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glBlendEquationEXT");

	__glPointParameterfEXT = (void (CALLBACK *)(GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glPointParameterfEXT");

	__glPointParameterfvEXT = (void (CALLBACK *)(GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glPointParameterfvEXT");

	__glColorTableEXT = (void (CALLBACK *)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glColorTableEXT");

	__glColorSubTableEXT = (void (CALLBACK *)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *))
	  GET_GL_PROCADDRESS ("glColorSubTableEXT");

	__glGetColorTableEXT = (void (CALLBACK *)(GLenum, GLenum, GLenum, GLvoid *))
	  GET_GL_PROCADDRESS ("glGetColorTableEXT");

	__glGetColorTableParameterfvEXT = (void (CALLBACK *)(GLenum, GLenum, GLfloat *))
	  GET_GL_PROCADDRESS ("glGetColorTableParameterfvEXT");

	__glGetColorTableParameterivEXT = (void (CALLBACK *)(GLenum, GLenum, GLint *))
	  GET_GL_PROCADDRESS ("glGetColorTableParameterivEXT");

	__glLockArraysEXT = (void (CALLBACK *)(GLint, GLsizei))
	  GET_GL_PROCADDRESS ("glLockArraysEXT");

	__glUnlockArraysEXT = (void (CALLBACK *)())
	  GET_GL_PROCADDRESS ("glUnlockArraysEXT");

	__glActiveTextureARB = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glActiveTextureARB");

	__glClientActiveTextureARB = (void (CALLBACK *)(GLenum))
	  GET_GL_PROCADDRESS ("glClientActiveTextureARB");

	__glMultiTexCoord1dARB = (void (CALLBACK *)(GLenum, GLdouble))
	  GET_GL_PROCADDRESS ("glMultiTexCoord1dARB");

	__glMultiTexCoord1dvARB = (void (CALLBACK *)(GLenum, const GLdouble *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord1dvARB");

	__glMultiTexCoord1fARB = (void (CALLBACK *)(GLenum, GLfloat))
	  GET_GL_PROCADDRESS ("glMultiTexCoord1fARB");

	__glMultiTexCoord1fvARB = (void (CALLBACK *)(GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord1fvARB");

	__glMultiTexCoord1iARB = (void (CALLBACK *)(GLenum, GLint))
	  GET_GL_PROCADDRESS ("glMultiTexCoord1iARB");

	__glMultiTexCoord1ivARB = (void (CALLBACK *)(GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord1ivARB");

	__glMultiTexCoord1sARB = (void (CALLBACK *)(GLenum, GLshort))
	  GET_GL_PROCADDRESS ("glMultiTexCoord1sARB");

	__glMultiTexCoord1svARB = (void (CALLBACK *)(GLenum, const GLshort *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord1svARB");

	__glMultiTexCoord2dARB = (void (CALLBACK *)(GLenum, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glMultiTexCoord2dARB");

	__glMultiTexCoord2dvARB = (void (CALLBACK *)(GLenum, const GLdouble *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord2dvARB");

	__glMultiTexCoord2fARB = (void (CALLBACK *)(GLenum, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glMultiTexCoord2fARB");

	__glMultiTexCoord2fvARB = (void (CALLBACK *)(GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord2fvARB");

	__glMultiTexCoord2iARB = (void (CALLBACK *)(GLenum, GLint, GLint))
	  GET_GL_PROCADDRESS ("glMultiTexCoord2iARB");

	__glMultiTexCoord2ivARB = (void (CALLBACK *)(GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord2ivARB");

	__glMultiTexCoord2sARB = (void (CALLBACK *)(GLenum, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glMultiTexCoord2sARB");

	__glMultiTexCoord2svARB = (void (CALLBACK *)(GLenum, const GLshort *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord2svARB");

	__glMultiTexCoord3dARB = (void (CALLBACK *)(GLenum, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glMultiTexCoord3dARB");

	__glMultiTexCoord3dvARB = (void (CALLBACK *)(GLenum, const GLdouble *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord3dvARB");

	__glMultiTexCoord3fARB = (void (CALLBACK *)(GLenum, GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glMultiTexCoord3fARB");

	__glMultiTexCoord3fvARB = (void (CALLBACK *)(GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord3fvARB");

	__glMultiTexCoord3iARB = (void (CALLBACK *)(GLenum, GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glMultiTexCoord3iARB");

	__glMultiTexCoord3ivARB = (void (CALLBACK *)(GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord3ivARB");

	__glMultiTexCoord3sARB = (void (CALLBACK *)(GLenum, GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glMultiTexCoord3sARB");

	__glMultiTexCoord3svARB = (void (CALLBACK *)(GLenum, const GLshort *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord3svARB");

	__glMultiTexCoord4dARB = (void (CALLBACK *)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble))
	  GET_GL_PROCADDRESS ("glMultiTexCoord4dARB");

	__glMultiTexCoord4dvARB = (void (CALLBACK *)(GLenum, const GLdouble *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord4dvARB");

	__glMultiTexCoord4fARB = (void (CALLBACK *)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat))
	  GET_GL_PROCADDRESS ("glMultiTexCoord4fARB");

	__glMultiTexCoord4fvARB = (void (CALLBACK *)(GLenum, const GLfloat *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord4fvARB");

	__glMultiTexCoord4iARB = (void (CALLBACK *)(GLenum, GLint, GLint, GLint, GLint))
	  GET_GL_PROCADDRESS ("glMultiTexCoord4iARB");

	__glMultiTexCoord4ivARB = (void (CALLBACK *)(GLenum, const GLint *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord4ivARB");

	__glMultiTexCoord4sARB = (void (CALLBACK *)(GLenum, GLshort, GLshort, GLshort, GLshort))
	  GET_GL_PROCADDRESS ("glMultiTexCoord4sARB");

	__glMultiTexCoord4svARB = (void (CALLBACK *)(GLenum, const GLshort *))
	  GET_GL_PROCADDRESS ("glMultiTexCoord4svARB");

/* C2J Parser Version 1.6 Beta:  Java program parsed successfully. */ 
