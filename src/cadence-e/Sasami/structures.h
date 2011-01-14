// --------------------------------------------------------------------------
// structures.h - This contains definations for the Sasami structures and
//				  functions to operate on them
// --------------------------------------------------------------------------

#ifndef __sa_structures__

#define __sa_structures__

#include <limits.h>

#include "debug.h"
#include "render.h"


// --------------------------------------------------------------------------
// Coordinate X,Y,Z triple
// --------------------------------------------------------------------------

typedef struct coordptr
{
	double					x,y,z;
} coord;

// --------------------------------------------------------------------------
// RGBA colour
// --------------------------------------------------------------------------

typedef struct colourinfoptr
{
	double					r,g,b,a;
} colourinfo;

// --------------------------------------------------------------------------
// Vertex
// --------------------------------------------------------------------------

typedef struct materialinfoptr
{
	int						n;
	colourinfo				ambient;
	colourinfo				diffuse;
	colourinfo				specular;
	char					*texturefile;
	GLint					texture;
	int						textureneedsloading;
	struct materialinfoptr	*next;
	struct materialinfoptr	*prev;
} materialinfo;

// --------------------------------------------------------------------------
// UID List
// --------------------------------------------------------------------------

typedef struct _uidlist
{
	int						n;		// UID of this UID (if that makes sense)
									// i.e. a unique reference number for this
									// entry in this UID list (may be either
									// a "true" GUID or just a convienient
									// number)
	int						uid;	// Actual list entry data
	struct _uidlist			*next;
	struct _uidlist			*prev;
} uidlist;

// --------------------------------------------------------------------------
// UID map divisions
// --------------------------------------------------------------------------

/* Partition the available UID space (INT_MAX, approx = 2^31 = 2147483647
 * if C int is a 32 bit signed value) into divisions for vertices, polygons
 * and other types.  See explanatory comments in structures.c.
 * 
 * In Charlie's planimeterCare2005, the distribution of the different types
 * is: 15000 UIDs total, 3370 vertices, 1579 polygons -- ie approx 10000
 * "general" UIDs.  Therefore distribution in that model is
 * 67% "general", 22% vertices, 11% polygons.  Partitioning the UID space
 * according to that distribution for the time being, ie divisions at
 * 1400000000 and 1900000000.
 *
 * If an instance of the vertexinfo struct takes 28 bytes, then the
 * 500000000 instances possible with this partitioning would require
 * 13GB of memory to hold the actual data.  So I don't think the use of
 * an int to store the divided UID space will be a problem in the short
 * term :).
 *
 * Only changing vertex and polygon stores into arrays as measurements
 * (using Apple's Shark utility) on planimeterCare2005 showed that searching
 * for vertices was taking approx 80% of EDEN CPU time, and for polygons
 * approx 20%.  The next heaviest activity was GL drawing, so leaving other
 * structures: no point optimising unless this would demonstrably give an
 * improvement.
 *
 * Changing vertices and polygons from linked lists to arrays has sped
 * rotation of Sasami window in planimeterCare2005 up by a factor of 10:
 * from 3 fps to 30 fps (measured on my PowerBook 1.67GHz).  The speed-up
 * should increase with the size of the model.  [Ash] */
#define GENUIDEND			(VERTEXINFOUIDSTART-1)

#define VERTEXINFOUIDSTART	1400000000
#define VERTEXINFOUIDEND	(POLYINFOUIDSTART-1)

#define POLYINFOUIDSTART	1900000000
#define POLYINFOUIDEND		INT_MAX

// --------------------------------------------------------------------------
// Vertex
// --------------------------------------------------------------------------

typedef struct _vertexinfo
{
	int						vuid;
	coord					pos;
} vertexinfo;

// --------------------------------------------------------------------------
// Polygon
// --------------------------------------------------------------------------

typedef struct _polyinfo
{
	int						puid;
	uidlist					*geometry; // List of vertices
	uidlist					*texcoords;
	colourinfo				colour;
	int						material;
} polyinfo;


// --------------------------------------------------------------------------
// Object
// --------------------------------------------------------------------------

typedef struct objectinfoptr
{
	char*					name;
	coord					pos;
	coord					rot;
	coord					scale;
	int					visible;
	uidlist					*polys;		// List of polygons
	int					material;	// Object-wide material
	int					primitivePolyCount;
	struct objectinfoptr	*next;
	struct objectinfoptr	*prev;
} objectinfo;

// --------------------------------------------------------------------------
// Light
// --------------------------------------------------------------------------

typedef struct lightinfoptr
{
	int						n;
	coord					pos;
	colourinfo				ambient;
	colourinfo				diffuse;
	colourinfo				specular;
	int						enabled;
	double					attenuation;
	int						directional;
	struct lightinfoptr		*next;
	struct lightinfoptr		*prev;
} lightinfo;

// --------------------------------------------------------------------------
// General-purpose list
// --------------------------------------------------------------------------

typedef struct _list
{
	int						n;		// UID of this UID (if that makes sense)
									// i.e. a unique reference number for this
									// entry in this UID list (may be either
									// a "true" GUID or just a convienient
									// number)
	int						idata;  // Integer value data
	void					*data;  // General-purpose data pointer
	struct _list			*next;
	struct _list			*prev;
} list;

// --------------------------------------------------------------------------
// Functions
// --------------------------------------------------------------------------

// Vertex-related

vertexinfo	*sa_addvertex(int n,double x,double y,double z);
vertexinfo	*sa_findvertex(int n);
vertexinfo *sa_getvertex(int n);
void		sa_dumpvertices(void);
vertexinfo	*sa_getfirstvertex(void);
void		sa_deletevertex(int n);

// UID list related

uidlist		*sa_adduidlist(uidlist **first,int n);
uidlist		*sa_adduidlistpos(uidlist **first,int n,int p);
uidlist		*sa_finduidlist(uidlist **first,int n);
uidlist		*sa_finduidlistn(uidlist **first,int n);
uidlist		*sa_getuidlist(uidlist **first,int n);

// Polygon-related

polyinfo	*sa_addpoly(int n);
polyinfo	*sa_findpoly(int n);
polyinfo	*sa_getpoly(int n);
void		sa_dumppolys(void);
polyinfo	*sa_getfirstpoly(void);
void		sa_deletepoly(int n);

// Object-related

objectinfo	*sa_addobject(char* name);
objectinfo	*sa_findobject(char* name);
objectinfo	*sa_getobject(char* name);
void		sa_dumpobjects(void);
objectinfo	*sa_getfirstobject(void);
objectinfo	*sa_truncprimitive(char* name);
void		sa_deleteobject(char* name);

// Material-related

materialinfo	*sa_addmaterial(int n);
materialinfo	*sa_findmaterial(int n);
materialinfo	*sa_getmaterial(int n);
void			sa_dumpmaterials(void);
materialinfo	*sa_getfirstmaterial(void);

// Light-related

lightinfo		*sa_addlight(int n);
lightinfo		*sa_findlight(int n);
lightinfo		*sa_getlight(int n);
void			sa_dumplights(void);
lightinfo		*sa_getfirstlight(void);

// List-related

list		*sa_addlist(list **first,int n);
list		*sa_findlist(list **first,int n);
list		*sa_getlist(list **first,int n);

// UID-related
int sa_getUID(void);
int sa_getvertexUID(void);

#endif
