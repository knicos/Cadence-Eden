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
// functions.c - Sasami functions called from within EDEN
//				 They are called by stub functions in eden/builtin.c, which
//				 are bound to the appropriate EDEN functions in
//				 eden/builtin.h
// --------------------------------------------------------------------------

#include "../../../config.h"

#ifdef WANT_SASAMI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// [Richard] : for strlen() etc.
#include <math.h>	// [Richard] : for sqrt() etc.

#include "structures.h"
#include "debug.h"
#include "render.h"

// --------------------------------------------------------------------------
// Called whenever a vertex co-ordinate is updated
// n contains the vertex number, and x,y & z are the new co-ordinates
// --------------------------------------------------------------------------

void sasami_vertex(int n,double x,double y,double z)
{
	vertexinfo *v;

	v=sa_findvertex(n);
	if (v==NULL)
	{
		// Vertex not found so it must be a new one...
#ifdef SASAMI_DEBUG
		printf("Vertex %d created at %g,%g,%g\n",n,x,y,z);
#endif
		v=sa_addvertex(n,x,y,z);
	}
	else
	{
		// Existing vertex - just move it
#ifdef SASAMI_DEBUG
		printf("Vertex %d moved to %g,%g,%g\n",n,x,y,z);
#endif
		v->pos.x=x;
		v->pos.y=y;
		v->pos.z=z;
	}
	sa_r_update(); // Tell the renderer we've changed something
}

// --------------------------------------------------------------------------
// Called whenever the background colour is updated
// --------------------------------------------------------------------------

void sasami_set_bgcolour(double r,double g,double b)
{
	extern colourinfo sa_r_bgcolour;

#ifdef SASAMI_DEBUG
	printf("Background colour set to %g,%g,%g\n",r,g,b);
#endif

	// Update the colour
	sa_r_bgcolour.r=r;
	sa_r_bgcolour.g=g;
	sa_r_bgcolour.b=b;
    sa_r_bgcolour.a=1; // Alpha is always 1 - it makes no sense to have a transparent background!
	// Tell the renderer
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever polygon <p>'s geometry vertex <v> is changed to <id>
// --------------------------------------------------------------------------

void sasami_poly_geom_vertex(int p,int v,int id)
{
	polyinfo	*poly;
	uidlist		*geometry;

	// First find/create polygon <p>

	poly=sa_getpoly(p);

	// Then find/create entry <v> in its UID list

	geometry=sa_getuidlist(&(poly->geometry),v);

	// ...and then set it

	geometry->uid=id;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever polygon <p>'s texture vertex <v> is changed to <id>
// --------------------------------------------------------------------------

void sasami_poly_tex_vertex(int p,int v,int id)
{
	polyinfo	*poly;
	uidlist		*geometry;

	// First find/create polygon <p>

	poly=sa_getpoly(p);

	// Then find/create entry <v> in its UID list

	geometry=sa_getuidlist(&(poly->texcoords),v);

	// ...and then set it

	geometry->uid=id;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever polygon <p>'s colour is changed to <r>,<g>,<b>,<a>
// --------------------------------------------------------------------------

void sasami_poly_colour(int p,double r,double g,double b,double a)
{
	polyinfo	*poly;

#ifdef SASAMI_DEBUG
	printf("Setting polygon %d colour to %g,%g,%g,%g\n",p,r,g,b,a);
#endif

	// Find/create polygon <p>

	poly=sa_getpoly(p);

	// Set the colour

	poly->colour.r=r;
	poly->colour.g=g;
	poly->colour.b=b;
	poly->colour.a=a;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever polygon <p>'s material is changed to <m>
// --------------------------------------------------------------------------

void sasami_poly_material(int p,int m)
{
	polyinfo	*poly;

#ifdef SASAMI_DEBUG
	printf("Setting polygon %d material to %d\n",p,m);
#endif

	// Find/create polygon <p>

	poly=sa_getpoly(p);

	// Set the material

	poly->material=m;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever object <o>'s polygon <p> is changed to <id>
// --------------------------------------------------------------------------

void sasami_object_poly(char* o,int p,int id)
{
	objectinfo	*obj;
	uidlist		*poly;

	// First find/create object <o>

	obj=sa_getobject(o);

	// Then find/create entry <p> in its UID list

	poly=sa_getuidlist(&(obj->polys),p);

	// ...and then set it

	poly->uid=id;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever object <o>'s position is changed to <x>,<y>,<z>
// --------------------------------------------------------------------------

void sasami_object_pos(char* o,double x,double y,double z)
{
	objectinfo	*object;

#ifdef SASAMI_DEBUG
	printf("Setting object %s position to %g,%g,%g\n",o,x,y,z);
#endif

	// Find/create object <o>

	object=sa_getobject(o);

	// Set the position

	object->pos.x=x;
	object->pos.y=y;
	object->pos.z=z;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever object <o>'s rotation is changed to <x>,<y>,<z>
// --------------------------------------------------------------------------

void sasami_object_rot(char* o,double x,double y,double z)
{
	objectinfo	*object;

#ifdef SASAMI_DEBUG
	printf("Setting object %s rotation to %g,%g,%g\n",o,x,y,z);
#endif

	// Find/create object <o>

	object=sa_getobject(o);

	// Set the position

	object->rot.x=x;
	object->rot.y=y;
	object->rot.z=z;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever object <o>'s scale is changed to <x>,<y>,<z>
// --------------------------------------------------------------------------

void sasami_object_scale(char* o,double x,double y,double z)
{
	objectinfo	*object;

#ifdef SASAMI_DEBUG
	printf("Setting object %s scale to %g,%g,%g\n",o,x,y,z);
#endif

	// Find/create object <o>

	object=sa_getobject(o);

	// Set the scale

	object->scale.x=x;
	object->scale.y=y;
	object->scale.z=z;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever object <o>'s visible status is changed to <v>
// --------------------------------------------------------------------------

void sasami_object_visible(char* o,int v)
{
	objectinfo	*object;

#ifdef SASAMI_DEBUG
	printf("Setting object %s visible flag to %d\n",o,v);
#endif

	// Find/create object <o>

	object=sa_getobject(o);

	// Set the flag

	object->visible=v;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever the viewport size is changed
// --------------------------------------------------------------------------

void sasami_viewport(int x,int y)
{
#ifdef SASAMI_DEBUG
	printf("Setting viewport size to %dx%d\n",x,y);
#endif

	sa_r_resizeviewport(x,y);
}

// --------------------------------------------------------------------------
// Called whenever the "show axes" state is changed
// --------------------------------------------------------------------------

void sasami_setshowaxes(int n)
{
#ifdef SASAMI_DEBUG
	printf("Setting axes display to %d\n",n);
#endif

	sa_r_setshowaxes(n);
}

// --------------------------------------------------------------------------
// Called whenever material <m>'s ambient is changed to <r>,<g>,<b>,<a>
// --------------------------------------------------------------------------

void sasami_material_ambient(int m,double r,double g,double b,double a)
{
	materialinfo	*material;

#ifdef SASAMI_DEBUG
	printf("Setting material %d ambient to %g,%g,%g,%g\n",m,r,g,b,a);
#endif

	// Find/create material <m>

	material=sa_getmaterial(m);

	// Set the colour

	material->ambient.r=r;
	material->ambient.g=g;
	material->ambient.b=b;
	material->ambient.a=a;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever material <m>'s diffuse is changed to <r>,<g>,<b>,<a>
// --------------------------------------------------------------------------

void sasami_material_diffuse(int m,double r,double g,double b,double a)
{
	materialinfo	*material;

#ifdef SASAMI_DEBUG
	printf("Setting material %d diffuse to %g,%g,%g,%g\n",m,r,g,b,a);
#endif

	// Find/create material <m>

	material=sa_getmaterial(m);

	// Set the colour

	material->diffuse.r=r;
	material->diffuse.g=g;
	material->diffuse.b=b;
	material->diffuse.a=a;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever material <m>'s specular is changed to <r>,<g>,<b>,<a>
// --------------------------------------------------------------------------

void sasami_material_specular(int m,double r,double g,double b,double a)
{
	materialinfo	*material;

#ifdef SASAMI_DEBUG
	printf("Setting material %d specular to %g,%g,%g,%g\n",m,r,g,b,a);
#endif

	// Find/create material <m>

	material=sa_getmaterial(m);

	// Set the colour

	material->specular.r=r;
	material->specular.g=g;
	material->specular.b=b;
	material->specular.a=a;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever material <m>'s texture is changed to <t>
// --------------------------------------------------------------------------

void sasami_material_texture(int m,char *t)
{
	materialinfo	*material;

#ifdef SASAMI_DEBUG
	printf("Setting material %d texture to \"%s\"\n",m,t);
#endif

	// Find/create material <m>

	material=sa_getmaterial(m);

	// Set the texture

	material->texturefile=realloc(material->texturefile,strlen(t)+1);
	strcpy(material->texturefile,t);

	// Set the texture binding to 0 and tell the renderer to load it
	// (we can't load texture now as OpenGL may not be initialised!)

	material->texture=0;
	material->textureneedsloading=1;

	// Redraw the display

	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever light <o>'s position is changed to <x>,<y>,<z>
// --------------------------------------------------------------------------

void sasami_light_pos(int o,double x,double y,double z)
{
	lightinfo	*light;

#ifdef SASAMI_DEBUG
	printf("Setting light %d position to %g,%g,%g\n",o,x,y,z);
#endif

	// Find/create light <o>

	light=sa_getlight(o);

	// Set the position

	light->pos.x=x;
	light->pos.y=y;
	light->pos.z=z;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever light <m>'s ambient is changed to <r>,<g>,<b>,<a>
// --------------------------------------------------------------------------

void sasami_light_ambient(int m,double r,double g,double b,double a)
{
	lightinfo	*light;

#ifdef SASAMI_DEBUG
	printf("Setting light %d ambient to %g,%g,%g,%g\n",m,r,g,b,a);
#endif

	// Find/create light <m>

	light=sa_getlight(m);

	// Set the colour

	light->ambient.r=r;
	light->ambient.g=g;
	light->ambient.b=b;
	light->ambient.a=a;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever light <m>'s diffuse is changed to <r>,<g>,<b>,<a>
// --------------------------------------------------------------------------

void sasami_light_diffuse(int m,double r,double g,double b,double a)
{
	lightinfo	*light;

#ifdef SASAMI_DEBUG
	printf("Setting light %d diffuse to %g,%g,%g,%g\n",m,r,g,b,a);
#endif

	// Find/create light <m>

	light=sa_getlight(m);

	// Set the colour

	light->diffuse.r=r;
	light->diffuse.g=g;
	light->diffuse.b=b;
	light->diffuse.a=a;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever light <m>'s specular is changed to <r>,<g>,<b>,<a>
// --------------------------------------------------------------------------

void sasami_light_specular(int m,double r,double g,double b,double a)
{
	lightinfo	*light;

#ifdef SASAMI_DEBUG
	printf("Setting light %d specular to %g,%g,%g,%g\n",m,r,g,b,a);
#endif

	// Find/create light <m>

	light=sa_getlight(m);

	// Set the colour

	light->specular.r=r;
	light->specular.g=g;
	light->specular.b=b;
	light->specular.a=a;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever light <m>'s enabled status is changed to <e>
// --------------------------------------------------------------------------

void sasami_light_enabled(int m,int e)
{
	lightinfo	*light;

#ifdef SASAMI_DEBUG
	printf("Setting light %d enabled flag to %d\n",m,e);
#endif

	// Find/create light <m>

	light=sa_getlight(m);

	// Set the flag

	light->enabled=e;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever light <m>'s attenuation is changed to <a>
// --------------------------------------------------------------------------

void sasami_light_attenuation(int m,double a)
{
	lightinfo	*light;

#ifdef SASAMI_DEBUG
	printf("Setting light %d attenuation to %g\n",m,a);
#endif

	// Find/create light <m>

	light=sa_getlight(m);

	// Set the attenuation

	light->attenuation=a;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever light <m>'s directional status is changed to <d>
// --------------------------------------------------------------------------

void sasami_light_directional(int m,int d)
{
	lightinfo	*light;

#ifdef SASAMI_DEBUG
	printf("Setting light %d directional flag to %d\n",m,d);
#endif

	// Find/create light <m>

	light=sa_getlight(m);

	// Set the flag

	light->directional=d;

	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Set all primitive polys in primitive object <c> to material <mat>
// --------------------------------------------------------------------------

void sasami_primitive_material(char* c, int mat) {
	objectinfo *o;
	uidlist *currentuid;
	int i;
	
	o = sa_findobject(c);
	if(o == NULL)
		return;
	
	o->material = mat;
	i = o->primitivePolyCount;
	
	currentuid = o->polys;
	while (currentuid != NULL && i-- > 0) {
		sasami_poly_material(currentuid->uid, mat);
		currentuid = currentuid->next;
	}
	sa_r_update();
}

void sasami_face_material(char* c, int face, int mat) {
	uidlist *polylist;
	polylist = sa_finduidlistn(&(sa_getobject(c)->polys),face);
	if(polylist != NULL)
		sasami_poly_material(polylist->uid, mat);
}

void sasami_object_delete(char* o) {
	sa_deleteobject(o);
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever cube <c>'s dimensions <w,h,d> are changed
// --------------------------------------------------------------------------

void sasami_cube(char* c, double w, double h, double d) {
	int vid[8],pid[6],i;
	polyinfo *poly;
	uidlist *vertexlist;
	objectinfo *object;
	objectinfo backupObj;
	
#ifdef SASAMI_DEBUG
	printf("Setting cube %s dimensions to %g,%g,%g\n",c,w,h,d);
#endif

	// Get a new object (truncates old one, if exists)
	object = sa_findobject(c);
	if(object == NULL)
		object = sa_addobject(c);
	else
		object = sa_truncprimitive(c);
	
	// Set number of primitive polys in object (used for truncating object when altering geometry)
	object->primitivePolyCount = 6;

	// Divide dimensions by 2 to make (0,0,0) the centre of the cube
	w = w/2;
	h = h/2;
	d = d/2;
	
	// Create vertex uids
	for(i=0;i<8;i++) {
		vid[i]=sa_getvertexUID();
	}
	
	// Create poly uids
	for(i=0;i<6;i++)
		pid[i] = sa_getpolyUID();
	
	// Create the vertices
	
	sa_addvertex(vid[0],-w,-h,d);	//lbf
	sa_addvertex(vid[1],w,-h,d);	//rbf
	sa_addvertex(vid[2],w,h,d);	//rtf
	sa_addvertex(vid[3],-w,h,d);	//ltf
	sa_addvertex(vid[4],-w,-h,-d);	//lbb
	sa_addvertex(vid[5],w,-h,-d);	//rbb
	sa_addvertex(vid[6],w,h,-d);	//rtb
	sa_addvertex(vid[7],-w,h,-d);	//ltb
	
	// Create polys and add vertices to them
	
	poly = sa_addpoly(pid[0]);				// Front poly
	sa_adduidlist(&(poly->geometry),0)->uid = vid[0];//lbf
	sa_adduidlist(&(poly->geometry),1)->uid = vid[1];//rbf
	sa_adduidlist(&(poly->geometry),2)->uid = vid[2];//rtf
	sa_adduidlist(&(poly->geometry),3)->uid = vid[3];//ltf	(right,top,front)
	
	poly = sa_addpoly(pid[1]);				// left poly
	sa_adduidlist(&(poly->geometry),0)->uid = vid[0];
	sa_adduidlist(&(poly->geometry),3)->uid = vid[3];
	sa_adduidlist(&(poly->geometry),7)->uid = vid[7];
	sa_adduidlist(&(poly->geometry),4)->uid = vid[4];
	
	poly = sa_addpoly(pid[2]);				// right poly
	sa_adduidlist(&(poly->geometry),5)->uid = vid[5];
	sa_adduidlist(&(poly->geometry),6)->uid = vid[6];
	sa_adduidlist(&(poly->geometry),2)->uid = vid[2];
	sa_adduidlist(&(poly->geometry),1)->uid = vid[1];
	
	poly = sa_addpoly(pid[3]);				// bottom poly
	sa_adduidlist(&(poly->geometry),4)->uid = vid[4];
	sa_adduidlist(&(poly->geometry),5)->uid = vid[5];
	sa_adduidlist(&(poly->geometry),1)->uid = vid[1];
	sa_adduidlist(&(poly->geometry),0)->uid = vid[0];
	
	poly = sa_addpoly(pid[4]);				// top poly
	sa_adduidlist(&(poly->geometry),2)->uid = vid[6];
	sa_adduidlist(&(poly->geometry),3)->uid = vid[7];
	sa_adduidlist(&(poly->geometry),7)->uid = vid[3];
	sa_adduidlist(&(poly->geometry),6)->uid = vid[2];
	
	poly = sa_addpoly(pid[5]);				// back poly
	sa_adduidlist(&(poly->geometry),7)->uid = vid[7];//ltb
	sa_adduidlist(&(poly->geometry),6)->uid = vid[6];//rtb
	sa_adduidlist(&(poly->geometry),5)->uid = vid[5];//rbb
	sa_adduidlist(&(poly->geometry),4)->uid = vid[4];//lbb
	
	// Add polys to object
	for(i=0;i<6;i++)
		sa_adduidlist(&(object->polys),sa_getUID())->uid = pid[i];

	// Reapply primitive material
	if(object->material)
		sasami_primitive_material(c, object->material);
	
	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever cylinder/cone <c>'s dimensions <l,r,s> are changed.
// --------------------------------------------------------------------------

void sasami_cylinder(char* c, double l, double r1, double r2, int s) {
	int i,toppoly,bottompoly;
	int sides[s],top[s],bottom[s];
	double angle;
	polyinfo *poly;
	objectinfo *object;
	
#ifdef SASAMI_DEBUG
	printf("Setting cylinder %s dimensions to %g,%g,%g,%d\n",c,l,r1,r2,s);
#endif

	// Get a new object (truncates old one, if exists)
	object = sa_findobject(c);
	if(object == NULL)
		object = sa_addobject(c);
	else
		object = sa_truncprimitive(c);
	
	// Set number of primitive polys in object (used for truncating object when altering geometry)
	object->primitivePolyCount = 0;
	
	// Create cap polys and add to object (if required)
	if(r1 > 0) {
		toppoly = sa_getpolyUID();
		sa_addpoly(toppoly);
		sa_adduidlistpos(&(object->polys),sa_getUID(),object->primitivePolyCount)->uid = toppoly;
		object->primitivePolyCount++;
	}
	if(r2 > 0) {
		bottompoly = sa_getpolyUID();
		sa_addpoly(bottompoly);
		sa_adduidlistpos(&(object->polys),sa_getUID(),object->primitivePolyCount)->uid = bottompoly;
		object->primitivePolyCount++;
	}
	
	// Calculate angle between each vertex
	angle = 2 * 3.14159265 / s;
	
	for(i=0;i<s;i++) {
		if(r1 == 0 && i > 0)		// Create top vertices (if top radius is 0, only create one)
			top[i] = top[0];
		else {
			top[i] = sa_getvertexUID();
			sa_addvertex(top[i],r1 * sin(angle * i), r1 * cos(angle * i),l);
		}
		
		if(r2 == 0 && i > 0)		// Create bottom vertices
			bottom[i] = bottom[0];
		else {
			bottom[i] = sa_getvertexUID();
			sa_addvertex(bottom[i],r2 * sin(angle * i), r2 * cos(angle * i),0);
		}
		
		sides[i] = sa_getpolyUID();
		sa_addpoly(sides[i]);						// Create side polys...
		sa_adduidlistpos(&(object->polys),sa_getUID(),object->primitivePolyCount)->uid = sides[i];	// ...and add to object
		object->primitivePolyCount++;
	}
	
	// Set polygon vertices
	for(i=0;i<s;i++) {
		// Set side polys to use either 3 or 4 (or 2!) vertices
		poly = sa_getpoly(sides[i]);
		sa_adduidlist(&(poly->geometry),0)->uid = top[i];
		if(r1 > 0)
			sa_adduidlist(&(poly->geometry),1)->uid = top[i==s-1?0:i+1];
		if(r2 > 0)
			sa_adduidlist(&(poly->geometry),2)->uid = bottom[i==s-1?0:i+1];
		sa_adduidlist(&(poly->geometry),3)->uid = bottom[i];
		
		// Set vertices for cap polys, if necessary
		if(r1 > 0) {
			poly = sa_getpoly(toppoly);
			sa_adduidlist(&(poly->geometry),i)->uid = top[s-1-i];
		}
		if(r2 > 0) {
			poly = sa_getpoly(bottompoly);
			sa_adduidlist(&(poly->geometry),i)->uid = bottom[i];
		}
	}
	
	// Reapply primitive material
	if(object->material)
		sasami_primitive_material(c, object->material);
	
	// Redraw the display
	sa_r_update();
}

// --------------------------------------------------------------------------
// Called whenever sphere <c>'s dimensions <r,s> are changed. Creates a
// longitudinal sphere (perhaps a geodesic sphere would be more appropriate -
// that's what The Eden Project uses after all :)
// --------------------------------------------------------------------------

void sasami_sphere(char* c, double r, int s) {
	int i,j;
	int top,bottom,verts[s-1][s],polys[s][s];
	polyinfo *poly;
	objectinfo *object;
	
	double x,y,z,zRadius,angle;
	
#ifdef SASAMI_DEBUG
	printf("Setting sphere %s radius to %g segments %d\n",c,r,s);
#endif

	// Get a new object (truncates old one, if exists)
	object = sa_findobject(c);
	if(object == NULL)
		object = sa_addobject(c);
	else
		object = sa_truncprimitive(c);
	
	object->primitivePolyCount = 0;
	
	angle = 2 * 3.14159265 / s;
	
	// Iterate through each point on each level
	for(i=1;i<s;i++) {
		z = r - i * (2*r/s);		// Depth of this level
		zRadius	= sqrt(r*r - z*z);	// Radius of this level
		for(j=0;j<s;j++) {
			x = zRadius * sin(angle * j);
			y = zRadius * cos(angle * j);
			verts[i-1][j] = sa_getvertexUID();
			sa_addvertex(verts[i-1][j],x,y,z);
		}
	}
	
	// Create top and bottom vertices
	top = sa_getvertexUID();
	bottom = sa_getvertexUID();
	sa_addvertex(top,0,0,r);
	sa_addvertex(bottom,0,0,-r);
	
	// Generate polygons
	for(i=0;i<s;i++) {
		for(j=0;j<s;j++) {
			polys[i][j] = sa_getpolyUID();
			sa_addpoly(polys[i][j]);					// Create polys...
			sa_adduidlist(&(object->polys),sa_getUID())->uid = polys[i][j];	// ...and add to object
			object->primitivePolyCount++;
		}
	}
	
	// Set polygon vertices
	for(i=0;i<s;i++) {
		for(j=0;j<s;j++) {
			poly = sa_getpoly(polys[i][j]);
			if(i==0) {			// Top 'cap'
				sa_adduidlist(&(poly->geometry),0)->uid = verts[i][j==s-1?0:j+1];
				sa_adduidlist(&(poly->geometry),1)->uid = verts[i][j];
				sa_adduidlist(&(poly->geometry),2)->uid = top;
			}
			else if(i==s-1) {		// Bottom 'cap'
				sa_adduidlist(&(poly->geometry),0)->uid = verts[i-1][j];
				sa_adduidlist(&(poly->geometry),1)->uid = verts[i-1][j==s-1?0:j+1];
				sa_adduidlist(&(poly->geometry),2)->uid = bottom;
			}
			else {				// The rest
				sa_adduidlist(&(poly->geometry),0)->uid = verts[i-1][j==s-1?0:j+1];
				sa_adduidlist(&(poly->geometry),1)->uid = verts[i][j==s-1?0:j+1];
				sa_adduidlist(&(poly->geometry),2)->uid = verts[i][j];
				sa_adduidlist(&(poly->geometry),3)->uid = verts[i-1][j];
			}
		}
	}
	// Reapply primitive material
	if(object->material)
		sasami_primitive_material(c, object->material);
	
	// Redraw the display
	sa_r_update();
}

#endif /* WANT_SASAMI */
