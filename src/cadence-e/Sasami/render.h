// --------------------------------------------------------------------------
// render.h - This contains the Sasami OpenGL renderer headers
// --------------------------------------------------------------------------

#ifndef __sa_render__

#define __sa_render__

#include <gl.h>
#include <glu.h>

#include "structures.h"

#include "togl.h"

// --------------------------------------------------------------------------
// Time (in milliseconds) between forced redraws - determines framerate
// --------------------------------------------------------------------------

#define SA_R_REDRAW_TIME 20

// --------------------------------------------------------------------------
// Functions
// --------------------------------------------------------------------------

int sa_r_opendisplay(void);
void sa_r_closedisplay(void);
void sa_r_init();
void sa_r_render(struct Togl *);
void sa_r_update(void);
void sa_r_resizeviewport(int x,int y);
void sa_r_setshowaxes(int s);
GLint sa_getandbindnewtexture();
void sa_loadtexture(char *f);

#endif
