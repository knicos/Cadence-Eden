/*
 * $Id: curses.c,v 1.7 2001/07/27 16:42:42 cssbz Exp $
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

static char rcsid[] = "$Id: curses.c,v 1.7 2001/07/27 16:42:42 cssbz Exp $";

/*********************************************************
 * 	MACROS FUNCTIONS OF "CURSES" ARE REWRITTEN       *
 *		  INTO TRUE FUNCTIONS                    *
 *********************************************************/

#include "../../../../../config.h"

#if (defined(HAVE_CURSES) || defined(HAVE_NCURSES)) && defined(TTYEDEN)

#include "../Eden/eden.h"

#include <curses.h>

int _eden_initscr()
{
  symbol *sp;

  initscr();
  sp = lookup("stdscr");
  sp->d.u.i = (Int) stdscr;
  change(sp, TRUE);
  sp = lookup("curscr");
  sp->d.u.i = (Int) curscr;
  change(sp, TRUE);
}

int _eden_waddch(WINDOW *win, chtype ch) { waddch(win, ch); }
int _eden_waddstr(WINDOW *win, char *str) { waddstr(win, str); }
int _eden_box(WINDOW *win, chtype verch, chtype horch)
{ box(win, verch, horch); }
int _eden_clearok(WINDOW *win, bool bf) { clearok(win, bf); }
int _eden_wclear(WINDOW *win) { wclear(win); }
int _eden_wclrtobot(WINDOW *win) { wclrtobot(win); }
int _eden_wclrtoeol(WINDOW *win) { wclrtoeol(win); }
int _eden_wdelch(WINDOW *win) { wdelch(win); }
int _eden_wdeleteln(WINDOW *win) { wdeleteln(win); }
int _eden_werase(WINDOW *win) { werase(win); }
int _eden_winsch(WINDOW *win, chtype ch) { winsch(win, ch); }
int _eden_winsertln(WINDOW *win) { winsertln(win); }
int _eden_wmove(WINDOW *win, int y, int x) { wmove(win, y, x); }
int _eden_overlay(WINDOW *srcwin, WINDOW *dstwin) { overlay(srcwin, dstwin); }
int _eden_overwrite(WINDOW *srcwin, WINDOW *dstwin)
{ overwrite(srcwin, dstwin); }
int _eden_wrefresh(WINDOW *win) { wrefresh(win); }
int _eden_wstandout(WINDOW *win) { wstandout(win); }
int _eden_wstandend(WINDOW *win) { wstandend(win); }
int _eden_echo(void) { echo(); }
int _eden_noecho(void) { noecho(); }
int _eden_wgetch(WINDOW *win) { wgetch(win); }
int _eden_wgetstr(WINDOW *win, char *str) { wgetstr(win, str); }
int _eden_raw(void) { raw(); }
int _eden_noraw(void) { noraw(); }
WINDOW *_eden_newwin(int nlines, int ncols, int begin_y, int begin_x)
{ newwin(nlines, ncols, begin_y, begin_x); }
int _eden_delwin(WINDOW *win) { delwin(win); }
int _eden_endwin(void) { endwin(); }
chtype _eden_winch(WINDOW *win) { winch(win); }
int _eden_leaveok(WINDOW *win, bool bf) { leaveok(win, bf); }
int _eden_mvcur(int oldrow, int oldcol, int newrow, int newcol)
{ mvcur(oldrow, oldcol, newrow, newcol); }
int _eden_nl(void) { nl(); }
int _eden_nonl(void) { nonl(); }
int _eden_scrollok(WINDOW *win, bool bf) { scrollok(win, bf); }
int _eden_scroll(WINDOW *win) { scroll(win); }
int _eden_cbreak(void) { cbreak(); }
int _eden_nocbreak(void) { nocbreak(); }

/* crmode was replaced by cbreak in Solaris 2 - keep it here for
   compatibility. I'm attempting to do what the old curses version of
   this did here, but I still don't think I've got it right. */

int _eden_crmode(void) { cbreak(); raw(); }
int _eden_nocrmode(void) { nocbreak(); noraw(); }

/* Don't know how to cope with multiple arguments here
int _eden_wprintw(WINDOW *win, char *fmt, arg ... );
int _eden_wscanw(WINDOW *win, char *fmt, arg ...);
*/

/* getx and gety seem to no longer exist */

#endif  /* ifdef HAVE_CURSES or HAVE_NCURSES */
