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
// parser.c - Sasami parser - takes Sasami statements and outputs EDEN code
// --------------------------------------------------------------------------

#include "../../../../../config.h"

#ifdef WANT_SASAMI

#include "../EX/script.h"
#include <tk.h>
#include "utils.h"
#include "debug.h"
#include "structures.h"
#include "../Eden/error.h"

// [Richard]
#include "../Eden/emalloc.h"
#include <string.h> // [Richard] : For strlen() etc.

#ifdef DEBUG
extern int Debug;
#define DEBUGPRINT(s,i) if (Debug&8) fprintf(stderr, s, i);
#define DEBUGPRINT2(s,i,j) if (Debug&8) fprintf(stderr, s, i, j);
#else
#define DEBUGPRINT(s,i)
#define DEBUGPRINT2(s,i,j)
#endif /* DEBUG */

// --------------------------------------------------------------------------
// Define some limits
// --------------------------------------------------------------------------

#define SA_MAX_INPUTSTRING	256	// The max length of string to parse

// --------------------------------------------------------------------------
// This is the script that Sasami will output with EDEN definations in it
// --------------------------------------------------------------------------

Script	*sa_script;

// --------------------------------------------------------------------------
// Useful variables
// --------------------------------------------------------------------------

char	sa_inputstring[SA_MAX_INPUTSTRING+1];
bool	sa_skipline;
bool	sa_eden_initialised = false; // Have we set up our global Eden definations yet?
list	*objmateriallist;

// --------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------

void sa_parsestring(char *input);

// --------------------------------------------------------------------------
// Error handler
// --------------------------------------------------------------------------

void sa_error(char *s)
{
    extern Tcl_Interp *interp;
    Tcl_DString err, message;

    deleteScript(sa_script);

    // Blank input string
    sa_inputstring[0]=0;

    errorf("Sasami: %s", s);
}

// --------------------------------------------------------------------------
// This is set up as an exit function during initialisation (above)
// so that we can be sure to clean up before Eden exits
// --------------------------------------------------------------------------

void sa_exit_trap(void)
{
  DEBUGPRINT("\n--------------------\nSasami closing down!\n--------------------\n", 0);
  sa_r_closedisplay();
}

// --------------------------------------------------------------------------
// This writes EDEN statements to the script for EDEN to parse
// --------------------------------------------------------------------------

void sa_output(char *s)
{
  DEBUGPRINT("Out:%s\n", s);
  appendEden(s,sa_script);
}

// --------------------------------------------------------------------------
// Called to initialise Sasami
// --------------------------------------------------------------------------

void sa_init_sasami()
{
  DEBUGPRINT("\n--------------------\nSasami initialising!\n--------------------\n", 0);
	// Initialise some parser variables

	sa_inputstring[0]=0;
	sa_skipline=false;

	// Give the renderer a chance to initialise stuff
	// NOTE : This is *NOT* where the renderer creates the display window, context
	//        and so on. That happens when open_display is called.

	sa_r_init();

	// Allocate memory for initial data structures
	sa_structures_init();

	// Register an exit function so that we can be sure that our closedown code will
	// be called...

	atexit(sa_exit_trap);
}

// --------------------------------------------------------------------------
// This gets sent Sasami input one character at a time from eden/lex.c
// --------------------------------------------------------------------------

void sa_input(char c)
{
  char temp[2];
  if ( (c != '\n') && (c != '\r') ) {
    if (!sa_skipline) {
      // Add character to input string
      if (strlen(sa_inputstring)==SA_MAX_INPUTSTRING) {
	sa_error("Input string too long!");
	sa_skipline=true;
      } else {
	sprintf(temp,"%c",c);		// For safety
	strcat(sa_inputstring,temp);
      }
    } else {
      /* We want to skip this line for some reason, so just trash the
         character */
    }

  } else {
    // Finished string
    if (sa_skipline) {
      // Don't parse this line, just reset flag
      sa_skipline=false;
    } else {
      // Parse string
#ifdef SASAMI_DEBUG
      //			printf("\n%s\n",sa_inputstring);
#endif
      sa_parsestring(sa_inputstring);
    }
    // Blank input string
    sa_inputstring[0]=0;
  }
}

// --------------------------------------------------------------------------
// This initialised the various "global" Eden definations for Sasami stuff,
// like the viewport size and background colour, and monitor functions to
// watch them for changes
// --------------------------------------------------------------------------

void sa_eden_init(void)
{
	char	o[SA_MAX_INPUTSTRING+1];
	extern	colourinfo	sa_r_bgcolour;
	extern	int			sa_r_xsize;
	extern	int			sa_r_ysize;
	extern	int			sa_r_bpp;
	extern	bool		sa_r_showaxes;

	sa_eden_initialised=true;

	// Background colour variables -
	// sasami_bgcolour_r,sasami_bgcolour_g,sasami_bgcolour_b

	sprintf(o,"sasami_bgcolour_r=%d;\n",sa_r_bgcolour.r);
	sa_output(o);
	sprintf(o,"sasami_bgcolour_g=%d;\n",sa_r_bgcolour.g);
	sa_output(o);
	sprintf(o,"sasami_bgcolour_b=%d;\n",sa_r_bgcolour.b);
	sa_output(o);
	sprintf(o,"proc _sasami_bgcolour_mon : sasami_bgcolour_r,sasami_bgcolour_g,sasami_bgcolour_b { sasami_set_bgcolour(sasami_bgcolour_r,sasami_bgcolour_g,sasami_bgcolour_b); };\n");
	sa_output(o);

	// Viewport dimensions and colour depth -
	// sasami_viewport_xsize,sasami_viewport_ysize,sasami_viewport_bpp

	sprintf(o,"sasami_viewport_xsize=%d;\n",sa_r_xsize);
	sa_output(o);
	sprintf(o,"sasami_viewport_ysize=%d;\n",sa_r_ysize);
	sa_output(o);
	sprintf(o, "sasami_viewport_bpp is int(tcl(\"winfo depth .\"));\n");
	sa_output(o);
	sprintf(o,"proc _sasami_viewport_mon : sasami_viewport_xsize,sasami_viewport_ysize { sasami_viewport(sasami_viewport_xsize,sasami_viewport_ysize); };\n");
	sa_output(o);

	// Axes display

	if (sa_r_showaxes)
	{
		sprintf(o,"sasami_show_axes=1;\n");
	}
	else
	{
		sprintf(o,"sasami_show_axes=0;\n");
	}
	sa_output(o);
	sprintf(o,"proc _sasami_showaxes_mon : sasami_show_axes { sasami_setshowaxes(sasami_show_axes); };\n");
	sa_output(o);
}

// --------------------------------------------------------------------------
// This parses a Lightwave format .MTL file and emits Sasami code to create
// it - the Sasami statements are passed to sa_parsestring for parsing.
// <name> is prefixed onto all the produced Sasami names to keep them unique
// (MTL files contain material definations to go with .OBJ files)
// --------------------------------------------------------------------------

void sa_load_mtl(char *name,char *file)
{
	char	o[SA_MAX_INPUTSTRING+30];
	char	cmd[SA_MAX_INPUTSTRING+30];
	char	sarg[3][SA_MAX_INPUTSTRING+30];
	char	l[SA_MAX_INPUTSTRING+30];
	FILE	*objfile;
	int		n,i,id;
	int		darg[6];
	bool	accepted;
	char	currentmaterial[SA_MAX_INPUTSTRING+30];
	list	*newmaterial;

	strcpy(currentmaterial,"none");

	objfile=fopen(file,"r");
	if (objfile==NULL)
	{
		sprintf(o,"Unable to open MTL file '%s'!",file);
		sa_error(o);
	}
	else
	{
		while (!feof(objfile))
		{
			// Read a line from the file

			l[0]='\0';	// To avoid lines with no characters acidentally including
						// the first character of the next line.

			n=-1;
			do
			{
				n++;
				fread(&l[n],sizeof(char),1,objfile);
			} while ((l[n]!=13) && (l[n]!=10) && (!feof(objfile)));

			if ((l[n]!=13) && (l[n]!=10)) // Overwrite EOL marker if present
			 n++;
			l[n]='\0'; // Ensure there's a terminator

			// Convert # signs to \0 - this makes comments a line terminator,
			// effectively stripping them.

			for (i=0;l[i]!='\0';i++)
			{
				if (l[i]=='#') l[i]='\0';
			}

			// Convert to lowercase

			for	(i=0;l[i]!='\0';i++)
			{
				l[i]=tolower(l[i]);
			}

			// Get the first word (the command)

			n=sscanf(l," %s \n",cmd); /* possible problem
                                                     with \r's here,
                                                     but I don't know
                                                     about .MTL format
                                                     so I'll leave it
                                                     [Ash] */

			if (n==1)
			{
				accepted=false;

				// ---------------------------------------------------------------
				// "newmtl" command - Add a material
				// ---------------------------------------------------------------

				if (strcmp(cmd,"newmtl")==0)
				{
					n=sscanf(l," newmtl %s ",sarg[0]);
					if (n==1)
					{
						// Create the material
						// This does not use SA_PARSESTRING because we need the material
						// id immediately (for creating polygon->material links with
						// nodetail set)
						id=sa_getUID(); // Create a new ID
						// Create the material
						sa_addmaterial(id);
						// First set the material name to contain the ID for future reference
						sprintf(o,"%s_mat_%s=%d;\n",name,sarg[0],id);
						sa_output(o);

						// Store the material ID with its name

						newmaterial=sa_addlist(&objmateriallist,sa_getUID());
						newmaterial->idata=id;
						newmaterial->data=(void *)malloc(strlen(sarg[0])+2);
						strcpy((char *)newmaterial->data,sarg[0]);

						DEBUGPRINT2("Got material name '%s', assigned id %d\n", sarg[0], id);

						// Set it as current
						strcpy(currentmaterial,sarg[0]);
					}
					else
					{
						sa_error("MTL file error - could not get material name!");
					}
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "map_kd" command - Set texture filename
				// ---------------------------------------------------------------

				if (strcmp(cmd,"map_kd")==0)
				{
					n=sscanf(l," map_kd %s ",sarg[0]);
					if (n==1)
					{
						// Set the texture
						sprintf(o,"material_texture %s_mat_%s \"%s\"\n",name,currentmaterial,sarg[0]);
						sa_parsestring(o);
					}
					else
					{
						sa_error("MTL file error - could not get texture filename!");
					}
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "kd" command - Set the material diffuse colour
				// ---------------------------------------------------------------

				if (strcmp(cmd,"kd")==0)
				{
					n=sscanf(l," kd %s %s %s ",sarg[0],sarg[1],sarg[2]);
					if (n==3)
					{
						// Set the colour
						sprintf(o,"material_diffuse %s_mat_%s %s %s %s\n",name,currentmaterial,sarg[0],sarg[1],sarg[2]);
						sa_parsestring(o);
					}
					else
					{
						sa_error("MTL file error - could not get diffuse colour!");
					}
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "ns" command - Unknown (ignore)
				// ---------------------------------------------------------------

				if (strcmp(cmd,"ns")==0)
				{
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "ks" command - Unknown (ignore)
				// ---------------------------------------------------------------

				if (strcmp(cmd,"ks")==0)
				{
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "illum" command - Unknown (ignore)
				// ---------------------------------------------------------------

				if (strcmp(cmd,"illum")==0)
				{
					accepted=true;
				}

				// ---------------------------------------------------------------
				// End of Lightwave MTL command list
				// ---------------------------------------------------------------

				if (!accepted)
				{
					sprintf(o,"MTL file error - unknown command '%s'",l);
					sa_error(o);
				}
			}
		}
		fclose(objfile);
	}
}

// --------------------------------------------------------------------------
// This converts a string (in [-]dd.dddd format) into a double value.
// I'm using this because atof() doesn't seem to be working(!) - it was
// returning "55556" for "-5.5556" (ie ignoring the - and . signs)
// --------------------------------------------------------------------------

double strtodouble(char *s)
{
	char *c;
	double result;
	bool negative,indecimal;
	int i,d,decimalcount;
	int exponent;

	c=s;
	result=0;
	negative=false;
	indecimal=false;
	decimalcount=0;
	exponent=0;

	while (((*c)!='\0') & (exponent==0))
	{
		if (((*c)=='e') | ((*c)=='E'))
		{
			exponent=strtodouble(c+1);
		}
		if ((*c)=='-')
		{
			negative=true;
		}
		if ((*c)=='.')
			indecimal=true;
		if (((*c)>='0') && ((*c)<='9'))
		{
			d=(int) ((*c)-'0');
			result=result*10;
			result=result+d;

			if (indecimal)
				decimalcount++;
		}
		c++;
	}

	if (negative)
	{
		result=-result;
	}

	for (i=1;i<=decimalcount;i++)
		result=result / 10;

	if (exponent>0)
		for (i=1;i<=exponent;i++)
			result=result*10;
	if (exponent<0)
		for (i=-1;i>=exponent;i--)
			result=result/10;

	return(result);
}

// --------------------------------------------------------------------------
// This parses a Lightwave format .OBJ file and emits Sasami code to create
// it - the Sasami statements are passed to sa_parsestring for parsing.
// <name> is prefixed onto all the produced Sasami names to keep them unique
// If <nodetail> is true then vertices and polygons are *not* created as Eden
// declarations, keeping the calculation overhead down (at the cost of not
// allowing model geometry to be dynamically altered)
// --------------------------------------------------------------------------

void sa_load_obj(char *name,char *file,bool nodetail)
{
	char	o[SA_MAX_INPUTSTRING+30];
	char	cmd[SA_MAX_INPUTSTRING+30];
	char	l[SA_MAX_INPUTSTRING+30];
	char	currentobjectname[SA_MAX_INPUTSTRING+30];
	char	currentmaterial[SA_MAX_INPUTSTRING+30];
	int		currentobjectid;
	int		currentmaterialid;
	objectinfo	*currentobject;
	FILE	*objfile;
	int		n,i,id,poly_id;
	int		darg[6];
	int		vertex_number;
	int		texture_vertex_number;
	int		polygon_number;
	uidlist	*cvertex;
	int		vertexuid;
	double	x,y,z;
	bool	accepted;
	char	*arg[SA_MAX_INPUTSTRING];
	int		numargs;
	bool	inword;
	polyinfo	*currentpoly;
	uidlist	*vertexlist,*texvertexlist;
	list	*temp;

	vertex_number=0;
	texture_vertex_number=0;
	polygon_number=0;

	strcpy(currentobjectname,"none"); // Make sure we can cope with OBJ files with no
									  // object names
	currentobject=NULL;
	strcpy(currentmaterial,"");

	// Initialise our lists to blank

	vertexlist=NULL;
	texvertexlist=NULL;
	objmateriallist=NULL;

	objfile=fopen(file,"r");
	if (objfile==NULL)
	{
		sprintf(o,"Unable to open obj file '%s'!",file);
		sa_error(o);
	}
	else
	{
		while (!feof(objfile))
		{
			// Read a line from the file

			l[0]='\0';	// To avoid lines with no characters acidentally including
						// the first character of the next line.

			n=-1;
			do
			{
				n++;
				fread(&l[n],sizeof(char),1,objfile);
			} while ((l[n]!=13) && (l[n]!=10) && (!feof(objfile)));

			if ((l[n]!=13) && (l[n]!=10)) // Overwrite EOL marker if present
			 n++;
			l[n]='\0'; // Ensure there's a terminator

			// Convert # signs to \0 - this makes comments a line terminator,
			// effectively stripping them.

			for (i=0;l[i]!='\0';i++)
			{
				if (l[i]=='#') l[i]='\0';
			}

			// Convert to lowercase

			for	(i=0;l[i]!='\0';i++)
			{
				l[i]=tolower(l[i]);
			}

			// Split the string into words

			numargs=0;
			inword=false;

			i=0;
			if (l[0]!='\0') // Check for null string
			{
				do
				{
					if ((l[i]==' ') || (l[i]==9) || (l[i]==','))
					{
						// Space character (space, tab or comma)
						if (inword)
						{
							inword=false;
							l[i]='\0'; // Make into word terminator
						}
					}
					else
					{
						// Alphanumeric character
						if (!inword)
						{
							inword=true;
							numargs++;
							arg[numargs]=&l[i];
						}
					}
					i++;
				} while (l[i]!='\0');
			}

			if (numargs>0)
			{
				accepted=false;

				// Get the first word (the command)

				strcpy(cmd,arg[1]);

				// ---------------------------------------------------------------
				// "mtllib" command - Load a material library
				// ---------------------------------------------------------------

				if (strcmp(cmd,"mtllib")==0)
				{
					if (numargs==2)
					{
						sa_load_mtl(name,arg[2]);
					}
					else
					{
						sa_error("Obj file error - could not get MTL filename!");
					}
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "usemtl" command - Set current material
				// ---------------------------------------------------------------

				if (strcmp(cmd,"usemtl")==0)
				{
					if (numargs==2)
					{
						strcpy(currentmaterial,arg[2]);
						temp=objmateriallist;
						currentmaterialid=0;
						while (temp!=NULL)
						{
							if (strcmp((char *)temp->data,arg[2])==0)
							{
								currentmaterialid=temp->idata;
							}
							temp=temp->next;
						}
						if (currentmaterialid==0)
						{
							sa_error("Could not find declaration of material!");
						}
						DEBUGPRINT2("Found material name '%s', retrieved id %d\n", arg[2], currentmaterialid);
					}
					else
					{
						sa_error("Obj file error - could not get USEMTL material name!");
					}
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "g" command - Start geometry of object <name>
				// ---------------------------------------------------------------

				if (strcmp(cmd,"g")==0)
				{
					if (numargs==2)
					{
						sprintf(o,"%s_%s",name,arg[2]);
						// Create the object
						currentobject=sa_addobject(o);
						// Set the object name to contain the name as a string
						sprintf(o,"%s_%s=\"%s_%s\";_sasami_object_%s_%s_change=1;\n",name,arg[2],name,arg[2],name,arg[2]);
						sa_output(o);

						currentobjectid=id;
						strcpy(currentobjectname,arg[2]);
					}
					else
					{
						sa_error("Obj file error - could not get object name!");
					}
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "v" command - add a vertex
				// ---------------------------------------------------------------

				if (strcmp(cmd,"v")==0)
				{
					if (numargs==4)
					{
						vertex_number++;
						if (nodetail)
						{
							id=sa_getvertexUID();
							x=strtodouble(arg[2]);
							y=strtodouble(arg[3]);
							z=strtodouble(arg[4]);
							sa_addvertex(id,x,y,z);
							sa_adduidlist(&vertexlist,vertex_number)->uid=id;
						}
						else
						{
							sprintf(o,"vertex %s_v_%d %s %s %s\n",name,vertex_number,arg[2],arg[3],arg[4]);
							sa_parsestring(o);
						}
					}
					else
					{
						sa_error("Obj file error - could not get vertex co-ordinates!");
					}
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "vn" command - add a vertex normal
				// ---------------------------------------------------------------

				if (strcmp(cmd,"vn")==0)
				{
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "vt" command - add a texture vertex
				// ---------------------------------------------------------------

				if (strcmp(cmd,"vt")==0)
				{
					if (numargs==4)
					{
						texture_vertex_number++;
						if (nodetail)
						{
							id=sa_getvertexUID();
							x=strtodouble(arg[2]);
							y=1-strtodouble(arg[3]);
							z=strtodouble(arg[4]);
							sa_addvertex(id,x,y,z);
							sa_adduidlist(&texvertexlist,texture_vertex_number)->uid=id;
						}
						else
						{
							sprintf(o,"vertex %s_vt_%d %s (1-%s) %s\n",name,texture_vertex_number,arg[2],arg[3],arg[4]);
							sa_parsestring(o);
						}
					}
					else
					{
						sa_error("Obj file error - could not get texture vertex co-ordinates!");
					}
					accepted=true;
				}

				// ---------------------------------------------------------------
				// "f" command - add a face
				// Faces are specified as <vertex #>/<texture vertex #>/<normal #>
				// ---------------------------------------------------------------

				if (strcmp(cmd,"f")==0)
				{
					if (numargs>1)
					{
						// Create the polygon
						polygon_number++;
						if (nodetail)
						{
							poly_id=sa_getpolyUID();
							currentpoly=sa_addpoly(poly_id);
						}
						else
						{
							sprintf(o,"polygon %s_p_%d\n",name,polygon_number);
							sa_parsestring(o);
						}

						// Create all the vertices

						for (i=2;i<=numargs;i++)
						{
							// Check if there are texture co-ordinates
							n=sscanf(arg[i]," %d/%d/%*d ",&darg[0],&darg[1]);

							if (n!=2)
							{
								// Try without them...
								n=sscanf(arg[i]," %d//%*d ",&darg[0]);
							}

							if ((n==1) || (n==2))
							{
								if (nodetail)
								{
									id=sa_getUID(); // Get a UID for this link
									// Get the vertex UID
									cvertex=sa_finduidlist(&vertexlist,darg[0]);
									if (cvertex==NULL)
									{
										sa_error("Unable to locate vertex for face!");
									}
									vertexuid=cvertex->uid;
									// Create link
									sa_adduidlist(&(currentpoly->geometry),id)->uid=vertexuid;
								}
								else
								{
									sprintf(o,"poly_geom_vertex %s_p_%d %s_v_%d\n",	name,polygon_number,
																					name,darg[0]);
									sa_parsestring(o);
								}
								if (n==2)
								{
									// We've got texture co-ordinates
									if (nodetail)
									{
										id=sa_getUID(); // Get a UID for this link
										// Get the vertex UID
										cvertex=sa_finduidlist(&texvertexlist,darg[1]);
										if (cvertex==NULL)
										{
											sa_error("Unable to locate texture vertex for face!");
										}
										vertexuid=cvertex->uid;
										// Create link
										sa_adduidlist(&(currentpoly->texcoords),id)->uid=vertexuid;
									}
									else
									{
										sprintf(o,"poly_tex_vertex %s_p_%d %s_vt_%d\n",	name,polygon_number,
																						name,darg[1]);
										sa_parsestring(o);
									}
								}
							}
						}

						// Set the polygon material to the current one (if present)

						if (strlen(currentmaterial)>0)
						{
							if (nodetail)
							{
								currentpoly->material=currentmaterialid;
							}
							else
							{
								sprintf(o,"poly_material %s_p_%d %s_mat_%s\n",name,polygon_number,name,currentmaterial);
								sa_parsestring(o);
							}
						}

						// Add this polygon to the current object
						if (nodetail)
						{
							if (currentobject==NULL)
							{
								sa_error("Obj file error - polygon data with no object!");
							}
							id=sa_getUID(); // Get a UID for this link
							// Create link
							sa_adduidlist(&(currentobject->polys),id)->uid=poly_id;
						}
						else
						{
							sprintf(o,"object_poly %s_%s %s_p_%d\n",name,currentobjectname,
																  name,polygon_number);
							sa_parsestring(o);
						}
					}
					else
					{
						sa_error("Obj file error - could not get polygon data!");
					}
					accepted=true;
				}

				// ---------------------------------------------------------------
				// End of Lightwave OBJ command list
				// ---------------------------------------------------------------

				if (!accepted)
				{
					sprintf(o,"Obj file error - unknown command '%s'",l);
					sa_error(o);
				}
			}
		}
		fclose(objfile);
	}
}

// --------------------------------------------------------------------------
// This adds escape characters to a string where needed.
// --------------------------------------------------------------------------

void sa_addescapes(char *s)
{
	char	*c;
	char	*out;
	char	*o;

	out=(char *)malloc(strlen(s)+30); // Add a bit to cope with the extra characters

	o=out;
	c=s;
	while (*c!='\0')
	{
		if (*c=='\\')
		{
			*o='\\';
			o++;
			*o=*c;
			o++;
		}
		else
		{
			*o=*c;
			o++;
		}
		c++;
	}

	*o='\0';

	strcpy(s,out);

	free(out);
}

// --------------------------------------------------------------------------
// This takes individual strings and parses them - they may be passed from
// the EDEN input system (via sa_input), or from functions in here, such
// as the Lightwave object reader (sa_load_obj).
// --------------------------------------------------------------------------

void sa_parsestring(char *input)
{
	char	s[SA_MAX_INPUTSTRING+30];
	char	scopy[SA_MAX_INPUTSTRING+30];
	char	o[SA_MAX_INPUTSTRING+30];
	char	cmd[SA_MAX_INPUTSTRING+30];
	int		i,n,id;
	bool	accepted;
	char	*arg[SA_MAX_INPUTSTRING];
	char	sarg[2][SA_MAX_INPUTSTRING];
	int		numargs;
	bool	inword;

	// Check to see if this is the first time the parser has been called,
	// and if so set up our "global" Eden definations

	if (!sa_eden_initialised) sa_eden_init();

	strcpy(s,input);

	// Convert # signs to \0 - this makes comments a line terminator,
	// effectively stripping them.

	for (i=0;s[i]!='\0';i++)
	{
		if (s[i]=='#') s[i]='\0';
	}

	// First check to see if this string starts with a "`" - if so then
	// it's an EDEN command and should be simply passed on "as is"

	if (s[0]=='`')
	{
		// It's an EDEN command - send all but the first character to EDEN
		sa_output(&s[1]);
		// Blank the input string to prevent further processing
		strcpy(s,"");
	}

	// Make a backup copy of the input string in case it's needed (eg for parsing
	// quoted filenames)

	strcpy(scopy,s);

	// Split the string into words

	numargs=0;
	inword=false;

	i=0;
	if (s[0]!='\0') // Check for null string
	{
		do
		{
			if ((s[i]==' ') || (s[i]==9) || (s[i]==','))
			{
				// Space character (space, tab or comma)
				if (inword)
				{
					inword=false;
					s[i]='\0'; // Make into word terminator
				}
			}
			else
			{
				// Alphanumeric character
				if (!inword)
				{
					inword=true;
					numargs++;
					arg[numargs]=&s[i];
				}
			}
			i++;
		} while (s[i]!='\0');
	}

	if (numargs>0) // Check that we actually *got* something!
	{
		// Extract the command word (the first word of the string)

		strcpy(cmd,arg[1]);

		// Convert command to lowercase

		for	(i=0;cmd[i]!='\0';i++)
		{
			cmd[i]=tolower(cmd[i]);
		}

		// Now check for the various accepted commands

		accepted=false;

		// ------------------------------------------------------------------
		// vertex <name> <x> <y> [z]	Create a vertex using X, Y and Z
		//				 		Vertices may or may not be part of a polygon
		// ------------------------------------------------------------------

		if (strcmp(cmd,"vertex")==0)
		{
			if ((numargs==4) || (numargs==5))
			{
				id=sa_getvertexUID(); // Create a new object ID
				// Create the vertex (at 0,0,0, initially, until the settings below get evaluated)
				sa_addvertex(id,0,0,0);
				// First set the vertex name to contain the ID for future reference
				sprintf(o,"%s=%d;\n",arg[2],id);
				sa_output(o);
				// Create variables for the parameters given
				sprintf(o,"_sasami_vertex_%d_x is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_vertex_%d_y is %s;\n",id,arg[4]);
				sa_output(o);
				if (numargs==5)
				{
					// All three were specified
					sprintf(o,"_sasami_vertex_%d_z is %s;\n",id,arg[5]);
				}
				else
				{
					// Only X and Y were specified
					sprintf(o,"_sasami_vertex_%d_z=0;\n",id);
				}
				sa_output(o);
				// Then create a monitor procedure to watch those variables
				sprintf(o,"proc _sasami_vertex_mon_%d : _sasami_vertex_%d_x,_sasami_vertex_%d_y,_sasami_vertex_%d_z { sasami_vertex(%d,_sasami_vertex_%d_x,_sasami_vertex_%d_y,_sasami_vertex_%d_z); };\n",id,id,id,id,id,id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : vertex <name> <x> <y> [z]");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		//  material <name> 	Create a new material
		// ------------------------------------------------------------------

		// Note : Be careful when modifying this code - there is a duplicate
		//        of it up in the OBJ file material library loader that needs
		//        to be changed too.

		if (strcmp(cmd,"material")==0)
		{
			if (numargs==2)
			{
				id=sa_getUID(); // Create a new ID
				// Create the material
				sa_addmaterial(id);
				// First set the material name to contain the ID for future reference
				sprintf(o,"%s=%d;\n",arg[2],id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : material <name>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// material_ambient	<name> <r> <g> <b> [a] Sets the ambient colour
		//											of material <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"material_ambient")==0)
		{
			if ((numargs==5) | (numargs==6))
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create four variables to hold the arguments
				sprintf(o,"_sasami_material_%d_ambient_r is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_material_%d_ambient_g is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_material_%d_ambient_b is %s;\n",id,arg[5]);
				sa_output(o);
				if (numargs==6)
				{
					sprintf(o,"_sasami_material_%d_ambient_a is %s;\n",id,arg[6]);
				}
				else
				{
					// Alpha not specified - assume 1
					sprintf(o,"_sasami_material_%d_ambient_a=1;\n",id);
				}
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_material_%d_ambient_mon : _sasami_material_%d_ambient_r,_sasami_material_%d_ambient_g,_sasami_material_%d_ambient_b,_sasami_material_%d_ambient_a { sasami_material_ambient(%s,_sasami_material_%d_ambient_r,_sasami_material_%d_ambient_g,_sasami_material_%d_ambient_b,_sasami_material_%d_ambient_a); };\n",
					    id,id,id,id,id,arg[2],id,id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : material_ambient <material name> <r> <g> <b> [a]");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// material_diffuse	<name> <r> <g> <b> [a]	Sets the diffuse colour
		//											of material <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"material_diffuse")==0)
		{
			if ((numargs==5) | (numargs==6))
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create four variables to hold the arguments
				sprintf(o,"_sasami_material_%d_diffuse_r is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_material_%d_diffuse_g is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_material_%d_diffuse_b is %s;\n",id,arg[5]);
				sa_output(o);
				if (numargs==6)
				{
					sprintf(o,"_sasami_material_%d_diffuse_a is %s;\n",id,arg[6]);
				}
				else
				{
					sprintf(o,"_sasami_material_%d_diffuse_a=1;\n",id);
				}
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_material_%d_diffuse_mon : _sasami_material_%d_diffuse_r,_sasami_material_%d_diffuse_g,_sasami_material_%d_diffuse_b,_sasami_material_%d_diffuse_a { sasami_material_diffuse(%s,_sasami_material_%d_diffuse_r,_sasami_material_%d_diffuse_g,_sasami_material_%d_diffuse_b,_sasami_material_%d_diffuse_a); };\n",
					    id,id,id,id,id,arg[2],id,id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : material_diffuse <material name> <r> <g> <b> [a]");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// material_specular <name> <r> <g> <b> [a]	Sets the ambient colour
		//											of material <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"material_specular")==0)
		{
			if ((numargs==5) | (numargs==6))
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create four variables to hold the arguments
				sprintf(o,"_sasami_material_%d_specular_r is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_material_%d_specular_g is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_material_%d_specular_b is %s;\n",id,arg[5]);
				sa_output(o);
				if (numargs==6)
				{
					sprintf(o,"_sasami_material_%d_specular_a is %s;\n",id,arg[6]);
				}
				else
				{
					sprintf(o,"_sasami_material_%d_specular_a=1;\n",id);
				}
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_material_%d_specular_mon : _sasami_material_%d_specular_r,_sasami_material_%d_specular_g,_sasami_material_%d_specular_b,_sasami_material_%d_specular_a { sasami_material_specular(%s,_sasami_material_%d_specular_r,_sasami_material_%d_specular_g,_sasami_material_%d_specular_b,_sasami_material_%d_specular_a); };\n",
					    id,id,id,id,id,arg[2],id,id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : material_specular <material name> <r> <g> <b> [a]");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// material_texture <name> "<filename>"	Sets the texture file
		//										of material <name>
		// ------------------------------------------------------------------

		// Note : the quotes exist in order to make it possible to specify
		//        filenames containing spaces directly

		// The parser code here is a bit different from the rest, as it needs
		// to deal with quoted strings containing spaces

		if (strcmp(cmd,"material_texture")==0)
		{
			// First check to see if it's a quoted string
			// This uses scopy as spaces in filenames may have caused word splits in
			// the arg[] list.
			n=sscanf(scopy,"%*s %s \"%[^\"]",sarg[0],sarg[1]);
			if (n==2)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Make sure all special characters are escaped
				sa_addescapes(sarg[1]);
				// Create a variable to hold the argument
				sprintf(o,"_sasami_material_%d_texture is \"%s\";\n",id,sarg[1]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_material_%d_texture_mon : _sasami_material_%d_texture { sasami_material_texture(%s,_sasami_material_%d_texture); };\n",
					    id,id,sarg[0],id);
				sa_output(o);
			}
			else
			{
				// OK - Maybe it's a variable reference

				n=sscanf(scopy,"%*s %s %s",sarg[0],sarg[1]);
				if (n==2)
				{
					// Create a UID for this link
					id=sa_getUID();
					// Create a variable to hold the argument
					sprintf(o,"_sasami_material_%d_texture is %s;\n",id,sarg[1]);
					sa_output(o);
					// Then create a monitor procedure to watch the variables
					sprintf(o,"proc _sasami_material_%d_texture_mon : _sasami_material_%d_texture { sasami_material_texture(%s,_sasami_material_%d_texture); };\n",
							id,id,sarg[0],id);
					sa_output(o);
				}
				else
				{
					sa_error("Syntax : material_texture <material name> (\"<filename>\" or <filename variable>)");
				}
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// load_obj <object name> "<filename>"	Creates an object <name>,
		//										using data from the specified
		//										lightwave OBJ file.
		//
		// This version does not create Eden declarations for any of the
		// vertices or polygons in the object file, to avoid speed problems.
		// Use load_full_obj to load and create declarations for an entire
		// object.
		// ------------------------------------------------------------------

		// Note : the quotes exist in order to make it possible to specify
		//        filenames containing spaces directly

		if (strcmp(cmd,"load_obj")==0)
		{
			// First check to see if it's a quoted string
			// This uses scopy as spaces in filenames may have caused word splits in
			// the arg[] list.
			n=sscanf(scopy,"%*s %s \"%[^\"]",sarg[0],sarg[1]);
			if (n==2)
			{
				// Load the object file
				sa_load_obj(sarg[0],sarg[1],true);
			}
			else
			{
				sa_error("Syntax : load_obj <object name> \"<filename>\"");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// load_full_obj <object name> "<filename>"	Creates an object <name>,
		//										using data from the specified
		//										lightwave OBJ file.
		//
		// This version creates Eden declarations for all of the
		// vertices or polygons in the object file, which is *SLOW* for
		// large objects.
		// Use load_obj instead of this if you are not going to manipulate
		// vertices/polys directly.
		// ------------------------------------------------------------------

		// Note : the quotes exist in order to make it possible to specify
		//        filenames containing spaces directly

		if (strcmp(cmd,"load_full_obj")==0)
		{
			// First check to see if it's a quoted string
			// This uses scopy as spaces in filenames may have caused word splits in
			// the arg[] list.
			n=sscanf(scopy,"%*s %s \"%[^\"]",sarg[0],sarg[1]);
			if (n==2)
			{
				// Load the object file
				sa_load_obj(sarg[0],sarg[1],false);
			}
			else
			{
				sa_error("Syntax : load_full_obj <object name> \"<filename>\"");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// polygon <names>				Create polygons with names <names>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"polygon")==0)
		{
			if (numargs>=2)
			{
				for (i=2;i<=numargs;i++)
				{
					// Create an ID for our new poly
					id=sa_getpolyUID();
					// Create the polygon
					sa_addpoly(id);
					// Set the poly name to contain the ID
					sprintf(o,"%s=%d;\n",arg[i],id);
					sa_output(o);
				}
				accepted=true;
			}
			else
			{
				sa_error("Syntax : polygon <poly names>");
			}
		}

		// ------------------------------------------------------------------
		// poly_geom_vertex	<name> <id>...	Add vertices <id>... to a polygon
		//									<name>'s geometry
		// ------------------------------------------------------------------

		if (strcmp(cmd,"poly_geom_vertex")==0)
		{
			if (numargs>=3)
			{
				for (i=3;i<=numargs;i++) // Repeat for every vertex
				{
					// Create a UID for this link
					id=sa_getUID();
					// First set up a variable to hold this...
					sprintf(o,"_sasami_poly_geom_vertex_%d is %s;\n",id,arg[i]);
					sa_output(o);
					// ...then a monitor to watch it
					sprintf(o,"proc _sasami_poly_geom_vertex_%d_mon : _sasami_poly_geom_vertex_%d { sasami_poly_geom_vertex(%s,%d,_sasami_poly_geom_vertex_%d); };\n",
							id,id,arg[2],id,id);
					sa_output(o);
				}
			}
			else
			{
				sa_error("Syntax : poly_geom_vertex <poly name> <vertex names>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// poly_tex_vertex	<name> <ids>...	Add vertices <ids> to a polygon
		//									<name>'s texture coordinates
		// ------------------------------------------------------------------

		if (strcmp(cmd,"poly_tex_vertex")==0)
		{
			if (numargs>=3)
			{
				for (i=3;i<=numargs;i++) // Repeat for every vertex
				{
					// Create a UID for this link
					id=sa_getUID();
					// First set up a variable to hold this...
					sprintf(o,"_sasami_poly_tex_vertex_%d is %s;\n",id,arg[i]);
					sa_output(o);
					// ...then a monitor to watch it
					sprintf(o,"proc _sasami_poly_tex_vertex_%d_mon : _sasami_poly_tex_vertex_%d { sasami_poly_tex_vertex(%s,%d,_sasami_poly_tex_vertex_%d); };\n",
							id,id,arg[2],id,id);
					sa_output(o);
				}
			}
			else
			{
				sa_error("Syntax : poly_tex_vertex <poly name> <vertex names>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// poly_colour	<name> <r> <g> <b> [a]		Sets the colour of poly
		//											<name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"poly_colour")==0)
		{
			if ((numargs==5) || (numargs==6))
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create four variables to hold the arguments
				sprintf(o,"_sasami_poly_%d_colour_r is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_poly_%d_colour_g is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_poly_%d_colour_b is %s;\n",id,arg[5]);
				sa_output(o);
				if (numargs==6)
				{
					sprintf(o,"_sasami_poly_%d_colour_a is %s;\n",id,arg[6]);
				}
				else
				{
					// Alpha is set to 1 if not specified
					sprintf(o,"_sasami_poly_%d_colour_a=1;\n",id);
				}
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_poly_%d_colour_mon : _sasami_poly_%d_colour_r,_sasami_poly_%d_colour_g,_sasami_poly_%d_colour_b,_sasami_poly_%d_colour_a { sasami_poly_colour(%s,_sasami_poly_%d_colour_r,_sasami_poly_%d_colour_g,_sasami_poly_%d_colour_b,_sasami_poly_%d_colour_a); };\n",
					    id,id,id,id,id,arg[2],id,id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : poly_colour <poly name> <r> <g> <b> [a]");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// poly_material	<name> <n>			Sets polygon <names> material
		//										to <n>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"poly_material")==0)
		{
			if (numargs==3)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create a variable to hold this
				sprintf(o,"_sasami_poly_%d_material is %s;\n",id,arg[3]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_poly_%d_material_mon : _sasami_poly_%d_material { sasami_poly_material(%s,_sasami_poly_%d_material); };\n",
					    id,id,arg[2],id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : poly_material <poly> <material>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// object <names>				Create objects with names <names>
		// ------------------------------------------------------------------

		// Note : There is a copy of this code in the OBJ file loader above -
		//        if modifing this, then remember to change it up there too!

		if (strcmp(cmd,"object")==0)
		{
			if (numargs>=2)
			{
				for (i=2;i<=numargs;i++)
				{
					// Create the object
					sa_addobject(arg[i]);
					// Set the object name to contain the ID
					sprintf(o,"%s=\"%s\";\n",arg[i],arg[i],arg[i]);
					sa_output(o);
				}
				accepted=true;
			}
			else
			{
				sa_error("Syntax : object <object names>");
			}
		}

		// ------------------------------------------------------------------
		// object_poly <name> <ids> 		Add polygons <ids> to object <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"object_poly")==0)
		{
			if (numargs>=3)
			{
				for (i=3;i<=numargs;i++) // Repeat for every polygon
				{
  	  				// Create a UID for this link
					id=sa_getUID();
					// First set up a variable to hold this...
					sprintf(o,"_sasami_object_poly_%d is %s;\n",id,arg[i]);
					sa_output(o);
					// ...then a monitor to watch it
					sprintf(o,"proc _sasami_object_poly_%d_mon : _sasami_object_poly_%d { sasami_object_poly(%s,%d,_sasami_object_poly_%d); };\n",
							id,id,arg[2],id,id);
					sa_output(o);
				}
			}
			else
			{
				sa_error("Syntax : object_poly <object name> <poly names>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// object_pos	<name> <x> <y> <z>		Sets the position of
		//						object <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"object_pos")==0)
		{
			if (numargs==5)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create three variables to hold the arguments
				sprintf(o,"_sasami_object_%d_pos_x is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_object_%d_pos_y is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_object_%d_pos_z is %s;\n",id,arg[5]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_object_%d_pos_mon : _sasami_object_%d_pos_x,_sasami_object_%d_pos_y,_sasami_object_%d_pos_z { sasami_object_pos(%s,_sasami_object_%d_pos_x,_sasami_object_%d_pos_y,_sasami_object_%d_pos_z); };\n",
					    id,id,id,id,arg[2],id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : object_pos <object name> <x> <y> <z>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// object_rot	<name> <x> <y> <z>			Sets the rotation of
		//											object <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"object_rot")==0)
		{
			if (numargs==5)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create three variables to hold the arguments
				sprintf(o,"_sasami_object_%d_rot_x is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_object_%d_rot_y is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_object_%d_rot_z is %s;\n",id,arg[5]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_object_%d_rot_mon : _sasami_object_%d_rot_x,_sasami_object_%d_rot_y,_sasami_object_%d_rot_z { sasami_object_rot(%s,_sasami_object_%d_rot_x,_sasami_object_%d_rot_y,_sasami_object_%d_rot_z); };\n",
					    id,id,id,id,arg[2],id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : object_rot <object name> <x> <y> <z>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// object_scale	<name> <x> <y> <z>			Sets the scale of
		//											object <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"object_scale")==0)
		{
			if (numargs==5)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create three variables to hold the arguments
				sprintf(o,"_sasami_object_%d_scale_x is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_object_%d_scale_y is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_object_%d_scale_z is %s;\n",id,arg[5]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_object_%d_scale_mon : _sasami_object_%d_scale_x,_sasami_object_%d_scale_y,_sasami_object_%d_scale_z { sasami_object_scale(%s,_sasami_object_%d_scale_x,_sasami_object_%d_scale_y,_sasami_object_%d_scale_z); };\n",
					    id,id,id,id,arg[2],id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : object_scale <object name> <x> <y> <z>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// object_visible <name> <e>	Makes <name> visible (e=1 -> visible
		//													  e=0 -> invisible)
		// ------------------------------------------------------------------

		if (strcmp(cmd,"object_visible")==0)
		{
			if (numargs==3)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create a variable to hold the argument
				sprintf(o,"_sasami_object_%d_visible is %s;\n",id,arg[3]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_object_%d_visible_mon : _sasami_object_%d_visible { sasami_object_visible(%s,_sasami_object_%d_visible); };\n",
					    id,id,arg[2],id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : object_visible <object name> <e> (e=0 to make invisible, 1 to make visible) ");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// light <names>				Create lights with names <names>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"light")==0)
		{
			if (numargs>=2)
			{
				for (i=2;i<=numargs;i++)
				{
					// Create an ID for our new light
					id=sa_getUID();
					// Create the light
					sa_addlight(id);
					// Set the light name to contain the ID
					sprintf(o,"%s=%d;\n",arg[i],id);
					sa_output(o);
				}
				accepted=true;
			}
			else
			{
				sa_error("Syntax : light <light names>");
			}
		}

		// ------------------------------------------------------------------
		// light_pos	<name> <x> <y> <z>			Sets the position of
		//											light <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"light_pos")==0)
		{
			if (numargs==5)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create three variables to hold the arguments
				sprintf(o,"_sasami_light_%d_pos_x is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_light_%d_pos_y is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_light_%d_pos_z is %s;\n",id,arg[5]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_light_%d_pos_mon : _sasami_light_%d_pos_x,_sasami_light_%d_pos_y,_sasami_light_%d_pos_z { sasami_light_pos(%s,_sasami_light_%d_pos_x,_sasami_light_%d_pos_y,_sasami_light_%d_pos_z); };\n",
					    id,id,id,id,arg[2],id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : light_pos <light name> <x> <y> <z>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// light_ambient	<name> <r> <g> <b> [a] Sets the ambient colour
		//											of light <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"light_ambient")==0)
		{
			if ((numargs==5) | (numargs==6))
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create four variables to hold the arguments
				sprintf(o,"_sasami_light_%d_ambient_r is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_light_%d_ambient_g is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_light_%d_ambient_b is %s;\n",id,arg[5]);
				sa_output(o);
				if (numargs==6)
				{
					sprintf(o,"_sasami_light_%d_ambient_a is %s;\n",id,arg[6]);
				}
				else
				{
					// Alpha not specified - assume 1
					sprintf(o,"_sasami_light_%d_ambient_a=1;\n",id);
				}
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_light_%d_ambient_mon : _sasami_light_%d_ambient_r,_sasami_light_%d_ambient_g,_sasami_light_%d_ambient_b,_sasami_light_%d_ambient_a { sasami_light_ambient(%s,_sasami_light_%d_ambient_r,_sasami_light_%d_ambient_g,_sasami_light_%d_ambient_b,_sasami_light_%d_ambient_a); };\n",
					    id,id,id,id,id,arg[2],id,id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : light_ambient <light name> <r> <g> <b> [a]");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// light_diffuse	<name> <r> <g> <b> [a]	Sets the diffuse colour
		//											of light <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"light_diffuse")==0)
		{
			if ((numargs==5) | (numargs==6))
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create four variables to hold the arguments
				sprintf(o,"_sasami_light_%d_diffuse_r is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_light_%d_diffuse_g is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_light_%d_diffuse_b is %s;\n",id,arg[5]);
				sa_output(o);
				if (numargs==6)
				{
					sprintf(o,"_sasami_light_%d_diffuse_a is %s;\n",id,arg[6]);
				}
				else
				{
					sprintf(o,"_sasami_light_%d_diffuse_a=1;\n",id);
				}
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_light_%d_diffuse_mon : _sasami_light_%d_diffuse_r,_sasami_light_%d_diffuse_g,_sasami_light_%d_diffuse_b,_sasami_light_%d_diffuse_a { sasami_light_diffuse(%s,_sasami_light_%d_diffuse_r,_sasami_light_%d_diffuse_g,_sasami_light_%d_diffuse_b,_sasami_light_%d_diffuse_a); };\n",
					    id,id,id,id,id,arg[2],id,id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : light_diffuse <light name> <r> <g> <b> [a]");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// light_specular <name> <r> <g> <b> [a]	Sets the ambient colour
		//											of light <name>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"light_specular")==0)
		{
			if ((numargs==5) | (numargs==6))
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create four variables to hold the arguments
				sprintf(o,"_sasami_light_%d_specular_r is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_light_%d_specular_g is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_light_%d_specular_b is %s;\n",id,arg[5]);
				sa_output(o);
				if (numargs==6)
				{
					sprintf(o,"_sasami_light_%d_specular_a is %s;\n",id,arg[6]);
				}
				else
				{
					sprintf(o,"_sasami_light_%d_specular_a=1;\n",id);
				}
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_light_%d_specular_mon : _sasami_light_%d_specular_r,_sasami_light_%d_specular_g,_sasami_light_%d_specular_b,_sasami_light_%d_specular_a { sasami_light_specular(%s,_sasami_light_%d_specular_r,_sasami_light_%d_specular_g,_sasami_light_%d_specular_b,_sasami_light_%d_specular_a); };\n",
					    id,id,id,id,id,arg[2],id,id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : light_specular <light name> <r> <g> <b> [a]");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// light_enabled <name> <e>		Enables light <name> (e=1 -> enable
		//													  e=0 -> disable)
		// ------------------------------------------------------------------

		if (strcmp(cmd,"light_enabled")==0)
		{
			if (numargs==3)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create a variable to hold the argument
				sprintf(o,"_sasami_light_%d_enabled is %s;\n",id,arg[3]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_light_%d_enabled_mon : _sasami_light_%d_enabled { sasami_light_enabled(%s,_sasami_light_%d_enabled); };\n",
					    id,id,arg[2],id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : light_enabled <light name> <e> (e=0 to disable, 1 to enable) ");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// light_directional <name> <d>		Sets light <name> as either :
		//									0 = Positional
		//									1 = Directional
		// ------------------------------------------------------------------

		if (strcmp(cmd,"light_directional")==0)
		{
			if (numargs==3)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create a variable to hold the argument
				sprintf(o,"_sasami_light_%d_directional is %s;\n",id,arg[3]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_light_%d_directional_mon : _sasami_light_%d_directional { sasami_light_directional(%s,_sasami_light_%d_directional); };\n",
					    id,id,arg[2],id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : light_directional <light name> <d> (d=0 for positional, 1 for directional) ");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// light_attenuation <name> <a>		Sets light <name>'s attenuation
		//									factor to <a>
		// ------------------------------------------------------------------

		if (strcmp(cmd,"light_attenuation")==0)
		{
			if (numargs==3)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create a variable to hold the argument
				sprintf(o,"_sasami_light_%d_attenuation is %s;\n",id,arg[3]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_light_%d_attenuation_mon : _sasami_light_%d_attenuation { sasami_light_attenuation(%s,_sasami_light_%d_attenuation); };\n",
					    id,id,arg[2],id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : light_attenuation <light name> <attenuation factor>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// dump_vertices		Primarily a debugging call - this causes the
		//						current list of vertices to be sent to STDOUT
		// ------------------------------------------------------------------

		if (strcmp(cmd,"dump_vertices")==0)
		{
			sa_dumpvertices();
			accepted=true;
		}

		// ------------------------------------------------------------------
		// dump_polys			Primarily a debugging call - this causes the
		//						current list of polys to be sent to STDOUT
		// ------------------------------------------------------------------

		if (strcmp(cmd,"dump_polys")==0)
		{
			sa_dumppolys();
			accepted=true;
		}

		// ------------------------------------------------------------------
		// dump_objects			Primarily a debugging call - this causes the
		//						current list of objects to be sent to STDOUT
		// ------------------------------------------------------------------

		if (strcmp(cmd,"dump_objects")==0)
		{
			sa_dumpobjects();
			accepted=true;
		}

		// ------------------------------------------------------------------
		// dump_materials		Primarily a debugging call - this causes the
		//						current materials list to be sent to STDOUT
		// ------------------------------------------------------------------

		if (strcmp(cmd,"dump_materials")==0)
		{
			sa_dumpmaterials();
			accepted=true;
		}

		// ------------------------------------------------------------------
		// dump_lights			Primarily a debugging call - this causes the
		//						current light list to be sent to STDOUT
		// ------------------------------------------------------------------

		if (strcmp(cmd,"dump_lights")==0)
		{
			sa_dumplights();
			accepted=true;
		}

		// ------------------------------------------------------------------
		// open_display			Initialises the Sasami OpenGL display
		// ------------------------------------------------------------------

		if (strcmp(cmd,"open_display")==0)
		{
			sa_r_opendisplay();
			accepted=true;
		}

		// ------------------------------------------------------------------
		// close_display		Shuts down the Sasami OpenGL display
		// ------------------------------------------------------------------

		if (strcmp(cmd,"close_display")==0)
		{
			sa_r_closedisplay();
			accepted=true;
		}

		// ------------------------------------------------------------------
		// bgcolour	<r> <g> <b>			Sets the viewport background colour
		// This is just a helper function - the same effect can be achieved
		// by setting the sasami_bgcolour_<x> variables
		// ------------------------------------------------------------------

		if (strcmp(cmd,"bgcolour")==0)
		{
			if (numargs==4)
			{
				// Set the three colour variables
				sprintf(o,"sasami_bgcolour_r is %s;\n",arg[2]);
				sa_output(o);
				sprintf(o,"sasami_bgcolour_g is %s;\n",arg[3]);
				sa_output(o);
				sprintf(o,"sasami_bgcolour_b is %s;\n",arg[4]);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : set_bgcolour <r> <g> <b>");
			}
			accepted=true;
		}

		// ------------------------------------------------------------------
		// viewport	<x> <y>				Sets the viewport size to <x>x<y>
		// This is just a helper function - the same effect can be achieved
		// by setting the sasami_viewport_<x> variables
		// ------------------------------------------------------------------

		if (strcmp(cmd,"viewport")==0)
		{
			if (numargs==3)
			{
				// Set the three colour variables
				sprintf(o,"sasami_viewport_xsize is %s;\n",arg[2]);
				sa_output(o);
				sprintf(o,"sasami_viewport_ysize is %s;\n",arg[3]);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : viewport <x size> <y size>");
			}
			accepted=true;
		}
		
		// ------------------------------------------------------------------
		// object_material	<object name> <material>	Sets the material of all
		// 							polys in <object name> to <material>
		// ------------------------------------------------------------------
		
		if (strcmp(cmd,"primitive_material")==0)
		{
			if (numargs==3)
			{
				// Create a UID for this link
				id=sa_getUID();
				// Create a variable to hold the argument
				sprintf(o,"_sasami_primitive_%d_mat is %s;\n",id,arg[3]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_primitive_%d_mat_mon : _sasami_primitive_%d_mat { sasami_primitive_material(%s,_sasami_primitive_%d_mat); };\n",
					    id,id,arg[2],id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : primitive_material <primitive name> <material>");
			}
			accepted=true;
		}
		
		// ------------------------------------------------------------------
		// object_delete	<object name>	Deletes object <object name>, including 
		// 					its polys and vertices
		// ------------------------------------------------------------------
		
		if (strcmp(cmd,"object_delete")==0)
		{
			if (numargs==2)
			{
				sprintf(o,"sasami_object_delete(%s);",arg[2]);
				sa_output(o);
				sprintf(o,"%s is @;\n",arg[2]);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : object_delete <object name>");
			}
			accepted=true;
		}
		
		// ------------------------------------------------------------------
		// face_material	<face name> <face number> <material>	Sets the material of one
		//								face of an object
		// ------------------------------------------------------------------
		
		if (strcmp(cmd,"face_material")==0)
		{
			if (numargs==4)
			{
				id = sa_getUID();
				// Create a variable to hold the argument
				sprintf(o,"_sasami_face_%d_mat is %s;\n",id,arg[4]);
				sa_output(o);
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_face_%d_mat_mon : _sasami_face_%d_mat { sasami_face_material(%s,%s,_sasami_face_%d_mat); };\n",
					    id,id,arg[2],arg[3],id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : face_material <object name> <face number> <material>");
			}
			accepted=true;
		}
		
		// ------------------------------------------------------------------
		// cube	<cube name> <width> <height> <depth>	Creates a cube
		// ------------------------------------------------------------------
		
		if (strcmp(cmd,"cube")==0)
		{
			if (numargs==5)
			{
				// Set the cube name variable to its name as a string
				sprintf(o,"%s=\"%s\";\n",arg[2],arg[2]);
				sa_output(o);
				// Create a UID for this link
				id=sa_getUID();
				// Create four variables to hold the arguments
				sprintf(o,"_sasami_cube_%d_w is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_cube_%d_h is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_cube_%d_d is %s;\n",id,arg[5]);
				sa_output(o);
				
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_cube_%d_mon : _sasami_cube_%d_w,_sasami_cube_%d_h,_sasami_cube_%d_d { sasami_cube(%s,_sasami_cube_%d_w,_sasami_cube_%d_h,_sasami_cube_%d_d); };\n",
					    id,id,id,id,arg[2],id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : cube <cube name> <width> <height> <depth>");
			}
			accepted=true;
		}
		
		// ------------------------------------------------------------------
		// cylinder	<cylinder name> <length> <radius1> <radius2> <segments>	
		//					Creates a cylinder/cone
		// ------------------------------------------------------------------
		
		if (strcmp(cmd,"cylinder")==0)
		{
			if (numargs==6)
			{
				// Set the cylinder name variable to its name as a string
				sprintf(o,"%s=\"%s\";\n",arg[2],arg[2]);
				sa_output(o);
				// Create a UID for this link
				id=sa_getUID();
				// Create four variables to hold the arguments
				sprintf(o,"_sasami_cyl_%d_l is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_cyl_%d_r1 is %s;\n",id,arg[4]);
				sa_output(o);
				sprintf(o,"_sasami_cyl_%d_r2 is %s;\n",id,arg[5]);
				sa_output(o);
				sprintf(o,"_sasami_cyl_%d_s is %s;\n",id,arg[6]);
				sa_output(o);
				
				// Then create a monitor procedure to watch the variables
				sprintf(o,"proc _sasami_cyl_%d_mon : _sasami_cyl_%d_l,_sasami_cyl_%d_r1,_sasami_cyl_%d_r2,_sasami_cyl_%d_s { sasami_cylinder(%s,_sasami_cyl_%d_l,_sasami_cyl_%d_r1,_sasami_cyl_%d_r2,_sasami_cyl_%d_s); };\n",
					    id,id,id,id,id,arg[2],id,id,id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : cylinder <cube name> <length> <radius1> <radius2> <segments>");
			}
			accepted=true;
		}
		
		// ------------------------------------------------------------------
		// sphere	<sphere name> <radius> <segments>	Creates a sphere
		// ------------------------------------------------------------------
		
		if (strcmp(cmd,"sphere")==0)
		{
			if (numargs==4)
			{
				// Set the sphere name variable to its name as a string
				sprintf(o,"%s=\"%s\";\n",arg[2],arg[2]);
				sa_output(o);
				// Create a UID for this link
				id=sa_getUID();
				// Create variables to hold the arguments
				sprintf(o,"_sasami_sphere_%d_r is %s;\n",id,arg[3]);
				sa_output(o);
				sprintf(o,"_sasami_sphere_%d_s is %s;\n",id,arg[4]);
				sa_output(o);
				
				// Then create a monitor procedure to watch the variables
				// The object's components are deleted and re-created, so the object
				// change monitor variable is touched to update transforms, materials etc.
				sprintf(o,"proc _sasami_sphere_%d_mon : _sasami_sphere_%d_r,_sasami_sphere_%d_s { sasami_sphere(%s,_sasami_sphere_%d_r,_sasami_sphere_%d_s); };\n",
					    id,id,id,arg[2],id,id);
				sa_output(o);
			}
			else
			{
				sa_error("Syntax : sphere <sphere name> <radius> <segments>");
			}
			accepted=true;
		}

		// Check that the command was processed OK

		if (!accepted)
		{
			// Raise an error
			sa_error("Unknown command");
		}
	}
	else
	{
		// If we got here, we didn't find a command word, implying that the
		// input was a null string. It would be silly to flag an error, so
		// we'll let it pass...
	}
}

#endif /* WANT_SASAMI */
