	extern const GLubyte *(CALLBACK * __glGetString )( GLenum name );
/**
 * C2J Parser Version 1.6 Beta
 * Jausoft - Sven Goethel Software Development
 * Reading from file: gl-proto-auto.orig.h . . .
 * Destination-Class: aa ! 
 */

	extern void (CALLBACK * __glClearIndex )(GLfloat);

	extern void (CALLBACK * __glClearColor )(GLclampf, GLclampf, GLclampf, GLclampf);

	extern void (CALLBACK * __glClear )(GLbitfield);

	extern void (CALLBACK * __glIndexMask )(GLuint);

	extern void (CALLBACK * __glColorMask )(GLboolean, GLboolean, GLboolean, GLboolean);

	extern void (CALLBACK * __glAlphaFunc )(GLenum, GLclampf);

	extern void (CALLBACK * __glBlendFunc )(GLenum, GLenum);

	extern void (CALLBACK * __glLogicOp )(GLenum);

	extern void (CALLBACK * __glCullFace )(GLenum);

	extern void (CALLBACK * __glFrontFace )(GLenum);

	extern void (CALLBACK * __glPointSize )(GLfloat);

	extern void (CALLBACK * __glLineWidth )(GLfloat);

	extern void (CALLBACK * __glLineStipple )(GLint, GLushort);

	extern void (CALLBACK * __glPolygonMode )(GLenum, GLenum);

	extern void (CALLBACK * __glPolygonOffset )(GLfloat, GLfloat);

	extern void (CALLBACK * __glPolygonStipple )(const GLubyte *);

	extern void (CALLBACK * __glGetPolygonStipple )(GLubyte *);

	extern void (CALLBACK * __glEdgeFlag )(GLboolean);

	extern void (CALLBACK * __glEdgeFlagv )(const GLboolean *);

	extern void (CALLBACK * __glScissor )(GLint, GLint, GLsizei, GLsizei);

	extern void (CALLBACK * __glClipPlane )(GLenum, const GLdouble *);

	extern void (CALLBACK * __glGetClipPlane )(GLenum, GLdouble *);

	extern void (CALLBACK * __glDrawBuffer )(GLenum);

	extern void (CALLBACK * __glReadBuffer )(GLenum);

	extern void (CALLBACK * __glEnable )(GLenum);

	extern void (CALLBACK * __glDisable )(GLenum);

	extern GLboolean (CALLBACK * __glIsEnabled )(GLenum);

	extern void (CALLBACK * __glEnableClientState )(GLenum);

	extern void (CALLBACK * __glDisableClientState )(GLenum);

	extern void (CALLBACK * __glGetBooleanv )(GLenum, GLboolean *);

	extern void (CALLBACK * __glGetDoublev )(GLenum, GLdouble *);

	extern void (CALLBACK * __glGetFloatv )(GLenum, GLfloat *);

	extern void (CALLBACK * __glGetIntegerv )(GLenum, GLint *);

	extern void (CALLBACK * __glPushAttrib )(GLbitfield);

	extern void (CALLBACK * __glPopAttrib )();

	extern void (CALLBACK * __glPushClientAttrib )(GLbitfield);

	extern void (CALLBACK * __glPopClientAttrib )();

	extern GLint (CALLBACK * __glRenderMode )(GLenum);

	extern GLenum (CALLBACK * __glGetError )();

	extern void (CALLBACK * __glFinish )();

	extern void (CALLBACK * __glFlush )();

	extern void (CALLBACK * __glHint )(GLenum, GLenum);

	extern void (CALLBACK * __glClearDepth )(GLclampd);

	extern void (CALLBACK * __glDepthFunc )(GLenum);

	extern void (CALLBACK * __glDepthMask )(GLboolean);

	extern void (CALLBACK * __glDepthRange )(GLclampd, GLclampd);

	extern void (CALLBACK * __glClearAccum )(GLfloat, GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glAccum )(GLenum, GLfloat);

	extern void (CALLBACK * __glMatrixMode )(GLenum);

	extern void (CALLBACK * __glOrtho )(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glFrustum )(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glViewport )(GLint, GLint, GLsizei, GLsizei);

	extern void (CALLBACK * __glPushMatrix )();

	extern void (CALLBACK * __glPopMatrix )();

	extern void (CALLBACK * __glLoadIdentity )();

	extern void (CALLBACK * __glLoadMatrixd )(const GLdouble *);

	extern void (CALLBACK * __glLoadMatrixf )(const GLfloat *);

	extern void (CALLBACK * __glMultMatrixd )(const GLdouble *);

	extern void (CALLBACK * __glMultMatrixf )(const GLfloat *);

	extern void (CALLBACK * __glRotated )(GLdouble, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glRotatef )(GLfloat, GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glScaled )(GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glScalef )(GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glTranslated )(GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glTranslatef )(GLfloat, GLfloat, GLfloat);

	extern GLboolean (CALLBACK * __glIsList )(GLuint);

	extern void (CALLBACK * __glDeleteLists )(GLuint, GLsizei);

	extern GLuint (CALLBACK * __glGenLists )(GLsizei);

	extern void (CALLBACK * __glNewList )(GLuint, GLenum);

	extern void (CALLBACK * __glEndList )();

	extern void (CALLBACK * __glCallList )(GLuint);

	extern void (CALLBACK * __glCallLists )(GLsizei, GLenum, const GLvoid *);

	extern void (CALLBACK * __glListBase )(GLuint);

	extern void (CALLBACK * __glBegin )(GLenum);

	extern void (CALLBACK * __glEnd )();

	extern void (CALLBACK * __glVertex2d )(GLdouble, GLdouble);

	extern void (CALLBACK * __glVertex2f )(GLfloat, GLfloat);

	extern void (CALLBACK * __glVertex2i )(GLint, GLint);

	extern void (CALLBACK * __glVertex2s )(GLshort, GLshort);

	extern void (CALLBACK * __glVertex3d )(GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glVertex3f )(GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glVertex3i )(GLint, GLint, GLint);

	extern void (CALLBACK * __glVertex3s )(GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glVertex4d )(GLdouble, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glVertex4f )(GLfloat, GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glVertex4i )(GLint, GLint, GLint, GLint);

	extern void (CALLBACK * __glVertex4s )(GLshort, GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glVertex2dv )(const GLdouble *);

	extern void (CALLBACK * __glVertex2fv )(const GLfloat *);

	extern void (CALLBACK * __glVertex2iv )(const GLint *);

	extern void (CALLBACK * __glVertex2sv )(const GLshort *);

	extern void (CALLBACK * __glVertex3dv )(const GLdouble *);

	extern void (CALLBACK * __glVertex3fv )(const GLfloat *);

	extern void (CALLBACK * __glVertex3iv )(const GLint *);

	extern void (CALLBACK * __glVertex3sv )(const GLshort *);

	extern void (CALLBACK * __glVertex4dv )(const GLdouble *);

	extern void (CALLBACK * __glVertex4fv )(const GLfloat *);

	extern void (CALLBACK * __glVertex4iv )(const GLint *);

	extern void (CALLBACK * __glVertex4sv )(const GLshort *);

	extern void (CALLBACK * __glNormal3b )(GLbyte, GLbyte, GLbyte);

	extern void (CALLBACK * __glNormal3d )(GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glNormal3f )(GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glNormal3i )(GLint, GLint, GLint);

	extern void (CALLBACK * __glNormal3s )(GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glNormal3bv )(const GLbyte *);

	extern void (CALLBACK * __glNormal3dv )(const GLdouble *);

	extern void (CALLBACK * __glNormal3fv )(const GLfloat *);

	extern void (CALLBACK * __glNormal3iv )(const GLint *);

	extern void (CALLBACK * __glNormal3sv )(const GLshort *);

	extern void (CALLBACK * __glIndexd )(GLdouble);

	extern void (CALLBACK * __glIndexf )(GLfloat);

	extern void (CALLBACK * __glIndexi )(GLint);

	extern void (CALLBACK * __glIndexs )(GLshort);

	extern void (CALLBACK * __glIndexub )(GLubyte);

	extern void (CALLBACK * __glIndexdv )(const GLdouble *);

	extern void (CALLBACK * __glIndexfv )(const GLfloat *);

	extern void (CALLBACK * __glIndexiv )(const GLint *);

	extern void (CALLBACK * __glIndexsv )(const GLshort *);

	extern void (CALLBACK * __glIndexubv )(const GLubyte *);

	extern void (CALLBACK * __glColor3b )(GLbyte, GLbyte, GLbyte);

	extern void (CALLBACK * __glColor3d )(GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glColor3f )(GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glColor3i )(GLint, GLint, GLint);

	extern void (CALLBACK * __glColor3s )(GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glColor3ub )(GLubyte, GLubyte, GLubyte);

	extern void (CALLBACK * __glColor3ui )(GLuint, GLuint, GLuint);

	extern void (CALLBACK * __glColor3us )(GLushort, GLushort, GLushort);

	extern void (CALLBACK * __glColor4b )(GLbyte, GLbyte, GLbyte, GLbyte);

	extern void (CALLBACK * __glColor4d )(GLdouble, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glColor4f )(GLfloat, GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glColor4i )(GLint, GLint, GLint, GLint);

	extern void (CALLBACK * __glColor4s )(GLshort, GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glColor4ub )(GLubyte, GLubyte, GLubyte, GLubyte);

	extern void (CALLBACK * __glColor4ui )(GLuint, GLuint, GLuint, GLuint);

	extern void (CALLBACK * __glColor4us )(GLushort, GLushort, GLushort, GLushort);

	extern void (CALLBACK * __glColor3bv )(const GLbyte *);

	extern void (CALLBACK * __glColor3dv )(const GLdouble *);

	extern void (CALLBACK * __glColor3fv )(const GLfloat *);

	extern void (CALLBACK * __glColor3iv )(const GLint *);

	extern void (CALLBACK * __glColor3sv )(const GLshort *);

	extern void (CALLBACK * __glColor3ubv )(const GLubyte *);

	extern void (CALLBACK * __glColor3uiv )(const GLuint *);

	extern void (CALLBACK * __glColor3usv )(const GLushort *);

	extern void (CALLBACK * __glColor4bv )(const GLbyte *);

	extern void (CALLBACK * __glColor4dv )(const GLdouble *);

	extern void (CALLBACK * __glColor4fv )(const GLfloat *);

	extern void (CALLBACK * __glColor4iv )(const GLint *);

	extern void (CALLBACK * __glColor4sv )(const GLshort *);

	extern void (CALLBACK * __glColor4ubv )(const GLubyte *);

	extern void (CALLBACK * __glColor4uiv )(const GLuint *);

	extern void (CALLBACK * __glColor4usv )(const GLushort *);

	extern void (CALLBACK * __glTexCoord1d )(GLdouble);

	extern void (CALLBACK * __glTexCoord1f )(GLfloat);

	extern void (CALLBACK * __glTexCoord1i )(GLint);

	extern void (CALLBACK * __glTexCoord1s )(GLshort);

	extern void (CALLBACK * __glTexCoord2d )(GLdouble, GLdouble);

	extern void (CALLBACK * __glTexCoord2f )(GLfloat, GLfloat);

	extern void (CALLBACK * __glTexCoord2i )(GLint, GLint);

	extern void (CALLBACK * __glTexCoord2s )(GLshort, GLshort);

	extern void (CALLBACK * __glTexCoord3d )(GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glTexCoord3f )(GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glTexCoord3i )(GLint, GLint, GLint);

	extern void (CALLBACK * __glTexCoord3s )(GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glTexCoord4d )(GLdouble, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glTexCoord4f )(GLfloat, GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glTexCoord4i )(GLint, GLint, GLint, GLint);

	extern void (CALLBACK * __glTexCoord4s )(GLshort, GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glTexCoord1dv )(const GLdouble *);

	extern void (CALLBACK * __glTexCoord1fv )(const GLfloat *);

	extern void (CALLBACK * __glTexCoord1iv )(const GLint *);

	extern void (CALLBACK * __glTexCoord1sv )(const GLshort *);

	extern void (CALLBACK * __glTexCoord2dv )(const GLdouble *);

	extern void (CALLBACK * __glTexCoord2fv )(const GLfloat *);

	extern void (CALLBACK * __glTexCoord2iv )(const GLint *);

	extern void (CALLBACK * __glTexCoord2sv )(const GLshort *);

	extern void (CALLBACK * __glTexCoord3dv )(const GLdouble *);

	extern void (CALLBACK * __glTexCoord3fv )(const GLfloat *);

	extern void (CALLBACK * __glTexCoord3iv )(const GLint *);

	extern void (CALLBACK * __glTexCoord3sv )(const GLshort *);

	extern void (CALLBACK * __glTexCoord4dv )(const GLdouble *);

	extern void (CALLBACK * __glTexCoord4fv )(const GLfloat *);

	extern void (CALLBACK * __glTexCoord4iv )(const GLint *);

	extern void (CALLBACK * __glTexCoord4sv )(const GLshort *);

	extern void (CALLBACK * __glRasterPos2d )(GLdouble, GLdouble);

	extern void (CALLBACK * __glRasterPos2f )(GLfloat, GLfloat);

	extern void (CALLBACK * __glRasterPos2i )(GLint, GLint);

	extern void (CALLBACK * __glRasterPos2s )(GLshort, GLshort);

	extern void (CALLBACK * __glRasterPos3d )(GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glRasterPos3f )(GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glRasterPos3i )(GLint, GLint, GLint);

	extern void (CALLBACK * __glRasterPos3s )(GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glRasterPos4d )(GLdouble, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glRasterPos4f )(GLfloat, GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glRasterPos4i )(GLint, GLint, GLint, GLint);

	extern void (CALLBACK * __glRasterPos4s )(GLshort, GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glRasterPos2dv )(const GLdouble *);

	extern void (CALLBACK * __glRasterPos2fv )(const GLfloat *);

	extern void (CALLBACK * __glRasterPos2iv )(const GLint *);

	extern void (CALLBACK * __glRasterPos2sv )(const GLshort *);

	extern void (CALLBACK * __glRasterPos3dv )(const GLdouble *);

	extern void (CALLBACK * __glRasterPos3fv )(const GLfloat *);

	extern void (CALLBACK * __glRasterPos3iv )(const GLint *);

	extern void (CALLBACK * __glRasterPos3sv )(const GLshort *);

	extern void (CALLBACK * __glRasterPos4dv )(const GLdouble *);

	extern void (CALLBACK * __glRasterPos4fv )(const GLfloat *);

	extern void (CALLBACK * __glRasterPos4iv )(const GLint *);

	extern void (CALLBACK * __glRasterPos4sv )(const GLshort *);

	extern void (CALLBACK * __glRectd )(GLdouble, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glRectf )(GLfloat, GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glRecti )(GLint, GLint, GLint, GLint);

	extern void (CALLBACK * __glRects )(GLshort, GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glRectdv )(const GLdouble *, const GLdouble *);

	extern void (CALLBACK * __glRectfv )(const GLfloat *, const GLfloat *);

	extern void (CALLBACK * __glRectiv )(const GLint *, const GLint *);

	extern void (CALLBACK * __glRectsv )(const GLshort *, const GLshort *);

	extern void (CALLBACK * __glVertexPointer )(GLint, GLenum, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glNormalPointer )(GLenum, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glColorPointer )(GLint, GLenum, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glIndexPointer )(GLenum, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glTexCoordPointer )(GLint, GLenum, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glEdgeFlagPointer )(GLsizei, const GLvoid *);

	extern void (CALLBACK * __glGetPointerv )(GLenum, void **);

	extern void (CALLBACK * __glArrayElement )(GLint);

	extern void (CALLBACK * __glDrawArrays )(GLenum, GLint, GLsizei);

	extern void (CALLBACK * __glDrawElements )(GLenum, GLsizei, GLenum, const GLvoid *);

	extern void (CALLBACK * __glInterleavedArrays )(GLenum, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glShadeModel )(GLenum);

	extern void (CALLBACK * __glLightf )(GLenum, GLenum, GLfloat);

	extern void (CALLBACK * __glLighti )(GLenum, GLenum, GLint);

	extern void (CALLBACK * __glLightfv )(GLenum, GLenum, const GLfloat *);

	extern void (CALLBACK * __glLightiv )(GLenum, GLenum, const GLint *);

	extern void (CALLBACK * __glGetLightfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetLightiv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glLightModelf )(GLenum, GLfloat);

	extern void (CALLBACK * __glLightModeli )(GLenum, GLint);

	extern void (CALLBACK * __glLightModelfv )(GLenum, const GLfloat *);

	extern void (CALLBACK * __glLightModeliv )(GLenum, const GLint *);

	extern void (CALLBACK * __glMaterialf )(GLenum, GLenum, GLfloat);

	extern void (CALLBACK * __glMateriali )(GLenum, GLenum, GLint);

	extern void (CALLBACK * __glMaterialfv )(GLenum, GLenum, const GLfloat *);

	extern void (CALLBACK * __glMaterialiv )(GLenum, GLenum, const GLint *);

	extern void (CALLBACK * __glGetMaterialfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetMaterialiv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glColorMaterial )(GLenum, GLenum);

	extern void (CALLBACK * __glPixelZoom )(GLfloat, GLfloat);

	extern void (CALLBACK * __glPixelStoref )(GLenum, GLfloat);

	extern void (CALLBACK * __glPixelStorei )(GLenum, GLint);

	extern void (CALLBACK * __glPixelTransferf )(GLenum, GLfloat);

	extern void (CALLBACK * __glPixelTransferi )(GLenum, GLint);

	extern void (CALLBACK * __glPixelMapfv )(GLenum, GLint, const GLfloat *);

	extern void (CALLBACK * __glPixelMapuiv )(GLenum, GLint, const GLuint *);

	extern void (CALLBACK * __glPixelMapusv )(GLenum, GLint, const GLushort *);

	extern void (CALLBACK * __glGetPixelMapfv )(GLenum, GLfloat *);

	extern void (CALLBACK * __glGetPixelMapuiv )(GLenum, GLuint *);

	extern void (CALLBACK * __glGetPixelMapusv )(GLenum, GLushort *);

	extern void (CALLBACK * __glBitmap )(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);

	extern void (CALLBACK * __glReadPixels )(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);

	extern void (CALLBACK * __glDrawPixels )(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glCopyPixels )(GLint, GLint, GLsizei, GLsizei, GLenum);

	extern void (CALLBACK * __glStencilFunc )(GLenum, GLint, GLuint);

	extern void (CALLBACK * __glStencilMask )(GLuint);

	extern void (CALLBACK * __glStencilOp )(GLenum, GLenum, GLenum);

	extern void (CALLBACK * __glClearStencil )(GLint);

	extern void (CALLBACK * __glTexGend )(GLenum, GLenum, GLdouble);

	extern void (CALLBACK * __glTexGenf )(GLenum, GLenum, GLfloat);

	extern void (CALLBACK * __glTexGeni )(GLenum, GLenum, GLint);

	extern void (CALLBACK * __glTexGendv )(GLenum, GLenum, const GLdouble *);

	extern void (CALLBACK * __glTexGenfv )(GLenum, GLenum, const GLfloat *);

	extern void (CALLBACK * __glTexGeniv )(GLenum, GLenum, const GLint *);

	extern void (CALLBACK * __glGetTexGendv )(GLenum, GLenum, GLdouble *);

	extern void (CALLBACK * __glGetTexGenfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetTexGeniv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glTexEnvf )(GLenum, GLenum, GLfloat);

	extern void (CALLBACK * __glTexEnvi )(GLenum, GLenum, GLint);

	extern void (CALLBACK * __glTexEnvfv )(GLenum, GLenum, const GLfloat *);

	extern void (CALLBACK * __glTexEnviv )(GLenum, GLenum, const GLint *);

	extern void (CALLBACK * __glGetTexEnvfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetTexEnviv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glTexParameterf )(GLenum, GLenum, GLfloat);

	extern void (CALLBACK * __glTexParameteri )(GLenum, GLenum, GLint);

	extern void (CALLBACK * __glTexParameterfv )(GLenum, GLenum, const GLfloat *);

	extern void (CALLBACK * __glTexParameteriv )(GLenum, GLenum, const GLint *);

	extern void (CALLBACK * __glGetTexParameterfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetTexParameteriv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glGetTexLevelParameterfv )(GLenum, GLint, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetTexLevelParameteriv )(GLenum, GLint, GLenum, GLint *);

	extern void (CALLBACK * __glTexImage1D )(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glTexImage2D )(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glGetTexImage )(GLenum, GLint, GLenum, GLenum, GLvoid *);

	extern void (CALLBACK * __glGenTextures )(GLsizei, GLuint *);

	extern void (CALLBACK * __glDeleteTextures )(GLsizei, const GLuint *);

	extern void (CALLBACK * __glBindTexture )(GLenum, GLuint);

	extern void (CALLBACK * __glPrioritizeTextures )(GLsizei, const GLuint *, const GLclampf *);

	extern GLboolean (CALLBACK * __glAreTexturesResident )(GLsizei, const GLuint *, GLboolean *);

	extern GLboolean (CALLBACK * __glIsTexture )(GLuint);

	extern void (CALLBACK * __glTexSubImage1D )(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glTexSubImage2D )(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glCopyTexImage1D )(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);

	extern void (CALLBACK * __glCopyTexImage2D )(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);

	extern void (CALLBACK * __glCopyTexSubImage1D )(GLenum, GLint, GLint, GLint, GLint, GLsizei);

	extern void (CALLBACK * __glCopyTexSubImage2D )(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);

	extern void (CALLBACK * __glMap1d )(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *);

	extern void (CALLBACK * __glMap1f )(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);

	extern void (CALLBACK * __glMap2d )(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *);

	extern void (CALLBACK * __glMap2f )(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *);

	extern void (CALLBACK * __glGetMapdv )(GLenum, GLenum, GLdouble *);

	extern void (CALLBACK * __glGetMapfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetMapiv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glEvalCoord1d )(GLdouble);

	extern void (CALLBACK * __glEvalCoord1f )(GLfloat);

	extern void (CALLBACK * __glEvalCoord1dv )(const GLdouble *);

	extern void (CALLBACK * __glEvalCoord1fv )(const GLfloat *);

	extern void (CALLBACK * __glEvalCoord2d )(GLdouble, GLdouble);

	extern void (CALLBACK * __glEvalCoord2f )(GLfloat, GLfloat);

	extern void (CALLBACK * __glEvalCoord2dv )(const GLdouble *);

	extern void (CALLBACK * __glEvalCoord2fv )(const GLfloat *);

	extern void (CALLBACK * __glMapGrid1d )(GLint, GLdouble, GLdouble);

	extern void (CALLBACK * __glMapGrid1f )(GLint, GLfloat, GLfloat);

	extern void (CALLBACK * __glMapGrid2d )(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);

	extern void (CALLBACK * __glMapGrid2f )(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);

	extern void (CALLBACK * __glEvalPoint1 )(GLint);

	extern void (CALLBACK * __glEvalPoint2 )(GLint, GLint);

	extern void (CALLBACK * __glEvalMesh1 )(GLenum, GLint, GLint);

	extern void (CALLBACK * __glEvalMesh2 )(GLenum, GLint, GLint, GLint, GLint);

	extern void (CALLBACK * __glFogf )(GLenum, GLfloat);

	extern void (CALLBACK * __glFogi )(GLenum, GLint);

	extern void (CALLBACK * __glFogfv )(GLenum, const GLfloat *);

	extern void (CALLBACK * __glFogiv )(GLenum, const GLint *);

	extern void (CALLBACK * __glFeedbackBuffer )(GLsizei, GLenum, GLfloat *);

	extern void (CALLBACK * __glPassThrough )(GLfloat);

	extern void (CALLBACK * __glSelectBuffer )(GLsizei, GLuint *);

	extern void (CALLBACK * __glInitNames )();

	extern void (CALLBACK * __glLoadName )(GLuint);

	extern void (CALLBACK * __glPushName )(GLuint);

	extern void (CALLBACK * __glPopName )();

	extern void (CALLBACK * __glDrawRangeElements )(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);

	extern void (CALLBACK * __glTexImage3D )(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glTexSubImage3D )(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glCopyTexSubImage3D )(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);

	extern void (CALLBACK * __glColorTable )(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glColorSubTable )(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glColorTableParameteriv )(GLenum, GLenum, const GLint *);

	extern void (CALLBACK * __glColorTableParameterfv )(GLenum, GLenum, const GLfloat *);

	extern void (CALLBACK * __glCopyColorSubTable )(GLenum, GLsizei, GLint, GLint, GLsizei);

	extern void (CALLBACK * __glCopyColorTable )(GLenum, GLenum, GLint, GLint, GLsizei);

	extern void (CALLBACK * __glGetColorTable )(GLenum, GLenum, GLenum, GLvoid *);

	extern void (CALLBACK * __glGetColorTableParameterfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetColorTableParameteriv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glBlendEquation )(GLenum);

	extern void (CALLBACK * __glBlendColor )(GLclampf, GLclampf, GLclampf, GLclampf);

	extern void (CALLBACK * __glHistogram )(GLenum, GLsizei, GLenum, GLboolean);

	extern void (CALLBACK * __glResetHistogram )(GLenum);

	extern void (CALLBACK * __glGetHistogram )(GLenum, GLboolean, GLenum, GLenum, GLvoid *);

	extern void (CALLBACK * __glGetHistogramParameterfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetHistogramParameteriv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glMinmax )(GLenum, GLenum, GLboolean);

	extern void (CALLBACK * __glResetMinmax )(GLenum);

	extern void (CALLBACK * __glGetMinmax )(GLenum, GLboolean, GLenum, GLenum, GLvoid *);

	extern void (CALLBACK * __glGetMinmaxParameterfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetMinmaxParameteriv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glConvolutionFilter1D )(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glConvolutionFilter2D )(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glConvolutionParameterf )(GLenum, GLenum, GLfloat);

	extern void (CALLBACK * __glConvolutionParameterfv )(GLenum, GLenum, const GLfloat *);

	extern void (CALLBACK * __glConvolutionParameteri )(GLenum, GLenum, GLint);

	extern void (CALLBACK * __glConvolutionParameteriv )(GLenum, GLenum, const GLint *);

	extern void (CALLBACK * __glCopyConvolutionFilter1D )(GLenum, GLenum, GLint, GLint, GLsizei);

	extern void (CALLBACK * __glCopyConvolutionFilter2D )(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei);

	extern void (CALLBACK * __glGetConvolutionFilter )(GLenum, GLenum, GLenum, GLvoid *);

	extern void (CALLBACK * __glGetConvolutionParameterfv )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetConvolutionParameteriv )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glSeparableFilter2D )(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *);

	extern void (CALLBACK * __glGetSeparableFilter )(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *);

	extern void (CALLBACK * __glBlendColorEXT )(GLclampf, GLclampf, GLclampf, GLclampf);

	extern void (CALLBACK * __glPolygonOffsetEXT )(GLfloat, GLfloat);

	extern void (CALLBACK * __glTexImage3DEXT )(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glTexSubImage3DEXT )(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glCopyTexSubImage3DEXT )(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);

	extern void (CALLBACK * __glGenTexturesEXT )(GLsizei, GLuint *);

	extern void (CALLBACK * __glDeleteTexturesEXT )(GLsizei, const GLuint *);

	extern void (CALLBACK * __glBindTextureEXT )(GLenum, GLuint);

	extern void (CALLBACK * __glPrioritizeTexturesEXT )(GLsizei, const GLuint *, const GLclampf *);

	extern GLboolean (CALLBACK * __glAreTexturesResidentEXT )(GLsizei, const GLuint *, GLboolean *);

	extern GLboolean (CALLBACK * __glIsTextureEXT )(GLuint);

	extern void (CALLBACK * __glVertexPointerEXT )(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glNormalPointerEXT )(GLenum, GLsizei, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glColorPointerEXT )(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glIndexPointerEXT )(GLenum, GLsizei, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glTexCoordPointerEXT )(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);

	extern void (CALLBACK * __glEdgeFlagPointerEXT )(GLsizei, GLsizei, const GLboolean *);

	extern void (CALLBACK * __glGetPointervEXT )(GLenum, void **);

	extern void (CALLBACK * __glArrayElementEXT )(GLint);

	extern void (CALLBACK * __glDrawArraysEXT )(GLenum, GLint, GLsizei);

	extern void (CALLBACK * __glBlendEquationEXT )(GLenum);

	extern void (CALLBACK * __glPointParameterfEXT )(GLenum, GLfloat);

	extern void (CALLBACK * __glPointParameterfvEXT )(GLenum, const GLfloat *);

	extern void (CALLBACK * __glColorTableEXT )(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glColorSubTableEXT )(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

	extern void (CALLBACK * __glGetColorTableEXT )(GLenum, GLenum, GLenum, GLvoid *);

	extern void (CALLBACK * __glGetColorTableParameterfvEXT )(GLenum, GLenum, GLfloat *);

	extern void (CALLBACK * __glGetColorTableParameterivEXT )(GLenum, GLenum, GLint *);

	extern void (CALLBACK * __glLockArraysEXT )(GLint, GLsizei);

	extern void (CALLBACK * __glUnlockArraysEXT )();

	extern void (CALLBACK * __glActiveTextureARB )(GLenum);

	extern void (CALLBACK * __glClientActiveTextureARB )(GLenum);

	extern void (CALLBACK * __glMultiTexCoord1dARB )(GLenum, GLdouble);

	extern void (CALLBACK * __glMultiTexCoord1dvARB )(GLenum, const GLdouble *);

	extern void (CALLBACK * __glMultiTexCoord1fARB )(GLenum, GLfloat);

	extern void (CALLBACK * __glMultiTexCoord1fvARB )(GLenum, const GLfloat *);

	extern void (CALLBACK * __glMultiTexCoord1iARB )(GLenum, GLint);

	extern void (CALLBACK * __glMultiTexCoord1ivARB )(GLenum, const GLint *);

	extern void (CALLBACK * __glMultiTexCoord1sARB )(GLenum, GLshort);

	extern void (CALLBACK * __glMultiTexCoord1svARB )(GLenum, const GLshort *);

	extern void (CALLBACK * __glMultiTexCoord2dARB )(GLenum, GLdouble, GLdouble);

	extern void (CALLBACK * __glMultiTexCoord2dvARB )(GLenum, const GLdouble *);

	extern void (CALLBACK * __glMultiTexCoord2fARB )(GLenum, GLfloat, GLfloat);

	extern void (CALLBACK * __glMultiTexCoord2fvARB )(GLenum, const GLfloat *);

	extern void (CALLBACK * __glMultiTexCoord2iARB )(GLenum, GLint, GLint);

	extern void (CALLBACK * __glMultiTexCoord2ivARB )(GLenum, const GLint *);

	extern void (CALLBACK * __glMultiTexCoord2sARB )(GLenum, GLshort, GLshort);

	extern void (CALLBACK * __glMultiTexCoord2svARB )(GLenum, const GLshort *);

	extern void (CALLBACK * __glMultiTexCoord3dARB )(GLenum, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glMultiTexCoord3dvARB )(GLenum, const GLdouble *);

	extern void (CALLBACK * __glMultiTexCoord3fARB )(GLenum, GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glMultiTexCoord3fvARB )(GLenum, const GLfloat *);

	extern void (CALLBACK * __glMultiTexCoord3iARB )(GLenum, GLint, GLint, GLint);

	extern void (CALLBACK * __glMultiTexCoord3ivARB )(GLenum, const GLint *);

	extern void (CALLBACK * __glMultiTexCoord3sARB )(GLenum, GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glMultiTexCoord3svARB )(GLenum, const GLshort *);

	extern void (CALLBACK * __glMultiTexCoord4dARB )(GLenum, GLdouble, GLdouble, GLdouble, GLdouble);

	extern void (CALLBACK * __glMultiTexCoord4dvARB )(GLenum, const GLdouble *);

	extern void (CALLBACK * __glMultiTexCoord4fARB )(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);

	extern void (CALLBACK * __glMultiTexCoord4fvARB )(GLenum, const GLfloat *);

	extern void (CALLBACK * __glMultiTexCoord4iARB )(GLenum, GLint, GLint, GLint, GLint);

	extern void (CALLBACK * __glMultiTexCoord4ivARB )(GLenum, const GLint *);

	extern void (CALLBACK * __glMultiTexCoord4sARB )(GLenum, GLshort, GLshort, GLshort, GLshort);

	extern void (CALLBACK * __glMultiTexCoord4svARB )(GLenum, const GLshort *);

/* C2J Parser Version 1.6 Beta:  Java program parsed successfully. */ 
