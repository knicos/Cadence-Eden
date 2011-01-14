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

#if defined(__APPLE__) && !defined(TTYEDEN) && !defined(WEDEN_ENABLED)

#include <Carbon/Carbon.h>

/* !@!@ Fudge! */
#include "/Developer/Headers/FlatCarbon/CFBundle.h"


extern void setLibLocation(char *);

#ifndef MAX_PATH_LEN
    #define MAX_PATH_LEN 1024
#endif

void setMacOSXLibLoc(void) {
  /* lib-tkeden is located within the tkeden.app "bundle" directory
     (which is represented to the user in the Finder as an application
     icon).  Get the reference to where it is. */
  CFBundleRef mainBundleRef;
  mainBundleRef = CFBundleGetMainBundle();
  if (mainBundleRef != NULL) {
    CFURLRef libtkedenURL;
    libtkedenURL = CFBundleCopyResourceURL(mainBundleRef,
					   CFSTR("lib-tkeden"),
					   NULL, NULL);
    if (libtkedenURL != NULL) {
      char *libtkedenPath = malloc(MAX_PATH_LEN + 1);

      /*
      char *cstr = malloc(1024);
      CFStringGetCString(CFURLGetString(libtkedenURL),
			 cstr, 1024, kCFStringEncodingISOLatin2);
      fprintf(stderr, "libtkedenPath=%s\n", cstr);
      */

      if (CFURLGetFileSystemRepresentation(libtkedenURL, true,
					   libtkedenPath, MAX_PATH_LEN)) {
	setLibLocation(libtkedenPath);
      } else {
	/* gcc needs the -fpascal-strings option to allow \p (which
           means form a Pascal string: max 255 chars, the first byte
           states the length) */
	StandardAlert(kAlertStopAlert, "\pOoops",
		      "\pCouldn't GetFileSystemRepresentation of libtkedenURL",
		      NULL, NULL);
	exit(1);
      }

      free(libtkedenPath);

    } else {
      StandardAlert(kAlertStopAlert, "\pOoops",
		    "\pCouldn't find libtkedenURL", NULL, NULL);

    }

  } else {
    StandardAlert(kAlertStopAlert, "\pOoops",
		  "\pCouldn't find mainBundleRef", NULL, NULL);

  }

}

#endif /* __APPLE__ and not TTYEDEN and not WEDEN_ENABLED */
