/*
 * $Id: curses.h,v 1.10 2001/07/27 16:43:33 cssbz Exp $
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

#if (defined(HAVE_CURSES) || defined(HAVE_NCURSES)) && defined(TTYEDEN)

#if INCLUDE == 'H'

#include <curses.h>

/* All this is designed to cope with the fact that some of the curses
   routines might be macros. We need to make them all functions. */

extern int _eden_initscr();
extern int _eden_waddch();
extern int _eden_winsch();
extern int _eden_initscr();
extern int _eden_waddch();
extern int _eden_waddstr();
extern int _eden_box();
extern int _eden_clearok();
extern int _eden_wclear();
extern int _eden_wclrtobot();
extern int _eden_wclrtoeol();
extern int _eden_wdelch();
extern int _eden_wdeleteln();
extern int _eden_werase();
extern int _eden_winsch();
extern int _eden_winsertln();
extern int _eden_wmove();
extern int _eden_overlay();
extern int _eden_overwrite();
extern int _eden_wrefresh();
extern int _eden_wstandout();
extern int _eden_wstandend();
extern int _eden_echo();
extern int _eden_noecho();
extern int _eden_wgetch();
extern int _eden_wgetstr();
extern int _eden_raw();
extern int _eden_noraw();
extern int _eden_newwin();
extern int _eden_delwin();
extern int _eden_endwin();
extern int _eden_winch();
extern int _eden_leaveok();
extern int _eden_mvcur();
extern int _eden_nl();
extern int _eden_nonl();
extern int _eden_scrollok();
extern int _eden_scroll();
extern int _eden_cbreak();
extern int _eden_nocbreak();
extern int _eden_crmode();
extern int _eden_nocrmode();

#endif	/* if INCLUDE == 'H' */

#if INCLUDE == 'T'

{ "initscr",      _eden_initscr,       },
{ "waddch",       _eden_waddch,        },
{ "waddstr",      _eden_waddstr,       },
{ "box",          _eden_box,           },
{ "clearok",      _eden_clearok,       },
{ "wclear",       _eden_wclear,        },
{ "wclrtobot",    _eden_wclrtobot,     },
{ "wclrtoeol",    _eden_wclrtoeol,     },
{ "wdelch",       _eden_wdelch,        },
{ "wdeleteln",    _eden_wdeleteln,     },
{ "werase",       _eden_werase,        },
{ "winsch",       _eden_winsch,        },
{ "winsertln",    _eden_winsertln,     },
{ "wmove",        _eden_wmove,         },
{ "overlay",      _eden_overlay,       },
{ "overwrite",    _eden_overwrite,     },
{ "wrefresh",     _eden_wrefresh,      },
{ "wstandout",    _eden_wstandout,     },
{ "wstandend",    _eden_wstandend,     },
{ "echo",         _eden_echo,          },
{ "noecho",       _eden_noecho,        },
{ "wgetch",       _eden_wgetch,        },
{ "wgetstr",      _eden_wgetstr,       },
{ "raw",          _eden_raw,           },
{ "noraw",        _eden_noraw,         },
{ "newwin",       _eden_newwin,        },
{ "delwin",       _eden_delwin,        },
{ "endwin",       _eden_endwin,        },
{ "winch",        _eden_winch,         },
{ "leaveok",      _eden_leaveok,       },
{ "mvcur",        _eden_mvcur,         },
{ "nl",           _eden_nl,            },
{ "nonl",         _eden_nonl,          },
{ "scrollok",     _eden_scrollok,      },
{ "scroll",       _eden_scroll,        },
{ "cbreak",       _eden_cbreak,        },
{ "nocbreak",     _eden_nocbreak,      },
  /* crmode was replaced by cbreak in Solaris 2 - keep it here for
     compatibility */
{ "crmode",       _eden_crmode,        },
{ "nocrmode",     _eden_nocrmode,      },

#endif	/* if INCLUDE == 'T' */

#endif	/* ifdef HAVE_CURSES or HAVE_NCURSES */
