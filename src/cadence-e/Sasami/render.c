/*
 *  This file is part of Eden.
 *
 *  Eden is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Eden is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Eden; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * render.c
 *
 * Rewritten from Ben Carter's original Windows-only version to one
 * using Togl which is thus portable across more platforms by Ash May
 * 2001
 */

#include "../../../../../config.h"

#ifdef WANT_SASAMI

#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
#	define __WIN32__
#   define _WIN32
#   endif
#endif

/* This bit copied from glpng.c - if we don't do this, we end up with
   pngLoad referenced here and pngLoad@16 compiled in glpng.o [Ash] */
#ifdef _WIN32 /* Stupid Windows needs to include windows.h before gl.h */
	#undef FAR
	#include <windows.h>
#endif



#include "glpng.h"

#include "togl.h"

#include "../Eden/eden.h"

#include "render.h"     /* this includes windows.h */

#include <math.h>		/* for sqrt etc */
#include <string.h>		/* for strlen etc [Richard] */

#include <gl.h>
#include <glu.h>

#include "utils.h"
#include "structures.h"

/* Globals which can be read/written by Eden */
int sa_r_xsize;	/* Display width (Cannot be set with display open) */
int sa_r_ysize;	/* Display height (Cannot be set with display open) */
colourinfo sa_r_bgcolour; /* Viewport background colour */
bool sa_r_showaxes; /* Are the axes shown? */

/* Camera information */
coord sa_camera_pos; /* Camera position */
coord sa_camera_rot; /* Camera rotation */
coord sa_camera_scale; /* Camera scale [Ash] */
int sa_camera_rotthentrans; /* Rotation, then translation in camera (1),
			   or translation then rotation (0).  0 was
			   how Sasami originally behaved: the camera
			   appears to rotate around a world located at
			   the origin.  1 allows implementation of a
			   camera moving and rotating /in/ the world.
			   [Ash] */
double sa_camera_frustum_left = -1; /* [Ash] */
double sa_camera_frustum_right = 1;
double sa_camera_frustum_bottom = -(640/480); /* don't know why in integer */
double sa_camera_frustum_top = 640/480;
double sa_camera_frustum_near = 10.0;
double sa_camera_frustum_far = 40000.0;

/* Default lighting info */
GLfloat		sa_def_light_pos1[4]	= {0.7,0.7,1.25,0};
GLfloat		sa_def_light_pos2[4]	= {-0.7,-0.7,-1.25,0};
GLfloat		sa_def_light_ambient[4]	= {0.8,0.8,0.8,1};
GLfloat		sa_def_light_specular[4]= {0.9,0.9,0.9,1};
GLfloat		sa_def_light_diffuse[4] = {0.7,0.7,0.7,1};

struct Togl *globTogl; /* keeping a copy to use within sa_r_update -
			  this is a hack [Ash] */

/* Internal globals */
bool sa_r_initialised = false;
extern Tcl_Interp *interp;
int sa_r_maxlights = 0;	/* The maximum number of lights the OpenGL
                           driver allows */

#ifdef DEBUG
#  define DEBUGPRINT(s,i) if (Debug&8) fprintf(stderr, s, i);
#  include <sys/time.h>
   struct timeval lastRenderTime;
   bool lastRenderTimeValid = false;
#else
#  define DEBUGPRINT(s,i)
#endif

/* Set/get camera rotation */



#ifdef USE_TCL_CONST84_OPTION
int getXrot(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[])
#else
int getXrot(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
#endif
{
	sprintf(interp->result, "%f", sa_camera_rot.x);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int getYrot(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[]) 
#else
int getYrot(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) 
#endif
{
	sprintf(interp->result, "%f", sa_camera_rot.y);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int getZrot(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[])
#else
int getZrot(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
#endif
{
	sprintf(interp->result, "%f", sa_camera_rot.z);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int setXrot(struct Togl *togl, int argc, CONST84 char *argv[])
#else
int setXrot(struct Togl *togl, int argc, char *argv[])
#endif
{
  Tcl_Interp *interp = Togl_Interp(togl);

  if (argc != 3) {
    Tcl_SetResult( interp,
		   "wrong # args: should be \"pathName setXrot pos\"",
		   TCL_STATIC );
    return TCL_ERROR;
  }

  sa_camera_rot.x = atof( argv[2] );

  strcpy( interp->result, argv[2] );
  return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int setYrot(struct Togl *togl, int argc, CONST84 char *argv[])
#else
int setYrot(struct Togl *togl, int argc, char *argv[])
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setYrot pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_rot.y = atof( argv[2] );

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int setZrot(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setZrot(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setZrot pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_rot.z = atof( argv[2] );

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

/* Get/set camera position */

#ifdef USE_TCL_CONST84_OPTION
int getXpos(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[])
#else
int getXpos(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
#endif
{
	sprintf(interp->result, "%f", sa_camera_pos.x);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int getYpos(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[])
#else
int getYpos(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
#endif
{
	sprintf(interp->result, "%f", sa_camera_pos.y);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int getZpos(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[])
#else
int getZpos(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
#endif
{
	sprintf(interp->result, "%f", sa_camera_pos.z);
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int setXpos(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setXpos(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setXpos pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_pos.x = atof( argv[2] );

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int setYpos(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setYpos(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setYpos pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_pos.y = atof( argv[2] );

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int setZpos(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setZpos(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setZpos pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_pos.z = atof( argv[2] );

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}




/* Set/get camera scale */
#ifdef USE_TCL_CONST84_OPTION
int setXscale(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setXscale(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setXscale pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_scale.x = atof( argv[2] );

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int getXscale(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[]) 
#else
int getXscale(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) 
#endif
{
	sprintf(interp->result, "%f", sa_camera_scale.x);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int setYscale(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setYscale(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setYscale pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_scale.y = atof( argv[2] );

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int getYscale(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[]) 
#else
int getYscale(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) 
#endif
{
	sprintf(interp->result, "%f", sa_camera_scale.y);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int setZscale(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setZscale(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setZscale pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_scale.z = atof( argv[2] );

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int getZscale(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[])
#else
int getZscale(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
#endif
{
	sprintf(interp->result, "%f", sa_camera_scale.z);
	return TCL_OK;
}


/* Set/get camera "rotate then translate" variable */
#ifdef USE_TCL_CONST84_OPTION
int setRotThenTrans(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setRotThenTrans(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setRotThenTrans pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_rotthentrans = atoi( argv[2] );

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int getRotThenTrans(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[])
#else
int getRotThenTrans(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
#endif
 {
	sprintf(interp->result, "%d", sa_camera_rotthentrans);
	return TCL_OK;
}



/* Set/get camera frustum variables */
void setGlFrustum() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//  glFrustum(-1, 1, -(sa_r_xsize/sa_r_ysize), (sa_r_xsize/sa_r_ysize),
	//	    10.0, 40000.0);  /* Near/far clip planes... */
	glFrustum(sa_camera_frustum_left, sa_camera_frustum_right,
			sa_camera_frustum_bottom, sa_camera_frustum_top,
			sa_camera_frustum_near, sa_camera_frustum_far);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


#ifdef USE_TCL_CONST84_OPTION
int setFrustumLeft(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setFrustumLeft(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setFrustumLeft pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_frustum_left = atof( argv[2] );
	setGlFrustum();

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int getFrustumLeft(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[]) 
#else
int getFrustumLeft(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) 
#endif
{
	sprintf(interp->result, "%d", sa_camera_frustum_left);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int setFrustumRight(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setFrustumRight(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setFrustumRight pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_frustum_right = atof( argv[2] );
	setGlFrustum();

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int getFrustumRight(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[]) 
#else
int getFrustumRight(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) 
#endif
{
	sprintf(interp->result, "%d", sa_camera_frustum_right);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int setFrustumTop(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setFrustumTop(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setFrustumTop pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_frustum_top = atof( argv[2] );
	setGlFrustum();

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int getFrustumTop(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[]) 
#else
int getFrustumTop(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) 
#endif
{
	sprintf(interp->result, "%d", sa_camera_frustum_top);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int setFrustumBottom(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setFrustumBottom(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setFrustumBottom pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_frustum_bottom = atof( argv[2] );
	setGlFrustum();

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int getFrustumBottom(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[]) 
#else
int getFrustumBottom(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) 
#endif
{
	sprintf(interp->result, "%d", sa_camera_frustum_bottom);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int setFrustumNear(struct Togl *togl, int argc, CONST84 char *argv[]) 
#else
int setFrustumNear(struct Togl *togl, int argc, char *argv[]) 
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setFrustumNear pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_frustum_near = atof( argv[2] );
	setGlFrustum();

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int getFrustumNear(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[]) 
#else
int getFrustumNear(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) 
#endif
{
	sprintf(interp->result, "%d", sa_camera_frustum_near);
	return TCL_OK;
}


#ifdef USE_TCL_CONST84_OPTION
int setFrustumFar(struct Togl *togl, int argc, CONST84 char *argv[])
#else
int setFrustumFar(struct Togl *togl, int argc, char *argv[])
#endif
{
	Tcl_Interp *interp = Togl_Interp(togl);

	if (argc != 3) {
		Tcl_SetResult( interp,
				"wrong # args: should be \"pathName setFrustumFar pos\"",
				TCL_STATIC );
		return TCL_ERROR;
	}

	sa_camera_frustum_far = atof( argv[2] );
	setGlFrustum();

	strcpy( interp->result, argv[2] );
	return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
int getFrustumFar(ClientData clientData, Tcl_Interp *interp, int argc, CONST84 char *argv[])
#else
int getFrustumFar(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
#endif
{
	sprintf(interp->result, "%d", sa_camera_frustum_far);
	return TCL_OK;
}



/* Initialisation code (called once only at startup, unlike
   open_display) */
void sa_r_init() {
  char *name = "/sasami.eden";
  char fullname[255];
  FILE *initFile;
  extern char *progname;
  extern char *libLocation;
  extern int run(short, void *, char *);
  int togl_error;

  if (togl_error = Togl_Init(interp) != TCL_OK)
    fprintf(stderr, "Togl_Init error %d\n", togl_error);

  Togl_CreateCommand("setXrot", 			setXrot);
  Togl_CreateCommand("setYrot", 			setYrot);
  Togl_CreateCommand("setZrot", 			setZrot);

  Tcl_CreateCommand(interp, "sasami_getXrot", getXrot, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getYrot", getYrot, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getZrot", getZrot, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );

  Togl_CreateCommand("setXpos", 			setXpos);
  Togl_CreateCommand("setYpos", 			setYpos);
  Togl_CreateCommand("setZpos", 			setZpos);

  Tcl_CreateCommand(interp, "sasami_getXpos", getXpos, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getYpos", getYpos, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getZpos", getZpos, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );

  Togl_CreateCommand("setXscale", 			setXscale);
  Togl_CreateCommand("setYscale", 			setYscale);
  Togl_CreateCommand("setZscale", 			setZscale);

  Tcl_CreateCommand(interp, "sasami_getXscale", getXscale, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getYscale", getYscale, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getZscale", getZscale, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );

  Togl_CreateCommand("setRotThenTrans", 	setRotThenTrans);

  Tcl_CreateCommand(interp, "sasami_getRotThenTrans", getRotThenTrans, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );

  Togl_CreateCommand("setFrustumLeft", 		setFrustumLeft);
  Togl_CreateCommand("setFrustumRight", 	setFrustumRight);
  Togl_CreateCommand("setFrustumTop", 		setFrustumTop);
  Togl_CreateCommand("setFrustumBottom", 	setFrustumBottom);
  Togl_CreateCommand("setFrustumNear", 		setFrustumNear);
  Togl_CreateCommand("setFrustumFar", 		setFrustumFar);

  Tcl_CreateCommand(interp, "sasami_getFrustumLeft", getFrustumLeft, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getFrustumRight", getFrustumRight, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getFrustumTop", getFrustumTop, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getFrustumBottom", getFrustumBottom, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getFrustumNear", getFrustumNear, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );
  Tcl_CreateCommand(interp, "sasami_getFrustumFar", getFrustumFar, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL );

  /* Set up default info */
  sa_r_xsize=640;
  sa_r_ysize=480;
  sa_r_bgcolour.r=0;
  sa_r_bgcolour.g=0;
  sa_r_bgcolour.b=0;
  sa_r_bgcolour.a=1;
  sa_r_showaxes=true;

  /* Load in sasami.eden */
  strcpy(fullname, libLocation);
  strcat(fullname, name);

  if ((initFile = fopen(fullname, "r")) == 0) {
    fprintf(stderr, "%s: can't open %s\n", progname, fullname);
    exit(1);
  }
  run(FILE_DEV, initFile, name);

}

void sa_togl_destroyfunc(struct Togl *togl) {
  sa_r_closedisplay();
}

/* !@!@ A hack: need the togl value in sa_r_update, so store it here.
   This will need rethinking if Sasami is given more than one window.
   [Ash] */
void sa_togl_createfunc(struct Togl *togl) {
  globTogl = togl;
}

/* Initialise the Sasami OpenGL renderer and get the display window
   created and open */
int sa_r_opendisplay(void) {
#define SA_TCL_COMMAND_MAXLEN 256
  char tclCommand[SA_TCL_COMMAND_MAXLEN];
  materialinfo *currentmaterial;

  if (sa_r_initialised)
    return false;

  sa_r_initialised=true;

  Togl_CreateFunc(sa_togl_createfunc);
  Togl_DisplayFunc(sa_r_render);
  Togl_DestroyFunc(sa_togl_destroyfunc);

  /* Call Tcl to create the window */
  snprintf(tclCommand, SA_TCL_COMMAND_MAXLEN,
	   "sasamiWindow %d %d", sa_r_xsize, sa_r_ysize);

  Tcl_EvalEC(interp, tclCommand);

  /* Find out what colour depth we're running at */
  /* This replaced by a sasami_viewport_bpp Eden definition in parser.c */
  /*
  Tcl_EvalEC(interp, "winfo depth .sasami");
  sa_r_bpp = atoi(interp->result);
  */

  /* Grab the driver information we need */
  glGetIntegerv(GL_MAX_LIGHTS,&sa_r_maxlights);

  DEBUGPRINT("OpenGL driver :\n", 0);
  DEBUGPRINT("Vendor     : %s\n", glGetString(GL_VENDOR));
  DEBUGPRINT("Version    : %s\n", glGetString(GL_VERSION));
  DEBUGPRINT("Renderer   : %s\n", glGetString(GL_RENDERER));
  DEBUGPRINT("Extensions : %s\n", glGetString(GL_EXTENSIONS));
  DEBUGPRINT("Max lights : %d\n", sa_r_maxlights);

  /* Set up our basic OpenGL state */

  /* Viewport */
  glViewport(0, 0, sa_r_xsize, sa_r_ysize);

  /* Projection matrix */
  setGlFrustum();

  /* Rendering state */
  glShadeModel(GL_SMOOTH);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_DITHER);
  glDisable(GL_FOG);
  glDisable(GL_LOGIC_OP);
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_ALPHA_TEST);
  glEnable(GL_POINT_SMOOTH);

  /* Texturing settings */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  /* Pixel mapping */
  glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
  glPixelTransferi(GL_RED_SCALE, 1);
  glPixelTransferi(GL_RED_BIAS, 0);
  glPixelTransferi(GL_GREEN_SCALE, 1);
  glPixelTransferi(GL_GREEN_BIAS, 0);
  glPixelTransferi(GL_BLUE_SCALE, 1);
  glPixelTransferi(GL_BLUE_BIAS, 0);
  glPixelTransferi(GL_ALPHA_SCALE, 1);
  glPixelTransferi(GL_ALPHA_BIAS, 0);

  /* Lighting */
  glEnable(GL_LIGHTING);

  /* Set up alpha-blending function */
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,0);

  /* OpenGL setup done */

  /* Set up the default camera */
  sa_camera_pos.x=0;
  sa_camera_pos.y=0;
  sa_camera_pos.z=-30; /* Ben originally had this at -8 [Ash] */
  sa_camera_rot.x=0;
  sa_camera_rot.y=0;
  sa_camera_rot.z=0;
  sa_camera_scale.x=10;
  sa_camera_scale.y=10;
  sa_camera_scale.z=10;
  sa_camera_rotthentrans = 0;

  /* Flag all loaded materials as needing re-loading for this new
     context */
  currentmaterial=sa_getfirstmaterial();

  while (currentmaterial != NULL) {
    if (currentmaterial->texturefile!=NULL)
      currentmaterial->textureneedsloading=1;
    currentmaterial=currentmaterial->next;
  }
}

/* Close down the Sasami renderer */
void sa_r_closedisplay(void) {
  if (sa_r_initialised) {
    sa_r_initialised = false;

    /* Don't think we need to clean up any GL stuff [Ash] */

    Tcl_EvalEC(interp, "sasamiWindowClose");
  }
}

/* Resizes the viewport (safely!) */
void sa_r_resizeviewport(int x, int y) {
  bool wasinitialised;

  wasinitialised=sa_r_initialised;
  /* Make sure the viewport is closed before resizing */
  if (sa_r_initialised) sa_r_closedisplay();

  sa_r_xsize=x;
  sa_r_ysize=y;

  /* Reopen the viewport (if it was open before) */
  if (wasinitialised) sa_r_opendisplay();
}

/* This sets up the camera position onto the modelview matrix */
void sa_r_setupcamera(void)
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  //  glScalef(10, 10, 10);
  glScalef(sa_camera_scale.x, sa_camera_scale.y, sa_camera_scale.z);

  if (!sa_camera_rotthentrans)
    glTranslatef(sa_camera_pos.x, sa_camera_pos.y, sa_camera_pos.z);

  glRotatef(sa_camera_rot.x, -1, 0, 0);
  glRotatef(sa_camera_rot.y, 0, -1, 0);
  glRotatef(sa_camera_rot.z, 0, 0, -1);

  if (sa_camera_rotthentrans)
    glTranslatef(sa_camera_pos.x, sa_camera_pos.y, sa_camera_pos.z);
}

/* This sets up the world lighting */
void sa_r_setuplighting(void)
{
  lightinfo		*light;
  int				i;
  GLfloat			lightparams[4];

  for (i=0;i<sa_r_maxlights;i++)
    glDisable(GL_LIGHT0+i);

  light=sa_getfirstlight();

  if (light==NULL) {
    /* Set up the default lighting */

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0,GL_POSITION,sa_def_light_pos1);
    glLightfv(GL_LIGHT0,GL_AMBIENT,sa_def_light_ambient);
    glLightfv(GL_LIGHT0,GL_SPECULAR,sa_def_light_specular);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,sa_def_light_specular);
    glLightf(GL_LIGHT0,GL_CONSTANT_ATTENUATION,1);

    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1,GL_POSITION,sa_def_light_pos2);
    glLightfv(GL_LIGHT1,GL_AMBIENT,sa_def_light_ambient);
    glLightfv(GL_LIGHT1,GL_SPECULAR,sa_def_light_specular);
    glLightfv(GL_LIGHT1,GL_DIFFUSE,sa_def_light_specular);
    glLightf(GL_LIGHT1,GL_CONSTANT_ATTENUATION,1);
  } else {
#ifdef SASAMI_SHOW_LIGHTS
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glPointSize(5);
#endif

    /* Use first <x> enabled lights */

    i=0;
    while ((light != NULL) && (i<sa_r_maxlights)) {
      if (light->enabled != 0) {
	glPushMatrix();

	glTranslatef(light->pos.x,light->pos.y,light->pos.z);

	if (light->directional==1) {
	  lightparams[0]=light->pos.x;
	  lightparams[1]=light->pos.y;
	  lightparams[2]=light->pos.z;
	  lightparams[3]=0;
	} else {
	  lightparams[0]=0;
	  lightparams[1]=0;
	  lightparams[2]=0;
	  lightparams[3]=1;
	}

	glLightfv(GL_LIGHT0+i,GL_POSITION,lightparams);

	lightparams[0]=light->ambient.r;
	lightparams[1]=light->ambient.g;
	lightparams[2]=light->ambient.b;
	lightparams[3]=light->ambient.a;
	glLightfv(GL_LIGHT0+i,GL_AMBIENT,lightparams);

	lightparams[0]=light->diffuse.r;
	lightparams[1]=light->diffuse.g;
	lightparams[2]=light->diffuse.b;
	lightparams[3]=light->diffuse.a;
	glLightfv(GL_LIGHT0+i,GL_DIFFUSE,lightparams);

	lightparams[0]=light->specular.r;
	lightparams[1]=light->specular.g;
	lightparams[2]=light->specular.b;
	lightparams[3]=light->specular.a;
	glLightfv(GL_LIGHT0+i,GL_SPECULAR,lightparams);

	if (light->attenuation!=0) {
	  glLightf(GL_LIGHT0+i,GL_CONSTANT_ATTENUATION,1);
	  glLightf(GL_LIGHT0+i,GL_LINEAR_ATTENUATION,light->attenuation);
	} else {
	  glLightf(GL_LIGHT0+i,GL_LINEAR_ATTENUATION,0);
	  glLightf(GL_LIGHT0+i,GL_CONSTANT_ATTENUATION,1);
	}

#ifdef SASAMI_SHOW_LIGHTS
	glBegin(GL_POINTS); /* Inefficient, but necessary as you
			       can't change light parameters in a
			       glBegin/glEnd block! */
	glColor4f(light->diffuse.r,light->diffuse.g,light->diffuse.b,
		  light->diffuse.a);
	glVertex3f(	0,
			0,
			0);
	glEnd();
#endif

	glEnable(GL_LIGHT0+i);
	i++;
	glPopMatrix();
      }

      light=light->next;
    }

#ifdef SASAMI_SHOW_LIGHTS
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
#endif

  }
}

/* Render axes indicators (unit length arrows along each axis) */
void sa_r_render_axes(void)
{
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);

  glBegin(GL_LINES);

  // X axis (red)

  glColor4f(1,0,0,1);
  glVertex3f(0,0,0); glVertex3f(1,0,0);
  glVertex3f(1,0,0); glVertex3f(0.8,0,0.1);
  glVertex3f(1,0,0); glVertex3f(0.8,0,-0.1);

  // Y axis (green)

  glColor4f(0,1,0,1);
  glVertex3f(0,0,0); glVertex3f(0,1,0);
  glVertex3f(0,1,0); glVertex3f(0.1,0.8,0);
  glVertex3f(0,1,0); glVertex3f(-0.1,0.8,0);

  // Z axis (blue)

  glColor4f(0,0,1,1);
  glVertex3f(0,0,0); glVertex3f(0,0,1);
  glVertex3f(0,0,1); glVertex3f(0.1,0,0.8);
  glVertex3f(0,0,1); glVertex3f(-0.1,0,0.8);

  glEnd();
}

/* Generates a face normal using 3 co-ordinates */
coord sa_get_face_normal(coord a,coord b,coord c)
{
  coord r;
  double l;

  a.x=a.x-c.x;
  a.y=a.y-c.y;
  a.z=a.z-c.z;

  b.x=b.x-c.x;
  b.y=b.y-c.y;
  b.z=b.z-c.z;

  r.x=(a.y*b.z)-(a.z*b.y);
  r.y=(a.z*b.x)-(a.x*b.z);
  r.z=(a.x*b.y)-(a.y*b.x);

  /* Normalisation is not strictly necessary (as GL_NORMALIZE is enabled),
     but avoids potential problems */

  l=sqrt((r.x*r.x)+(r.y*r.y)+(r.z*r.z));
  r.x=r.x/l;
  r.y=r.y/l;
  r.z=r.z/l;

  return r;
}

/* Generates a face normal from a vertex UID list.  The UID list must
   have at least 3 vertices to generate a normal, otherwise a normal
   of (0,0,0) is returned */
coord sa_get_face_normal_from_uid_list(uidlist *first)
{
  int i;
  coord c[3];
  uidlist *currentid;
  vertexinfo *v;

  /* Run through the vertex UID list extracting the coordinates of the
     first three vertices */

  currentid=first;
  i=0;
  while ((currentid!=NULL) && (i!=3)) {
    v=sa_findvertex(currentid->uid);
    c[i]=v->pos;
    currentid=currentid->next;
    i++;
  }

  if (i==3) {
    /* We got 3 vertices - calculate normal */
    return sa_get_face_normal(c[0],c[1],c[2]);
  } else {
    /* We didn't find enough vertices - return (0,0,0); */
    c[0].x=0;
    c[0].y=0;
    c[0].z=0;
    return c[0];
  }
}

/* Renders all of the polygons for a given polygon UID list */
void sa_r_render_object_polys(uidlist *polylist)
{
  polyinfo	*currentpoly;
  uidlist	*currentpolyid;
  uidlist	*currentvertexid;
  uidlist	*currenttexcoordid;
  vertexinfo	*v;
  vertexinfo	*tv;
  coord		normal;
  GLfloat	mat_amb[4]		= {0.01,0.01,0.01,1.00};
  GLfloat	mat_diff[4]		= {0.65,0.05,0.20,0.60};
  GLfloat	mat_spec[4]		= {0.50,0.50,0.50,1.00};
  GLfloat	mat_shine		= 20;
  materialinfo	*polymat;

  currentpolyid=polylist;

  glEnable(GL_LIGHTING);

  while (currentpolyid!=NULL) {
    currentpoly = sa_findpoly(currentpolyid->uid);

    /* Get the polygon material */
    polymat = sa_findmaterial(currentpoly->material);

    /* Check to see if there is a valid texture attached */
    if (polymat!=NULL) {
      /* Load texture if necessary.  This is done here to ensure that
	 OpenGL is initialised when textures are loaded! */
      if (polymat->textureneedsloading > 0) {
	polymat->texture = sa_getandbindnewtexture();
	sa_loadtexture(polymat->texturefile);
	polymat->textureneedsloading = 0;
      }
      if (polymat->texture!=0) {
	glBindTexture(GL_TEXTURE_2D,polymat->texture);
	glEnable(GL_TEXTURE_2D);
      } else {
	glDisable(GL_TEXTURE_2D);
      }
    } else {
      glDisable(GL_TEXTURE_2D);
    }

    /* Per-polygon lighting calculations */
    glEnable(GL_CULL_FACE);
    glDepthMask(1); /* Allow Z-Buffer writes */

    /* Get the face normal */
    normal=sa_get_face_normal_from_uid_list(currentpoly->geometry);

    if
#ifdef SASAMI_ANIME
    (((normal.x==0) && (normal.y==0) && (normal.z==0)) | (true))
#else
    (((normal.x==0) && (normal.y==0) && (normal.z==0)))
#endif
      {
      /* We couldn't calculate a normal, so don't light this polygon,
	 and just use the basic colour */
      glDisable(GL_LIGHTING);
      glColor4f(	currentpoly->colour.r,
			currentpoly->colour.g,
			currentpoly->colour.b,
			currentpoly->colour.a);
      if (currentpoly->colour.a<1) {
	glDisable(GL_CULL_FACE); /* Transparent stuff is two-sided */
	glDepthMask(0); /* Transparent stuff doesn't affect Z buffer */
      }
    } else {
      if (polymat==NULL) {
	/* This poly has no material - set up default lighting
	   parameters from the colour given */
	/* Set up material parameters for this surface */
	mat_amb[0]=0.1;
	mat_amb[1]=0.1;
	mat_amb[2]=0.1;
	mat_amb[3]=1;
	mat_diff[0]=currentpoly->colour.r;
	mat_diff[1]=currentpoly->colour.g;
	mat_diff[2]=currentpoly->colour.b;
	mat_diff[3]=currentpoly->colour.a;
	mat_spec[0]=0.7;
	mat_spec[1]=0.7;
	mat_spec[2]=0.7;
	mat_spec[3]=1;
	if (currentpoly->colour.a<1) {
	  glDisable(GL_CULL_FACE); /* Transparent stuff is two-sided */
	  glDepthMask(0); /* Transparent stuff doesn't affect Z buffer */
	}
      } else {
	/* This poly has a material - use it! */
	mat_amb[0]=polymat->ambient.r;
	mat_amb[1]=polymat->ambient.g;
	mat_amb[2]=polymat->ambient.b;
	mat_amb[3]=polymat->ambient.a;
	mat_diff[0]=polymat->diffuse.r;
	mat_diff[1]=polymat->diffuse.g;
	mat_diff[2]=polymat->diffuse.b;
	mat_diff[3]=polymat->diffuse.a;
	mat_spec[0]=polymat->specular.r;
	mat_spec[1]=polymat->specular.g;
	mat_spec[2]=polymat->specular.b;
	mat_spec[3]=polymat->specular.a;
	if ((polymat->ambient.a<1) ||
	    (polymat->diffuse.a<1) ||
	    (polymat->specular.a<1)) {
	  glDisable(GL_CULL_FACE); /* Transparent stuff is two-sided */
	  glDepthMask(0); /* Transparent stuff doesn't affect Z buffer */
	}
      }

      /* Pass the material data to OpenGL */
      glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,mat_amb);
      glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,mat_diff);
      glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mat_spec);
      glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,mat_shine);
      glEnable(GL_LIGHTING);
    }

    /* Run through each vertex of the polygon's vertex UID list and
       feed it to OpenGL */
    currentvertexid=currentpoly->geometry;
    currenttexcoordid=currentpoly->texcoords;

    glBegin(GL_POLYGON);
    while (currentvertexid!=NULL) {
      v=sa_findvertex(currentvertexid->uid);

      /* It seems that v==NULL when eager() is used in a tight loop.
         Ignoring the vertex like this stops tkeden crashing when
         dereferencing v, but the resulting 3D geometry is corrupted
         :( [Ash] */
      if (v != NULL) {
	glNormal3f(normal.x,normal.y,normal.z);

	if (currenttexcoordid!=NULL) {
	  /* We've got a texture co-ordinate to use */
	  tv=sa_findvertex(currenttexcoordid->uid);
	  
	  glTexCoord2f(tv->pos.x,tv->pos.y);
	  currenttexcoordid=currenttexcoordid->next;
	} else {
	  /* Default to generating texture co-ordinates from X,Y */
#ifndef SASAMI_ANIME
	  glTexCoord2f(v->pos.x,v->pos.y);
#else
	  /* SASAMI_ANIME "enables the experimental renderer" apparently
	     [Ash] */
	  glTexCoord2f(0.5+(normal.x/2),0.5+(normal.y/2));
#endif
	}
	
	glVertex3f(	v->pos.x,
			v->pos.y,
			v->pos.z);
      }

      currentvertexid=currentvertexid->next;
    }

    glEnd();

    currentpolyid=currentpolyid->next;
  }

  glDepthMask(1);
  glDisable(GL_TEXTURE_2D);
}


/* Renders all the objects in the current scene */
void sa_r_render_objects(void)
{
  objectinfo	*current;

  current=sa_getfirstobject();

  while (current!=NULL) {
    if (current->visible!=0) {
      /* Set up this object's position, rotation and scale on the
	 modelview matrix */

      glPushMatrix();

      glTranslatef(	current->pos.x,
			current->pos.y,
			current->pos.z);

      /* Rotation precidence is X->Y->Z, as set here... */

      glRotatef(current->rot.x,1,0,0);
      glRotatef(current->rot.y,0,1,0); /* 0,cos(rad(current->rot.x)),
					  sin(rad(current->rot.x)) */
      glRotatef(current->rot.z,0,0,1);

      glScalef(		current->scale.x,
			current->scale.y,
			current->scale.z);

      /* Render all of the polys for this object */
      sa_r_render_object_polys(current->polys);

      /* Restore the previous modelview matrix */
      glPopMatrix();
    }
    current=current->next;
  }
}


/* The actual renderer function */
void sa_r_render(struct Togl *togl) {
#ifdef DEBUG
  struct timezone tzone;
  struct timeval currentRenderTime;
  long timeSinceLast;
#endif /* DEBUG */

  globTogl = togl;		/* hack [Ash] */

  if (!sa_r_initialised)
    return;

#ifdef DEBUG
  if (Debug & 8) {
    gettimeofday(&currentRenderTime, &tzone);

    fprintf(stderr, "sa_r_render: ");

    if (lastRenderTimeValid) {
      timeSinceLast = (currentRenderTime.tv_sec - lastRenderTime.tv_sec)
			* 1000000
			+ (currentRenderTime.tv_usec - lastRenderTime.tv_usec);
      fprintf(stderr, "%d usec since last render (= effectively %Lf fps)\n",
		timeSinceLast,
	        (long double)1000000/(long double)timeSinceLast);
      lastRenderTime = currentRenderTime;  
    } else {
      fprintf(stderr, "first render -- no timings available\n");
      lastRenderTime = currentRenderTime;
      lastRenderTimeValid = true;
    }
  }
#endif /* DEBUG */

  glClearColor(sa_r_bgcolour.r, sa_r_bgcolour.g, sa_r_bgcolour.b,
	       sa_r_bgcolour.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  sa_r_setupcamera();

  sa_r_setuplighting();

  if (sa_r_showaxes)
    sa_r_render_axes();

  sa_r_render_objects();

  Togl_SwapBuffers(togl);
}

/* Called whenever something has been changed by Eden (from
   functions.c, mainly) to get the renderer to rerender the scene */
void sa_r_update(void) {
  if (globTogl) {
    /* Ben called sa_r_render(globTogl) here, but the performance was
       pretty poor.  This call ensures that many updates made by Eden
       in one RunSet get combined into one redraw.  [Ash] */
    Togl_PostRedisplay(globTogl);
  }
}

/* Set the axes display state */
void sa_r_setshowaxes(int s) {
  if (s==1) {
    sa_r_showaxes=true;
  } else {
    sa_r_showaxes=false;
  }

  sa_r_update();
}

/* Creates a new texture object, binds it and sets up default parameters */
GLint sa_getandbindnewtexture() {
  GLint a;

  glGenTextures(1, &a);

  if (a==0) {
    errorf("Sasami: Unable to generate texture binding!\n");
  }

  glBindTexture(GL_TEXTURE_2D,a);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  return a;
}

/* Loads a texture from the specified file */
void sa_loadtexture(char *f) {
  char o[256];
  pngInfo info;

  DEBUGPRINT("Loading texture file '%s'\n", f);

  /* pngSetStandardOrientation(1); */
  if (pngLoad(f, PNG_NOMIPMAP, PNG_ALPHA, &info) != 1) {
    errorf("Sasami: unable to load texture file `%s`", f);
  } else {
#ifdef DEBUG
    if (Debug&8) {
      fprintf(stderr,
	      "Successfully loaded %s: size=%i,%i depth=%i alpha=%i\n",
	      f, info.Width, info.Height, info.Depth, info.Alpha);
    }
#endif
  }
}

#endif /* WANT_SASAMI */
