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



// --------------------------------------------------------------------------
// structures.c - This contains various global Sasami structure functions
// --------------------------------------------------------------------------

#include "../../../config.h"

#ifdef WANT_SASAMI

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>	// [Richard] : For strlen() etc

#include "structures.h"
#include "debug.h"
#include "../Eden/eden.h" /* for Debug */


#include "../Eden/emalloc.h"


// --------------------------------------------------------------------------
// Global stuff
// --------------------------------------------------------------------------

polyinfo		*firstpoly		= NULL;
objectinfo		*firstobject	= NULL;
materialinfo	*firstmaterial	= NULL;
lightinfo		*firstlight		= NULL;

#define INITIALVERTEXINFOSTORESIZE 8
vertexinfo		*vertexinfostore	= NULL;
int				vertexinfostoresize	= 0;

#define INITIALPOLYINFOSTORESIZE 8
polyinfo		*polyinfostore		= NULL;
int				polyinfostoresize	= 0;

void sa_clear_vertexinfostore(int);
void sa_clear_polyinfostore(int);


// --------------------------------------------------------------------------
// Initialisation
// --------------------------------------------------------------------------
void sa_structures_init(void)
{
	vertexinfostore = emalloc(sizeof(vertexinfo) * INITIALVERTEXINFOSTORESIZE);
	vertexinfostoresize	= INITIALVERTEXINFOSTORESIZE;
	sa_clear_vertexinfostore(0);

	polyinfostore = emalloc(sizeof(polyinfo) * INITIALPOLYINFOSTORESIZE);
	polyinfostoresize = INITIALPOLYINFOSTORESIZE;
	sa_clear_polyinfostore(0);
}


// --------------------------------------------------------------------------
// Unique IDentifier (UID)-related functions
// --------------------------------------------------------------------------

/*
 * UIDs are used to identify all kinds of Sasami data, including materials,
 * objects, vertices, polygons, lights.  In the initial implementation of
 * Sasami, UIDs were assigned in request sequence.  The requests mix the
 * different kinds of data, and so the UIDs assigned to vertices (for example)
 * did not make a contiguous sequence of integers.  This was not a problem
 * in the initial Sasami implementation, which stored data in linked lists
 * which were searched for the required UID.  The linked lists were highly
 * inefficient, however, and I (Ashley) am replacing some of them with array
 * data structures instead.  In this case, it is important that the UIDs are
 * in contiguous sequence, to avoid wasted memory for the UIDs that are missing
 * from the sequence.  (I could try and implement a sparse array instead, but
 * it seems simpler to change the assignment of UIDs).
 *
 * Therefore UIDs are now assigned using the following map:
 *
 * general (not one of the below types): 1 -> GENUIDEND
 * vertices: VERTEXINFOUIDSTART -> VERTEXINFOUIDEND
 * polys: POLYINFOUIDSTART -> POLYINFOUIDEND
 *
 * [Ash]
 */

static int sa_uidcounter = 1;
static int sa_vertexuidcounter = VERTEXINFOUIDSTART;
static int sa_polyuidcounter = POLYINFOUIDSTART;


// --------------------------------------------------------------------------
// This returns a Sasami UID (unique ID) number which can then be used to
// reference an object.  NB Not for vertex/vertices or poly/polygons!
// --------------------------------------------------------------------------
int sa_getUID(void)
{
	if (sa_uidcounter < GENUIDEND) {
		return sa_uidcounter++;
	} else {
		errorf("sa_getUID: exceeded limit for number of identifiers for Sasami general data, presently %d -- please ask for GENUIDEND to be increased", GENUIDEND);
	}
}


// --------------------------------------------------------------------------
// This returns a Sasami UID (unique ID) number which can be used to
// reference vertex/vertices.
// --------------------------------------------------------------------------
int sa_getvertexUID(void)
{
	if (sa_vertexuidcounter < VERTEXINFOUIDEND) {
		return sa_vertexuidcounter++;
	} else {
		errorf("sa_getvertexUID: exceeded limit for number of identifiers for Sasami vertex data, presently %d -- please ask for VERTEXINFOUIDSTART to be modified", VERTEXINFOUIDEND-VERTEXINFOUIDSTART);
	}
}


// --------------------------------------------------------------------------
// This returns a Sasami UID (unique ID) number which can be used to
// reference poly/polygons.
// --------------------------------------------------------------------------
int sa_getpolyUID(void)
{
	if (sa_polyuidcounter < POLYINFOUIDEND) {
		return sa_polyuidcounter++;
	} else {
		errorf("sa_getpolyUID: exceeded limit for number of identifiers for Sasami polygon data, presently %d -- please ask for POLYINFOUIDSTART to be modified", POLYINFOUIDEND-POLYINFOUIDSTART);
	}
}


// --------------------------------------------------------------------------
// Vertex-related functions
// --------------------------------------------------------------------------


// --------------------------------------------------------------------------
// vertexinfostore is indexed from 0, whereas the vertexUIDs passed in to the
// functions below are indexed VERTEXINFOUIDSTART -> VERTEXINFOUIDEND
// (see comments above).  This function gets the index from the vUID.
// --------------------------------------------------------------------------
int sa_index_from_vUID(int vUID)
{
	assert(vUID >= VERTEXINFOUIDSTART);
	assert(vUID <= VERTEXINFOUIDEND);
	return vUID - VERTEXINFOUIDSTART;
}


// --------------------------------------------------------------------------
// With the absence of a standard recalloc function, want to make sure that
// the vertexinfostore is initialised with appropriate zeros so that we can
// detect missing vertex information
// --------------------------------------------------------------------------
void sa_clear_vertexinfostore(int startindex)
{
	int index;

	for (index = startindex; index < vertexinfostoresize; index++) {
		/* just clearing vuid is sufficient for detection by sa_findvertex */
		(vertexinfostore[index]).vuid = 0;
	}
}

// --------------------------------------------------------------------------
// Add a new vertex, returning a pointer to the new vertex
// --------------------------------------------------------------------------
vertexinfo *sa_addvertex(int vuid,double x,double y,double z)
{
	int index;
	
	index = sa_index_from_vUID(vuid);
	
	if (vertexinfostoresize <= index) {
		int oldsize = vertexinfostoresize;
		
		while (vertexinfostoresize <= index) {
			vertexinfostoresize *= 2;
		}
		
#ifdef DEBUG
		if (Debug&8) {
			fprintf(stderr,
						"Resizing vertexinfostore from %d items to %d items\n",
						oldsize, vertexinfostoresize);
		}
#endif

		vertexinfostore = erealloc(vertexinfostore,
									sizeof(vertexinfo) * vertexinfostoresize);
		sa_clear_vertexinfostore(oldsize+1);
	}

	(vertexinfostore[index]).vuid=vuid;
	(vertexinfostore[index]).pos.x=x;
	(vertexinfostore[index]).pos.y=y;
	(vertexinfostore[index]).pos.z=z;

	return &(vertexinfostore[index]);
}


// --------------------------------------------------------------------------
// Finds vertex number <n> in the vertex list
// --------------------------------------------------------------------------
vertexinfo *sa_findvertex(int vuid)
{
	int index, foundvuid;
	
	index = sa_index_from_vUID(vuid);
	
	foundvuid = vertexinfostore[index].vuid;

	if (foundvuid == 0) {
		// No vertex found
		return NULL;
	} else {
		// Found it -- sanity check, then return pointer
		assert(foundvuid == vuid);
		return &(vertexinfostore[index]);
	}

}


// --------------------------------------------------------------------------
// Finds vertex number <n> in the vertex list, or creates it if it doesn't
// exist
// --------------------------------------------------------------------------
vertexinfo *sa_getvertex(int vuid)
{
	vertexinfo *v;

	v=sa_findvertex(vuid);

	if (v==NULL)
	{
		// Create it
		v=sa_addvertex(vuid,0,0,0);
	}

	return v;
}

// --------------------------------------------------------------------------
// Finds vertex number <vuid> in the vertex list and deletes its entry
// --------------------------------------------------------------------------

void sa_deletevertex(int vuid)
{
	vertexinfo *v;

	v=sa_findvertex(vuid);
	if(v==NULL)
		return;
	
	(vertexinfostore[sa_index_from_vUID(vuid)]).vuid = 0;
}

// --------------------------------------------------------------------------
// Dumps the contents of the vertex list to STDOUT (for debugging)
// --------------------------------------------------------------------------
void sa_dumpvertices(void)
{
	int index;
	
	printf("--- Vertex list ---\n");
	for (index = 0; index <= vertexinfostoresize; index++) {
		if (vertexinfostore[index].vuid != 0) {
			printf("Vertex %d : (%g,%g,%g)\n",
				vertexinfostore[index].vuid,
				vertexinfostore[index].pos.x,
				vertexinfostore[index].pos.y,
				vertexinfostore[index].pos.z);
		}
	}
	printf("-------------------\n");

}



// --------------------------------------------------------------------------
// UID list-related functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Add a new UID to end of list, returning a pointer to this UID
// --------------------------------------------------------------------------

uidlist *sa_adduidlist(uidlist **first,int n)
{
	uidlist *newuidlist;
	uidlist *currentuidlist;

	newuidlist=malloc(sizeof(uidlist));

	newuidlist->n=n;
	newuidlist->uid=0; // No UID in list initially

	newuidlist->next=NULL;

	if ((*first) == NULL) {
		newuidlist->prev=NULL;

		(*first)=newuidlist;
	}
	else {
		currentuidlist=(*first);
		while ((currentuidlist->next)!=NULL)
			currentuidlist=currentuidlist->next;

		newuidlist->prev=currentuidlist;
		currentuidlist->next=newuidlist;
	}

	return newuidlist;
}

// --------------------------------------------------------------------------
// Add a new UID to list at position <p>, returning a pointer to this UID
// --------------------------------------------------------------------------

uidlist *sa_adduidlistpos(uidlist **first,int n,int p)
{
	uidlist *newuidlist;
	uidlist *currentuidlist;

	newuidlist=malloc(sizeof(uidlist));

	newuidlist->n=n;
	newuidlist->uid=0; // No UID in list initially

	newuidlist->next=NULL;

	if ((*first) == NULL) {
		newuidlist->prev=NULL;

		(*first)=newuidlist;
	}
	else {
		currentuidlist=(*first);
		while ((currentuidlist->next)!=NULL && p > 1) {
			currentuidlist=currentuidlist->next;
			p--;
		}
		
		if(p == 0) {			//before current entry (at the start)
			newuidlist->prev=NULL;
			newuidlist->next=currentuidlist;
			currentuidlist->prev=newuidlist;
			(*first)=newuidlist;
		}
		else {				//after current entry
			newuidlist->prev=currentuidlist;
			newuidlist->next=currentuidlist->next;
			currentuidlist->next=newuidlist;
			if(currentuidlist->next!=NULL)
				currentuidlist->next->prev=newuidlist;
		}
		
		

	}

	return newuidlist;
}

// --------------------------------------------------------------------------
// Finds UID list entry number <n>
// --------------------------------------------------------------------------

uidlist *sa_finduidlist(uidlist **first,int n)
{
	uidlist *currentuidlist;

	if ((*first)==NULL)
	{
		// No UIDs in list
		return NULL;
	}

	currentuidlist=(*first);

	while (((currentuidlist->n)!=n) && ((currentuidlist->next)!=NULL))
		currentuidlist=currentuidlist->next;

	if ((currentuidlist->n)==n)
	{
		// index found
		return currentuidlist;
	}
	else
	{
		// No index found
		return NULL;
	}
}

// --------------------------------------------------------------------------
// Finds <n>th entry
// --------------------------------------------------------------------------

uidlist *sa_finduidlistn(uidlist **first,int n)
{
	uidlist *currentuidlist;

	if ((*first)==NULL)
	{
		// No UIDs in list
		return NULL;
	}

	currentuidlist=(*first);

	while ((currentuidlist->next!=NULL) && (n-- > 0))
		currentuidlist=currentuidlist->next;

	if (n == -1)
	{
		// UID found
		return currentuidlist;
	}
	else
	{
		// No UID found
		return NULL;
	}
}

// --------------------------------------------------------------------------
// Finds index number <n> in the UID list, or creates it if it doesn't exist
// --------------------------------------------------------------------------

uidlist *sa_getuidlist(uidlist **first,int n)
{
	uidlist *r;

	r=sa_finduidlist(first,n);

	if (r==NULL)
	{
		// Create it
		r=sa_adduidlist(first,n);
	}

	return r;
}



// --------------------------------------------------------------------------
// Polygon-related functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// polyinfostore is indexed from 0, whereas the polyUIDs passed in to the
// functions below are indexed from POLYINFOUIDSTART -> POLYINFOUIDEND
// (see comments above).  This function gets the index from the pUID.
// --------------------------------------------------------------------------
int sa_index_from_pUID(int pUID)
{
	assert(pUID >= POLYINFOUIDSTART);
	assert(pUID <= POLYINFOUIDEND);
	return pUID - POLYINFOUIDSTART;
}

// --------------------------------------------------------------------------
// With the absence of a standard recalloc function, want to make sure that
// the polyinfostore is initialised with appropriate zeros so that we can
// detect missing poly information
// --------------------------------------------------------------------------
void sa_clear_polyinfostore(int startindex)
{
	int index;
	
	for (index = startindex; index < polyinfostoresize; index++) {
		/* just clearing puid is sufficient for detection by sa_getpoly */
		(polyinfostore[index]).puid = 0;
	}
}

// --------------------------------------------------------------------------
// Add a new polygon, returning a pointer to the new poly
// --------------------------------------------------------------------------
polyinfo *sa_addpoly(int puid)
{
	int index;
	
	index = sa_index_from_pUID(puid);
	
	if (polyinfostoresize <= index) {
		int oldsize = polyinfostoresize;
		
		while (polyinfostoresize <= index) {
			polyinfostoresize *= 2;
		}

#ifdef DEBUG
		if (Debug&8) {
			fprintf(stderr,
						"Resizing polyinfostore from %d items to %d items\n",
						oldsize, polyinfostoresize);
		}
#endif

		polyinfostore = erealloc(polyinfostore,
									sizeof(polyinfo) * polyinfostoresize);
		sa_clear_polyinfostore(oldsize+1);
	}
	
	(polyinfostore[index]).puid=puid;
	(polyinfostore[index]).colour.r=1; // Default polygon colour (white)
	(polyinfostore[index]).colour.g=1;
	(polyinfostore[index]).colour.b=1;
	(polyinfostore[index]).colour.a=1;
	(polyinfostore[index]).material=0; // No material
	(polyinfostore[index]).geometry=NULL; // No geometry in polygon initially
	(polyinfostore[index]).texcoords=NULL;
	
	return &(polyinfostore[index]);
}

// --------------------------------------------------------------------------
// Finds polygon number <n> in the polygon list
// --------------------------------------------------------------------------
polyinfo *sa_findpoly(int puid)
{
	int index, foundpuid;
	
	index = sa_index_from_pUID(puid);
	
	foundpuid = polyinfostore[index].puid;
	
	if (foundpuid == 0) {
		// No poly found
		return NULL;
	} else {
		// Found it -- sanity check, then return pointer
		assert(foundpuid == puid);
		return &(polyinfostore[index]);
	}
}

// --------------------------------------------------------------------------
// Finds polygon number <n> in the polygon list, or creates it if it doesn't
// exist
// --------------------------------------------------------------------------
polyinfo *sa_getpoly(int puid)
{
	polyinfo *p;

	p=sa_findpoly(puid);

	if (p==NULL)
	{
		// Create it
		p=sa_addpoly(puid);
	}

	return p;
}

// --------------------------------------------------------------------------
// Finds poly number <puid> in the poly list and deletes its entry and
// vertices
// --------------------------------------------------------------------------

void sa_deletepoly(int puid)
{
	polyinfo *p;
	uidlist *currentuid;
	uidlist *nextuid;

	p = sa_findpoly(puid);
	if(p==NULL)
		return;
	
	currentuid = p->geometry;
	while (currentuid != NULL) {
		sa_deletevertex(currentuid->uid);
		nextuid = currentuid->next;
		free(currentuid);
		currentuid = nextuid;
	}
	
	(polyinfostore[sa_index_from_pUID(puid)]).puid = 0;
}

// --------------------------------------------------------------------------
// Dumps the contents of the polygon list to STDOUT (for debugging)
// --------------------------------------------------------------------------
void sa_dumppolys(void)
{
	int index;
	uidlist *currentuid;
	
	printf("--- Poly list ---\n");
	for (index = 0; index <= polyinfostoresize; index++) {
		if (polyinfostore[index].puid == 0)
			continue;
		
		printf("Poly %d :\n", polyinfostore[index].puid);

		printf(" Geometry :\n");
		currentuid=polyinfostore[index].geometry;
		while (currentuid != NULL) {
			printf("  Vertex %d (link UID %d)\n",
			currentuid->uid, currentuid->n);
			currentuid=currentuid->next;
		}

		printf(" Texture co-ordinates :\n");
		currentuid=polyinfostore[index].texcoords;
		while (currentuid != NULL) {
			printf("  Vertex %d (link UID %d)\n",
			currentuid->uid, currentuid->n);
			currentuid=currentuid->next;
		}
	}
	printf("-----------------\n");

}


// --------------------------------------------------------------------------
// Object-related functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Add a new object to the object list, returning a pointer to the new object
// --------------------------------------------------------------------------

objectinfo *sa_addobject(char* name)
{
	objectinfo *newobj;
	objectinfo *currentobj;

	newobj=malloc(sizeof(objectinfo));
	
	currentobj = sa_findobject(name);
	if (currentobj != NULL) {
		memcpy(newobj,currentobj,sizeof(objectinfo));	//Copy transform settings from old object
		sa_deleteobject(name);
	}
	else {
		newobj->pos.x=0;
		newobj->pos.y=0;
		newobj->pos.z=0;
		newobj->rot.x=0;
		newobj->rot.y=0;
		newobj->rot.z=0;
		newobj->scale.x=1;
		newobj->scale.y=1;
		newobj->scale.z=1;
		newobj->visible=1;
	}
	
	newobj->primitivePolyCount=0;
	
	newobj->name = malloc((strlen(name)+1)*sizeof(char));
	strcpy(newobj->name,name);
	
	newobj->polys=NULL;
	newobj->next=NULL;

	if (firstobject == NULL)
	{
		newobj->prev=NULL;

		firstobject=newobj;
	}
	else
	{
		currentobj=firstobject;
		while ((currentobj->next)!=NULL)
			currentobj=currentobj->next;

		newobj->prev=currentobj;
		currentobj->next=newobj;
	}
	return newobj;
}

// --------------------------------------------------------------------------
// Finds object name <name> in the object list
// --------------------------------------------------------------------------

objectinfo *sa_findobject(char* name)
{
	objectinfo *currentobj;
	
	if (firstobject==NULL)
	{
		// No objects in list
		return NULL;
	}

	currentobj=firstobject;

	while ((strcmp(currentobj->name,name)!=0) && ((currentobj->next)!=NULL))
		currentobj=currentobj->next;

	if (strcmp(currentobj->name,name)==0)
	{
		// Object found
		return currentobj;
	}
	else
	{
		// No object found
		return NULL;
	}
}

// --------------------------------------------------------------------------
// Finds object name <name> in the object list, or creates it if it doesn't
// exist
// --------------------------------------------------------------------------

objectinfo *sa_getobject(char* name)
{
	objectinfo *r;
	r=sa_findobject(name);
	if (r==NULL)
	{
		// Create it
		r=sa_addobject(name);
	}

	return r;
}

// --------------------------------------------------------------------------
// Delete the primitive polys of an object, returns pointer to object
// --------------------------------------------------------------------------

objectinfo *sa_truncprimitive(char* name)
{
	objectinfo *o;
	uidlist *currentuid;
	uidlist *nextuid;

	o = sa_findobject(name);
	if(o == NULL)
		return;
	
	currentuid = o->polys;
	while (currentuid != NULL && o->primitivePolyCount > 0) {
		sa_deletepoly(currentuid->uid);
		nextuid = currentuid->next;
		free(currentuid);
		currentuid = nextuid;
		
		o->primitivePolyCount--;
	}
	o->polys = currentuid;
	
	return o;
}

// --------------------------------------------------------------------------
// Finds object name <name> in the object list and deletes its entry, polys
// and vertices
// --------------------------------------------------------------------------

void sa_deleteobject(char* name)
{
	objectinfo *o;
	uidlist *currentuid;
	uidlist *nextuid;

	o = sa_findobject(name);
	if(o == NULL)
		return;
	
	currentuid = o->polys;
	while (currentuid != NULL) {
		sa_deletepoly(currentuid->uid);
		nextuid = currentuid->next;
		free(currentuid);
		currentuid = nextuid;
	}
	if(o->next != NULL)
		(o->next)->prev = o->prev;
	if(o->prev != NULL)
		(o->prev)->next = o->next;
	
	if(firstobject == o)
		firstobject = o->next;
	free(o->name);
	free(o);
}

// --------------------------------------------------------------------------
// Dumps the contents of the object list to STDOUT (for debugging)
// --------------------------------------------------------------------------

void sa_dumpobjects(void)
{
	objectinfo *currentobject;
	uidlist *currentuid;

	if (firstobject!=NULL)
	{
		currentobject=firstobject;

		printf("--- Object list ---\n");

		while (currentobject!=NULL)
		{
			printf("Object %s :\n",currentobject->name);

			printf(" Polys :\n");
			currentuid=currentobject->polys;
			while (currentuid!=NULL)
			{
				printf("  Poly %d (link UID %d)\n",currentuid->uid,currentuid->n);
				currentuid=currentuid->next;
			}

			currentobject=currentobject->next;
		}

		printf("-------------------\n");

	}
	else
	{
		printf("No objects to list.\n");
	}
}

// --------------------------------------------------------------------------
// Returns a pointer to the first object
// --------------------------------------------------------------------------

objectinfo *sa_getfirstobject(void)
{
	return firstobject;
}

// --------------------------------------------------------------------------
// Material-related functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Add a new material to the material list, returning a pointer to the new
// material.
// --------------------------------------------------------------------------

materialinfo *sa_addmaterial(int n)
{
	materialinfo *newmaterial;
	materialinfo *currentmaterial;

	newmaterial=malloc(sizeof(materialinfo));

	newmaterial->n=n;
	newmaterial->ambient.r=0.1;
	newmaterial->ambient.g=0.1;
	newmaterial->ambient.b=0.1;
	newmaterial->ambient.a=1;
	newmaterial->diffuse.r=0.7;
	newmaterial->diffuse.g=0.7;
	newmaterial->diffuse.b=0.7;
	newmaterial->diffuse.a=1;
	newmaterial->specular.r=1;
	newmaterial->specular.g=1;
	newmaterial->specular.b=1;
	newmaterial->specular.a=1;
	newmaterial->texturefile=NULL;
	newmaterial->texture=0;
	newmaterial->textureneedsloading=0;
	newmaterial->next=NULL;

	if (firstmaterial == NULL)
	{
		newmaterial->prev=NULL;

		firstmaterial=newmaterial;
	}
	else
	{
		currentmaterial=firstmaterial;
		while ((currentmaterial->next)!=NULL)
			currentmaterial=currentmaterial->next;

		newmaterial->prev=currentmaterial;
		currentmaterial->next=newmaterial;
	}
	return newmaterial;
}

// --------------------------------------------------------------------------
// Finds material number <n> in the list
// --------------------------------------------------------------------------

materialinfo *sa_findmaterial(int n)
{
	materialinfo *currentmaterial;

	if (firstmaterial==NULL)
	{
		// No materials in list
		return NULL;
	}

	currentmaterial=firstmaterial;

	while (((currentmaterial->n)!=n) && ((currentmaterial->next)!=NULL))
		currentmaterial=currentmaterial->next;

	if ((currentmaterial->n)==n)
	{
		// Material found
		return currentmaterial;
	}
	else
	{
		// Nothing found
		return NULL;
	}
}

// --------------------------------------------------------------------------
// Finds material number <n> in the list, or creates it if it doesn't
// exist
// --------------------------------------------------------------------------

materialinfo *sa_getmaterial(int n)
{
	materialinfo *r;

	r=sa_findmaterial(n);

	if (r==NULL)
	{
		// Create it
		r=sa_addmaterial(n);
	}

	return r;
}

// --------------------------------------------------------------------------
// Dumps the contents of the material list to STDOUT (for debugging)
// --------------------------------------------------------------------------

void sa_dumpmaterials(void)
{
	materialinfo *currentmaterial;

	if (firstmaterial!=NULL)
	{
		currentmaterial=firstmaterial;

		printf("--- Material list ---\n");

		while (currentmaterial!=NULL)
		{
			printf("Material %d :\n",currentmaterial->n);

			printf("Ambient  : %g,%g,%g\n",currentmaterial->ambient.r,currentmaterial->ambient.g,currentmaterial->ambient.b);
			printf("Diffuse  : %g,%g,%g\n",currentmaterial->diffuse.r,currentmaterial->diffuse.g,currentmaterial->diffuse.b);
			printf("Specular : %g,%g,%g\n",currentmaterial->specular.r,currentmaterial->specular.g,currentmaterial->specular.b);
			if (currentmaterial->texturefile==NULL)
			{
				printf("No texture\n");
			}
			else
			{
				printf("Texture file : \"%s\" \n",currentmaterial->texturefile);
			}

			currentmaterial=currentmaterial->next;
		}

		printf("---------------------\n");

	}
	else
	{
		printf("No materials to list.\n");
	}
}

// --------------------------------------------------------------------------
// Returns a pointer to the first material
// --------------------------------------------------------------------------

materialinfo *sa_getfirstmaterial(void)
{
	return firstmaterial;
}
// --------------------------------------------------------------------------
// Light-related functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Add a new light to the light list, returning a pointer to the new
// light.
// --------------------------------------------------------------------------

lightinfo *sa_addlight(int n)
{
	lightinfo *newlight;
	lightinfo *currentlight;

	newlight=malloc(sizeof(lightinfo));

	newlight->n=n;
	newlight->ambient.r=0.1;
	newlight->ambient.g=0.1;
	newlight->ambient.b=0.1;
	newlight->ambient.a=1;
	newlight->diffuse.r=0.7;
	newlight->diffuse.g=0.7;
	newlight->diffuse.b=0.7;
	newlight->diffuse.a=1;
	newlight->specular.r=1;
	newlight->specular.g=1;
	newlight->specular.b=1;
	newlight->specular.a=1;
	newlight->pos.x=0;
	newlight->pos.y=0;
	newlight->pos.z=0;
	newlight->attenuation=0;
	newlight->enabled=1;
	newlight->directional=0;
	newlight->next=NULL;

	if (firstlight == NULL)
	{
		newlight->prev=NULL;

		firstlight=newlight;
	}
	else
	{
		currentlight=firstlight;
		while ((currentlight->next)!=NULL)
			currentlight=currentlight->next;

		newlight->prev=currentlight;
		currentlight->next=newlight;
	}
	return newlight;
}

// --------------------------------------------------------------------------
// Finds light number <n> in the list
// --------------------------------------------------------------------------

lightinfo *sa_findlight(int n)
{
	lightinfo *currentlight;

	if (firstlight==NULL)
	{
		// No lights in list
		return NULL;
	}

	currentlight=firstlight;

	while (((currentlight->n)!=n) && ((currentlight->next)!=NULL))
		currentlight=currentlight->next;

	if ((currentlight->n)==n)
	{
		// light found
		return currentlight;
	}
	else
	{
		// Nothing found
		return NULL;
	}
}

// --------------------------------------------------------------------------
// Finds light number <n> in the list, or creates it if it doesn't
// exist
// --------------------------------------------------------------------------

lightinfo *sa_getlight(int n)
{
	lightinfo *r;

	r=sa_findlight(n);

	if (r==NULL)
	{
		// Create it
		r=sa_addlight(n);
	}

	return r;
}

// --------------------------------------------------------------------------
// Dumps the contents of the light list to STDOUT (for debugging)
// --------------------------------------------------------------------------

void sa_dumplights(void)
{
	lightinfo *currentlight;

	if (firstlight!=NULL)
	{
		currentlight=firstlight;

		printf("--- light list ---\n");

		while (currentlight!=NULL)
		{
			printf("light %d :\n",currentlight->n);

			if (currentlight->enabled==1)
				printf("Enabled\n");
			else
				printf("Disabled\n");

			if (currentlight->directional==1)
				printf("Directional\n");
			else
				printf("Positional\n");

			printf("Position : %g,%g,%g\n",currentlight->pos.x,currentlight->pos.y,currentlight->pos.z);
			printf("Ambient  : %g,%g,%g\n",currentlight->ambient.r,currentlight->ambient.g,currentlight->ambient.b);
			printf("Diffuse  : %g,%g,%g\n",currentlight->diffuse.r,currentlight->diffuse.g,currentlight->diffuse.b);
			printf("Specular : %g,%g,%g\n",currentlight->specular.r,currentlight->specular.g,currentlight->specular.b);
			printf("Attenuation : %g\n",currentlight->attenuation);

			currentlight=currentlight->next;
		}

		printf("---------------------\n");

	}
	else
	{
		printf("No lights to list.\n");
	}
}

// --------------------------------------------------------------------------
// Returns a pointer to the first light
// --------------------------------------------------------------------------

lightinfo *sa_getfirstlight(void)
{
	return firstlight;
}

// --------------------------------------------------------------------------
// General list-related functions
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Add a new entry to <list>, returning a pointer to it
// --------------------------------------------------------------------------

list *sa_addlist(list **first,int n)
{
	list *newlist;
	list *currentlist;

	newlist=malloc(sizeof(list));

	newlist->n=n;
	newlist->idata=0; // No data to start with
	newlist->data=NULL;

	newlist->next=NULL;

	if ((*first) == NULL)
	{
		newlist->prev=NULL;

		(*first)=newlist;
	}
	else
	{
		currentlist=(*first);
		while ((currentlist->next)!=NULL)
			currentlist=currentlist->next;

		newlist->prev=currentlist;
		currentlist->next=newlist;
	}

	return newlist;
}

// --------------------------------------------------------------------------
// Finds list entry number <n>
// --------------------------------------------------------------------------

list *sa_findlist(list **first,int n)
{
	list *currentlist;

	if ((*first)==NULL)
	{
		// No UIDs in list
		return NULL;
	}

	currentlist=(*first);

	while (((currentlist->n)!=n) && ((currentlist->next)!=NULL))
		currentlist=currentlist->next;

	if ((currentlist->n)==n)
	{
		// UID found
		return currentlist;
	}
	else
	{
		// No UID found
		return NULL;
	}
}

// --------------------------------------------------------------------------
// Finds UID number <n> in the UID list, or creates it if it doesn't exist
// --------------------------------------------------------------------------

list *sa_getlist(list **first,int n)
{
	list *r;

	r=sa_findlist(first,n);

	if (r==NULL)
	{
		// Create it
		r=sa_addlist(first,n);
	}

	return r;
}

#endif /* WANT_SASAMI */
