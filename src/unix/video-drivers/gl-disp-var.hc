	const GLubyte *(CALLBACK * __glGetString )( GLenum name ) = NULL;
/**
 * C2J Parser Version 1.6 Beta
 * Jausoft - Sven Goethel Software Development
 * Reading from file: gl-proto-auto.orig.h . . .
 * Destination-Class: aa ! 
 */

	void (CALLBACK * __glClearIndex )(GLfloat) = NULL;

	void (CALLBACK * __glClearColor )(GLclampf, GLclampf, GLclampf, GLclampf) = NULL;

	void (CALLBACK * __glClear )(GLbitfield) = NULL;

	void (CALLBACK * __glIndexMask )(GLuint) = NULL;

	void (CALLBACK * __glColorMask )(GLboolean, GLboolean, GLboolean, GLboolean) = NULL;

	void (CALLBACK * __glAlphaFunc )(GLenum, GLclampf) = NULL;

	void (CALLBACK * __glBlendFunc )(GLenum, GLenum) = NULL;

	void (CALLBACK * __glLogicOp )(GLenum) = NULL;

	void (CALLBACK * __glCullFace )(GLenum) = NULL;

	void (CALLBACK * __glFrontFace )(GLenum) = NULL;

	void (CALLBACK * __glPointSize )(GLfloat) = NULL;

	void (CALLBACK * __glLineWidth )(GLfloat) = NULL;

	void (CALLBACK * __glLineStipple )(GLint, GLushort) = NULL;

	void (CALLBACK * __glPolygonMode )(GLenum, GLenum) = NULL;

	void (CALLBACK * __glPolygonOffset )(GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glPolygonStipple )(const GLubyte *) = NULL;

	void (CALLBACK * __glGetPolygonStipple )(GLubyte *) = NULL;

	void (CALLBACK * __glEdgeFlag )(GLboolean) = NULL;

	void (CALLBACK * __glEdgeFlagv )(const GLboolean *) = NULL;

	void (CALLBACK * __glScissor )(GLint, GLint, GLsizei, GLsizei) = NULL;

	void (CALLBACK * __glClipPlane )(GLenum, const GLdouble *) = NULL;

	void (CALLBACK * __glGetClipPlane )(GLenum, GLdouble *) = NULL;

	void (CALLBACK * __glDrawBuffer )(GLenum) = NULL;

	void (CALLBACK * __glReadBuffer )(GLenum) = NULL;

	void (CALLBACK * __glEnable )(GLenum) = NULL;

	void (CALLBACK * __glDisable )(GLenum) = NULL;

	GLboolean (CALLBACK * __glIsEnabled )(GLenum) = NULL;

	void (CALLBACK * __glEnableClientState )(GLenum) = NULL;

	void (CALLBACK * __glDisableClientState )(GLenum) = NULL;

	void (CALLBACK * __glGetBooleanv )(GLenum, GLboolean *) = NULL;

	void (CALLBACK * __glGetDoublev )(GLenum, GLdouble *) = NULL;

	void (CALLBACK * __glGetFloatv )(GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetIntegerv )(GLenum, GLint *) = NULL;

	void (CALLBACK * __glPushAttrib )(GLbitfield) = NULL;

	void (CALLBACK * __glPopAttrib )() = NULL;

	void (CALLBACK * __glPushClientAttrib )(GLbitfield) = NULL;

	void (CALLBACK * __glPopClientAttrib )() = NULL;

	GLint (CALLBACK * __glRenderMode )(GLenum) = NULL;

	GLenum (CALLBACK * __glGetError )() = NULL;

	void (CALLBACK * __glFinish )() = NULL;

	void (CALLBACK * __glFlush )() = NULL;

	void (CALLBACK * __glHint )(GLenum, GLenum) = NULL;

	void (CALLBACK * __glClearDepth )(GLclampd) = NULL;

	void (CALLBACK * __glDepthFunc )(GLenum) = NULL;

	void (CALLBACK * __glDepthMask )(GLboolean) = NULL;

	void (CALLBACK * __glDepthRange )(GLclampd, GLclampd) = NULL;

	void (CALLBACK * __glClearAccum )(GLfloat, GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glAccum )(GLenum, GLfloat) = NULL;

	void (CALLBACK * __glMatrixMode )(GLenum) = NULL;

	void (CALLBACK * __glOrtho )(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glFrustum )(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glViewport )(GLint, GLint, GLsizei, GLsizei) = NULL;

	void (CALLBACK * __glPushMatrix )() = NULL;

	void (CALLBACK * __glPopMatrix )() = NULL;

	void (CALLBACK * __glLoadIdentity )() = NULL;

	void (CALLBACK * __glLoadMatrixd )(const GLdouble *) = NULL;

	void (CALLBACK * __glLoadMatrixf )(const GLfloat *) = NULL;

	void (CALLBACK * __glMultMatrixd )(const GLdouble *) = NULL;

	void (CALLBACK * __glMultMatrixf )(const GLfloat *) = NULL;

	void (CALLBACK * __glRotated )(GLdouble, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glRotatef )(GLfloat, GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glScaled )(GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glScalef )(GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glTranslated )(GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glTranslatef )(GLfloat, GLfloat, GLfloat) = NULL;

	GLboolean (CALLBACK * __glIsList )(GLuint) = NULL;

	void (CALLBACK * __glDeleteLists )(GLuint, GLsizei) = NULL;

	GLuint (CALLBACK * __glGenLists )(GLsizei) = NULL;

	void (CALLBACK * __glNewList )(GLuint, GLenum) = NULL;

	void (CALLBACK * __glEndList )() = NULL;

	void (CALLBACK * __glCallList )(GLuint) = NULL;

	void (CALLBACK * __glCallLists )(GLsizei, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glListBase )(GLuint) = NULL;

	void (CALLBACK * __glBegin )(GLenum) = NULL;

	void (CALLBACK * __glEnd )() = NULL;

	void (CALLBACK * __glVertex2d )(GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glVertex2f )(GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glVertex2i )(GLint, GLint) = NULL;

	void (CALLBACK * __glVertex2s )(GLshort, GLshort) = NULL;

	void (CALLBACK * __glVertex3d )(GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glVertex3f )(GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glVertex3i )(GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glVertex3s )(GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glVertex4d )(GLdouble, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glVertex4f )(GLfloat, GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glVertex4i )(GLint, GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glVertex4s )(GLshort, GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glVertex2dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glVertex2fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glVertex2iv )(const GLint *) = NULL;

	void (CALLBACK * __glVertex2sv )(const GLshort *) = NULL;

	void (CALLBACK * __glVertex3dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glVertex3fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glVertex3iv )(const GLint *) = NULL;

	void (CALLBACK * __glVertex3sv )(const GLshort *) = NULL;

	void (CALLBACK * __glVertex4dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glVertex4fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glVertex4iv )(const GLint *) = NULL;

	void (CALLBACK * __glVertex4sv )(const GLshort *) = NULL;

	void (CALLBACK * __glNormal3b )(GLbyte, GLbyte, GLbyte) = NULL;

	void (CALLBACK * __glNormal3d )(GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glNormal3f )(GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glNormal3i )(GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glNormal3s )(GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glNormal3bv )(const GLbyte *) = NULL;

	void (CALLBACK * __glNormal3dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glNormal3fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glNormal3iv )(const GLint *) = NULL;

	void (CALLBACK * __glNormal3sv )(const GLshort *) = NULL;

	void (CALLBACK * __glIndexd )(GLdouble) = NULL;

	void (CALLBACK * __glIndexf )(GLfloat) = NULL;

	void (CALLBACK * __glIndexi )(GLint) = NULL;

	void (CALLBACK * __glIndexs )(GLshort) = NULL;

	void (CALLBACK * __glIndexub )(GLubyte) = NULL;

	void (CALLBACK * __glIndexdv )(const GLdouble *) = NULL;

	void (CALLBACK * __glIndexfv )(const GLfloat *) = NULL;

	void (CALLBACK * __glIndexiv )(const GLint *) = NULL;

	void (CALLBACK * __glIndexsv )(const GLshort *) = NULL;

	void (CALLBACK * __glIndexubv )(const GLubyte *) = NULL;

	void (CALLBACK * __glColor3b )(GLbyte, GLbyte, GLbyte) = NULL;

	void (CALLBACK * __glColor3d )(GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glColor3f )(GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glColor3i )(GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glColor3s )(GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glColor3ub )(GLubyte, GLubyte, GLubyte) = NULL;

	void (CALLBACK * __glColor3ui )(GLuint, GLuint, GLuint) = NULL;

	void (CALLBACK * __glColor3us )(GLushort, GLushort, GLushort) = NULL;

	void (CALLBACK * __glColor4b )(GLbyte, GLbyte, GLbyte, GLbyte) = NULL;

	void (CALLBACK * __glColor4d )(GLdouble, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glColor4f )(GLfloat, GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glColor4i )(GLint, GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glColor4s )(GLshort, GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glColor4ub )(GLubyte, GLubyte, GLubyte, GLubyte) = NULL;

	void (CALLBACK * __glColor4ui )(GLuint, GLuint, GLuint, GLuint) = NULL;

	void (CALLBACK * __glColor4us )(GLushort, GLushort, GLushort, GLushort) = NULL;

	void (CALLBACK * __glColor3bv )(const GLbyte *) = NULL;

	void (CALLBACK * __glColor3dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glColor3fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glColor3iv )(const GLint *) = NULL;

	void (CALLBACK * __glColor3sv )(const GLshort *) = NULL;

	void (CALLBACK * __glColor3ubv )(const GLubyte *) = NULL;

	void (CALLBACK * __glColor3uiv )(const GLuint *) = NULL;

	void (CALLBACK * __glColor3usv )(const GLushort *) = NULL;

	void (CALLBACK * __glColor4bv )(const GLbyte *) = NULL;

	void (CALLBACK * __glColor4dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glColor4fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glColor4iv )(const GLint *) = NULL;

	void (CALLBACK * __glColor4sv )(const GLshort *) = NULL;

	void (CALLBACK * __glColor4ubv )(const GLubyte *) = NULL;

	void (CALLBACK * __glColor4uiv )(const GLuint *) = NULL;

	void (CALLBACK * __glColor4usv )(const GLushort *) = NULL;

	void (CALLBACK * __glTexCoord1d )(GLdouble) = NULL;

	void (CALLBACK * __glTexCoord1f )(GLfloat) = NULL;

	void (CALLBACK * __glTexCoord1i )(GLint) = NULL;

	void (CALLBACK * __glTexCoord1s )(GLshort) = NULL;

	void (CALLBACK * __glTexCoord2d )(GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glTexCoord2f )(GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glTexCoord2i )(GLint, GLint) = NULL;

	void (CALLBACK * __glTexCoord2s )(GLshort, GLshort) = NULL;

	void (CALLBACK * __glTexCoord3d )(GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glTexCoord3f )(GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glTexCoord3i )(GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glTexCoord3s )(GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glTexCoord4d )(GLdouble, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glTexCoord4f )(GLfloat, GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glTexCoord4i )(GLint, GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glTexCoord4s )(GLshort, GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glTexCoord1dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glTexCoord1fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glTexCoord1iv )(const GLint *) = NULL;

	void (CALLBACK * __glTexCoord1sv )(const GLshort *) = NULL;

	void (CALLBACK * __glTexCoord2dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glTexCoord2fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glTexCoord2iv )(const GLint *) = NULL;

	void (CALLBACK * __glTexCoord2sv )(const GLshort *) = NULL;

	void (CALLBACK * __glTexCoord3dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glTexCoord3fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glTexCoord3iv )(const GLint *) = NULL;

	void (CALLBACK * __glTexCoord3sv )(const GLshort *) = NULL;

	void (CALLBACK * __glTexCoord4dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glTexCoord4fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glTexCoord4iv )(const GLint *) = NULL;

	void (CALLBACK * __glTexCoord4sv )(const GLshort *) = NULL;

	void (CALLBACK * __glRasterPos2d )(GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glRasterPos2f )(GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glRasterPos2i )(GLint, GLint) = NULL;

	void (CALLBACK * __glRasterPos2s )(GLshort, GLshort) = NULL;

	void (CALLBACK * __glRasterPos3d )(GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glRasterPos3f )(GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glRasterPos3i )(GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glRasterPos3s )(GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glRasterPos4d )(GLdouble, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glRasterPos4f )(GLfloat, GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glRasterPos4i )(GLint, GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glRasterPos4s )(GLshort, GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glRasterPos2dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glRasterPos2fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glRasterPos2iv )(const GLint *) = NULL;

	void (CALLBACK * __glRasterPos2sv )(const GLshort *) = NULL;

	void (CALLBACK * __glRasterPos3dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glRasterPos3fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glRasterPos3iv )(const GLint *) = NULL;

	void (CALLBACK * __glRasterPos3sv )(const GLshort *) = NULL;

	void (CALLBACK * __glRasterPos4dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glRasterPos4fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glRasterPos4iv )(const GLint *) = NULL;

	void (CALLBACK * __glRasterPos4sv )(const GLshort *) = NULL;

	void (CALLBACK * __glRectd )(GLdouble, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glRectf )(GLfloat, GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glRecti )(GLint, GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glRects )(GLshort, GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glRectdv )(const GLdouble *, const GLdouble *) = NULL;

	void (CALLBACK * __glRectfv )(const GLfloat *, const GLfloat *) = NULL;

	void (CALLBACK * __glRectiv )(const GLint *, const GLint *) = NULL;

	void (CALLBACK * __glRectsv )(const GLshort *, const GLshort *) = NULL;

	void (CALLBACK * __glVertexPointer )(GLint, GLenum, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glNormalPointer )(GLenum, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glColorPointer )(GLint, GLenum, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glIndexPointer )(GLenum, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glTexCoordPointer )(GLint, GLenum, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glEdgeFlagPointer )(GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glGetPointerv )(GLenum, void **) = NULL;

	void (CALLBACK * __glArrayElement )(GLint) = NULL;

	void (CALLBACK * __glDrawArrays )(GLenum, GLint, GLsizei) = NULL;

	void (CALLBACK * __glDrawElements )(GLenum, GLsizei, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glInterleavedArrays )(GLenum, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glShadeModel )(GLenum) = NULL;

	void (CALLBACK * __glLightf )(GLenum, GLenum, GLfloat) = NULL;

	void (CALLBACK * __glLighti )(GLenum, GLenum, GLint) = NULL;

	void (CALLBACK * __glLightfv )(GLenum, GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glLightiv )(GLenum, GLenum, const GLint *) = NULL;

	void (CALLBACK * __glGetLightfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetLightiv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glLightModelf )(GLenum, GLfloat) = NULL;

	void (CALLBACK * __glLightModeli )(GLenum, GLint) = NULL;

	void (CALLBACK * __glLightModelfv )(GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glLightModeliv )(GLenum, const GLint *) = NULL;

	void (CALLBACK * __glMaterialf )(GLenum, GLenum, GLfloat) = NULL;

	void (CALLBACK * __glMateriali )(GLenum, GLenum, GLint) = NULL;

	void (CALLBACK * __glMaterialfv )(GLenum, GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glMaterialiv )(GLenum, GLenum, const GLint *) = NULL;

	void (CALLBACK * __glGetMaterialfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetMaterialiv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glColorMaterial )(GLenum, GLenum) = NULL;

	void (CALLBACK * __glPixelZoom )(GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glPixelStoref )(GLenum, GLfloat) = NULL;

	void (CALLBACK * __glPixelStorei )(GLenum, GLint) = NULL;

	void (CALLBACK * __glPixelTransferf )(GLenum, GLfloat) = NULL;

	void (CALLBACK * __glPixelTransferi )(GLenum, GLint) = NULL;

	void (CALLBACK * __glPixelMapfv )(GLenum, GLint, const GLfloat *) = NULL;

	void (CALLBACK * __glPixelMapuiv )(GLenum, GLint, const GLuint *) = NULL;

	void (CALLBACK * __glPixelMapusv )(GLenum, GLint, const GLushort *) = NULL;

	void (CALLBACK * __glGetPixelMapfv )(GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetPixelMapuiv )(GLenum, GLuint *) = NULL;

	void (CALLBACK * __glGetPixelMapusv )(GLenum, GLushort *) = NULL;

	void (CALLBACK * __glBitmap )(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *) = NULL;

	void (CALLBACK * __glReadPixels )(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *) = NULL;

	void (CALLBACK * __glDrawPixels )(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glCopyPixels )(GLint, GLint, GLsizei, GLsizei, GLenum) = NULL;

	void (CALLBACK * __glStencilFunc )(GLenum, GLint, GLuint) = NULL;

	void (CALLBACK * __glStencilMask )(GLuint) = NULL;

	void (CALLBACK * __glStencilOp )(GLenum, GLenum, GLenum) = NULL;

	void (CALLBACK * __glClearStencil )(GLint) = NULL;

	void (CALLBACK * __glTexGend )(GLenum, GLenum, GLdouble) = NULL;

	void (CALLBACK * __glTexGenf )(GLenum, GLenum, GLfloat) = NULL;

	void (CALLBACK * __glTexGeni )(GLenum, GLenum, GLint) = NULL;

	void (CALLBACK * __glTexGendv )(GLenum, GLenum, const GLdouble *) = NULL;

	void (CALLBACK * __glTexGenfv )(GLenum, GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glTexGeniv )(GLenum, GLenum, const GLint *) = NULL;

	void (CALLBACK * __glGetTexGendv )(GLenum, GLenum, GLdouble *) = NULL;

	void (CALLBACK * __glGetTexGenfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetTexGeniv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glTexEnvf )(GLenum, GLenum, GLfloat) = NULL;

	void (CALLBACK * __glTexEnvi )(GLenum, GLenum, GLint) = NULL;

	void (CALLBACK * __glTexEnvfv )(GLenum, GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glTexEnviv )(GLenum, GLenum, const GLint *) = NULL;

	void (CALLBACK * __glGetTexEnvfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetTexEnviv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glTexParameterf )(GLenum, GLenum, GLfloat) = NULL;

	void (CALLBACK * __glTexParameteri )(GLenum, GLenum, GLint) = NULL;

	void (CALLBACK * __glTexParameterfv )(GLenum, GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glTexParameteriv )(GLenum, GLenum, const GLint *) = NULL;

	void (CALLBACK * __glGetTexParameterfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetTexParameteriv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glGetTexLevelParameterfv )(GLenum, GLint, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetTexLevelParameteriv )(GLenum, GLint, GLenum, GLint *) = NULL;

	void (CALLBACK * __glTexImage1D )(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glTexImage2D )(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glGetTexImage )(GLenum, GLint, GLenum, GLenum, GLvoid *) = NULL;

	void (CALLBACK * __glGenTextures )(GLsizei, GLuint *) = NULL;

	void (CALLBACK * __glDeleteTextures )(GLsizei, const GLuint *) = NULL;

	void (CALLBACK * __glBindTexture )(GLenum, GLuint) = NULL;

	void (CALLBACK * __glPrioritizeTextures )(GLsizei, const GLuint *, const GLclampf *) = NULL;

	GLboolean (CALLBACK * __glAreTexturesResident )(GLsizei, const GLuint *, GLboolean *) = NULL;

	GLboolean (CALLBACK * __glIsTexture )(GLuint) = NULL;

	void (CALLBACK * __glTexSubImage1D )(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glTexSubImage2D )(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glCopyTexImage1D )(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint) = NULL;

	void (CALLBACK * __glCopyTexImage2D )(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint) = NULL;

	void (CALLBACK * __glCopyTexSubImage1D )(GLenum, GLint, GLint, GLint, GLint, GLsizei) = NULL;

	void (CALLBACK * __glCopyTexSubImage2D )(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei) = NULL;

	void (CALLBACK * __glMap1d )(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *) = NULL;

	void (CALLBACK * __glMap1f )(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *) = NULL;

	void (CALLBACK * __glMap2d )(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *) = NULL;

	void (CALLBACK * __glMap2f )(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *) = NULL;

	void (CALLBACK * __glGetMapdv )(GLenum, GLenum, GLdouble *) = NULL;

	void (CALLBACK * __glGetMapfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetMapiv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glEvalCoord1d )(GLdouble) = NULL;

	void (CALLBACK * __glEvalCoord1f )(GLfloat) = NULL;

	void (CALLBACK * __glEvalCoord1dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glEvalCoord1fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glEvalCoord2d )(GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glEvalCoord2f )(GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glEvalCoord2dv )(const GLdouble *) = NULL;

	void (CALLBACK * __glEvalCoord2fv )(const GLfloat *) = NULL;

	void (CALLBACK * __glMapGrid1d )(GLint, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glMapGrid1f )(GLint, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glMapGrid2d )(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glMapGrid2f )(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glEvalPoint1 )(GLint) = NULL;

	void (CALLBACK * __glEvalPoint2 )(GLint, GLint) = NULL;

	void (CALLBACK * __glEvalMesh1 )(GLenum, GLint, GLint) = NULL;

	void (CALLBACK * __glEvalMesh2 )(GLenum, GLint, GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glFogf )(GLenum, GLfloat) = NULL;

	void (CALLBACK * __glFogi )(GLenum, GLint) = NULL;

	void (CALLBACK * __glFogfv )(GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glFogiv )(GLenum, const GLint *) = NULL;

	void (CALLBACK * __glFeedbackBuffer )(GLsizei, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glPassThrough )(GLfloat) = NULL;

	void (CALLBACK * __glSelectBuffer )(GLsizei, GLuint *) = NULL;

	void (CALLBACK * __glInitNames )() = NULL;

	void (CALLBACK * __glLoadName )(GLuint) = NULL;

	void (CALLBACK * __glPushName )(GLuint) = NULL;

	void (CALLBACK * __glPopName )() = NULL;

	void (CALLBACK * __glDrawRangeElements )(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glTexImage3D )(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glTexSubImage3D )(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glCopyTexSubImage3D )(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei) = NULL;

	void (CALLBACK * __glColorTable )(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glColorSubTable )(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glColorTableParameteriv )(GLenum, GLenum, const GLint *) = NULL;

	void (CALLBACK * __glColorTableParameterfv )(GLenum, GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glCopyColorSubTable )(GLenum, GLsizei, GLint, GLint, GLsizei) = NULL;

	void (CALLBACK * __glCopyColorTable )(GLenum, GLenum, GLint, GLint, GLsizei) = NULL;

	void (CALLBACK * __glGetColorTable )(GLenum, GLenum, GLenum, GLvoid *) = NULL;

	void (CALLBACK * __glGetColorTableParameterfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetColorTableParameteriv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glBlendEquation )(GLenum) = NULL;

	void (CALLBACK * __glBlendColor )(GLclampf, GLclampf, GLclampf, GLclampf) = NULL;

	void (CALLBACK * __glHistogram )(GLenum, GLsizei, GLenum, GLboolean) = NULL;

	void (CALLBACK * __glResetHistogram )(GLenum) = NULL;

	void (CALLBACK * __glGetHistogram )(GLenum, GLboolean, GLenum, GLenum, GLvoid *) = NULL;

	void (CALLBACK * __glGetHistogramParameterfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetHistogramParameteriv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glMinmax )(GLenum, GLenum, GLboolean) = NULL;

	void (CALLBACK * __glResetMinmax )(GLenum) = NULL;

	void (CALLBACK * __glGetMinmax )(GLenum, GLboolean, GLenum, GLenum, GLvoid *) = NULL;

	void (CALLBACK * __glGetMinmaxParameterfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetMinmaxParameteriv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glConvolutionFilter1D )(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glConvolutionFilter2D )(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glConvolutionParameterf )(GLenum, GLenum, GLfloat) = NULL;

	void (CALLBACK * __glConvolutionParameterfv )(GLenum, GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glConvolutionParameteri )(GLenum, GLenum, GLint) = NULL;

	void (CALLBACK * __glConvolutionParameteriv )(GLenum, GLenum, const GLint *) = NULL;

	void (CALLBACK * __glCopyConvolutionFilter1D )(GLenum, GLenum, GLint, GLint, GLsizei) = NULL;

	void (CALLBACK * __glCopyConvolutionFilter2D )(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei) = NULL;

	void (CALLBACK * __glGetConvolutionFilter )(GLenum, GLenum, GLenum, GLvoid *) = NULL;

	void (CALLBACK * __glGetConvolutionParameterfv )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetConvolutionParameteriv )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glSeparableFilter2D )(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *) = NULL;

	void (CALLBACK * __glGetSeparableFilter )(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *) = NULL;

	void (CALLBACK * __glBlendColorEXT )(GLclampf, GLclampf, GLclampf, GLclampf) = NULL;

	void (CALLBACK * __glPolygonOffsetEXT )(GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glTexImage3DEXT )(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glTexSubImage3DEXT )(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glCopyTexSubImage3DEXT )(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei) = NULL;

	void (CALLBACK * __glGenTexturesEXT )(GLsizei, GLuint *) = NULL;

	void (CALLBACK * __glDeleteTexturesEXT )(GLsizei, const GLuint *) = NULL;

	void (CALLBACK * __glBindTextureEXT )(GLenum, GLuint) = NULL;

	void (CALLBACK * __glPrioritizeTexturesEXT )(GLsizei, const GLuint *, const GLclampf *) = NULL;

	GLboolean (CALLBACK * __glAreTexturesResidentEXT )(GLsizei, const GLuint *, GLboolean *) = NULL;

	GLboolean (CALLBACK * __glIsTextureEXT )(GLuint) = NULL;

	void (CALLBACK * __glVertexPointerEXT )(GLint, GLenum, GLsizei, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glNormalPointerEXT )(GLenum, GLsizei, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glColorPointerEXT )(GLint, GLenum, GLsizei, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glIndexPointerEXT )(GLenum, GLsizei, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glTexCoordPointerEXT )(GLint, GLenum, GLsizei, GLsizei, const GLvoid *) = NULL;

	void (CALLBACK * __glEdgeFlagPointerEXT )(GLsizei, GLsizei, const GLboolean *) = NULL;

	void (CALLBACK * __glGetPointervEXT )(GLenum, void **) = NULL;

	void (CALLBACK * __glArrayElementEXT )(GLint) = NULL;

	void (CALLBACK * __glDrawArraysEXT )(GLenum, GLint, GLsizei) = NULL;

	void (CALLBACK * __glBlendEquationEXT )(GLenum) = NULL;

	void (CALLBACK * __glPointParameterfEXT )(GLenum, GLfloat) = NULL;

	void (CALLBACK * __glPointParameterfvEXT )(GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glColorTableEXT )(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glColorSubTableEXT )(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *) = NULL;

	void (CALLBACK * __glGetColorTableEXT )(GLenum, GLenum, GLenum, GLvoid *) = NULL;

	void (CALLBACK * __glGetColorTableParameterfvEXT )(GLenum, GLenum, GLfloat *) = NULL;

	void (CALLBACK * __glGetColorTableParameterivEXT )(GLenum, GLenum, GLint *) = NULL;

	void (CALLBACK * __glLockArraysEXT )(GLint, GLsizei) = NULL;

	void (CALLBACK * __glUnlockArraysEXT )() = NULL;

	void (CALLBACK * __glActiveTextureARB )(GLenum) = NULL;

	void (CALLBACK * __glClientActiveTextureARB )(GLenum) = NULL;

	void (CALLBACK * __glMultiTexCoord1dARB )(GLenum, GLdouble) = NULL;

	void (CALLBACK * __glMultiTexCoord1dvARB )(GLenum, const GLdouble *) = NULL;

	void (CALLBACK * __glMultiTexCoord1fARB )(GLenum, GLfloat) = NULL;

	void (CALLBACK * __glMultiTexCoord1fvARB )(GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glMultiTexCoord1iARB )(GLenum, GLint) = NULL;

	void (CALLBACK * __glMultiTexCoord1ivARB )(GLenum, const GLint *) = NULL;

	void (CALLBACK * __glMultiTexCoord1sARB )(GLenum, GLshort) = NULL;

	void (CALLBACK * __glMultiTexCoord1svARB )(GLenum, const GLshort *) = NULL;

	void (CALLBACK * __glMultiTexCoord2dARB )(GLenum, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glMultiTexCoord2dvARB )(GLenum, const GLdouble *) = NULL;

	void (CALLBACK * __glMultiTexCoord2fARB )(GLenum, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glMultiTexCoord2fvARB )(GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glMultiTexCoord2iARB )(GLenum, GLint, GLint) = NULL;

	void (CALLBACK * __glMultiTexCoord2ivARB )(GLenum, const GLint *) = NULL;

	void (CALLBACK * __glMultiTexCoord2sARB )(GLenum, GLshort, GLshort) = NULL;

	void (CALLBACK * __glMultiTexCoord2svARB )(GLenum, const GLshort *) = NULL;

	void (CALLBACK * __glMultiTexCoord3dARB )(GLenum, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glMultiTexCoord3dvARB )(GLenum, const GLdouble *) = NULL;

	void (CALLBACK * __glMultiTexCoord3fARB )(GLenum, GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glMultiTexCoord3fvARB )(GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glMultiTexCoord3iARB )(GLenum, GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glMultiTexCoord3ivARB )(GLenum, const GLint *) = NULL;

	void (CALLBACK * __glMultiTexCoord3sARB )(GLenum, GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glMultiTexCoord3svARB )(GLenum, const GLshort *) = NULL;

	void (CALLBACK * __glMultiTexCoord4dARB )(GLenum, GLdouble, GLdouble, GLdouble, GLdouble) = NULL;

	void (CALLBACK * __glMultiTexCoord4dvARB )(GLenum, const GLdouble *) = NULL;

	void (CALLBACK * __glMultiTexCoord4fARB )(GLenum, GLfloat, GLfloat, GLfloat, GLfloat) = NULL;

	void (CALLBACK * __glMultiTexCoord4fvARB )(GLenum, const GLfloat *) = NULL;

	void (CALLBACK * __glMultiTexCoord4iARB )(GLenum, GLint, GLint, GLint, GLint) = NULL;

	void (CALLBACK * __glMultiTexCoord4ivARB )(GLenum, const GLint *) = NULL;

	void (CALLBACK * __glMultiTexCoord4sARB )(GLenum, GLshort, GLshort, GLshort, GLshort) = NULL;

	void (CALLBACK * __glMultiTexCoord4svARB )(GLenum, const GLshort *) = NULL;

/* C2J Parser Version 1.6 Beta:  Java program parsed successfully. */ 
