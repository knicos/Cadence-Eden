/*
 * $Id: input_device.h,v 1.5 2001/07/27 17:34:19 cssbz Exp $
 *
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

struct input_device {		/* Input device */
  char       *name;		/* File name */
  short       type;		/* Device type: file / string */
  char       *ptr;		/* File-ptr / char-ptr */

  char       *sptr;		/* Pointer to buffer for strings */

  char        newline;		/* Newline read? */
  int         lineno;		/* Line number */
  int         lastc;		/* The forward char of last device */

  char        *linebuf;		/* Buffer to hold the current line (for error
				   messages) [Ash] */
  int         linebufsize;	/* Currently allocated size of linebuf
                                   [Ash] */
  int         charno;		/* Index of last read character in
                                   linebuf (for error messages) [Ash] */
  int         linebufend;	/* Index of last written character in
                                   linebuf (which might not be charno
                                   after flushRestOfLine) [Ash] */

#ifdef TTYEDEN
  int         usereadline;	/* whether to use readline or just
                                   simple getc [Ash] */
#endif

  Frame      *frame;		/* Frame */
  jmp_buf     begin;		/* Resume point when error occurred */
}           Input_Devices[16];  /* 15 levels of input */

extern struct input_device *Inp_Dev;
