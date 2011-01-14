%eden

## dots in the component name implies that we are doing gel
func scout_is_gel_component {
   para component;
   auto i;
   for (i=1; i<=component#; i++)
      if (component[i]=='.')
         return 1;
   return 0;
}

## get the name of the display from gel
func scout_get_gel_display_name {
   para component;
   auto i, last;
   last=1;
   for (i=1; i<=component#; i++)
      if (component[i]=='.')
         last=i;
   return `substr(component,last+1,component#)//"_display"`;
}

proc scout_show_canvas
/* display a text window */
{
    para screen, winNo, boxName;

    auto win, box, frame, string;
    auto i, command;
    auto width, height, thickness, bdwidth, bdcolour, bdrelief, bdtype;
    auto tclVarNameStart;

    ## Get the display name if this is a gel component
    if (scout_is_gel_component(screen))
       win = `scout_get_gel_display_name(screen)`[winNo];
    else
       win = `screen`[winNo];
    frame = win[2];
    string = win[3];
    box = win[4];

    bdwidth = int(win[12]);
		bdcolour = win[15];
    bdrelief = win[17];
    if (bdcolour != "black" && bdrelief != "flat")
			writeln("SCOUT warning: attempting to apply border colour and relief for window \""//win[18]//"\" (but you can only use one or the other).");

    /* display boxes */
    for (i = 1; i <= frame#; i++) {
      tclVarNameStart = screen//"."//boxName//"_"//str(i);
      command = "."//tclVarNameStart;

      width = box_width(frame[i]);
      height = box_height(frame[i]);
      xoutput("place", command,
          "-x", int(frame[i][1]-bdwidth),
          "-y", int(frame[i][2]-bdwidth));
      xoutput(command, "configure",
        "-width", width,
        "-height", height,
        "-bg", win[10],
        "-bd", bdrelief!="flat" ? bdwidth : 0,
        "-relief", bdrelief,
				"-highlightthickness", bdrelief!="flat" ? 0 : bdwidth,
        "-highlightbackground", win[15],
        "-highlightcolor", win[15]
      );
      xoutput(command, "delete text");
      xoutput(command, "delete image");
      xoutput(command, "create text",
          Position(bdwidth, box_width(frame[i]), win[13]),
          "-fill", win[11],
          "-text {"//string//"}",
          "-width", box_width(frame[i]),
          "-font", win[16],
          "-tags text"
     );

      if (win# >= 18) {
      xoutput("set", tclVarNameStart//"_name", win[18]);
      xoutput("set", tclVarNameStart//"_box", i);
      }

      xoutput("refresh", command);

      dobinding(win[14], command, win[18], tclVarNameStart, i);
    }
}

proc scout_show_2D
/*
   display a DoNaLD/ARCA picture
*/
{
    para screen, winNo, boxName;

    ## Get the display name if this is a gel component [Ant][10/05/2005]
    if (scout_is_gel_component(screen))
       scout_show_2D_window(`scout_get_gel_display_name(screen)`[winNo],
          "."//screen//"."//boxName//"_1", boxName//"_1");
    else
       scout_show_2D_window(`screen`[winNo],
          "."//screen//"."//boxName//"_1", boxName//"_1");
}

proc scout_show_text  /* rename old scout_show_text to scout_show_canvas  */
                      /* this proc is to show TEXTBOX. It is a tcl text widget. --sun*/
/*
   display a text window
*/
{
    para screen, winNo, boxName;

    auto win, box, frame, string;
    auto i, var, command;
    auto width, height, thickness, bdwidth;
    auto alignment, currstr, new_string, command_string;
    auto bordercolour;

    ## Get the display name if this is a gel component [Ant][06/05/2005]
    if (scout_is_gel_component(screen))
       win = `scout_get_gel_display_name(screen)`[winNo];
    else
       win = `screen`[winNo];
    frame = win[2];
    string = win[3];
    new_string = "";    /*  fix a bug of which a string cannot contain {}[] -sun */ 
    for (i = 1; i <= string#; i++) {
      currstr = substr(string, i, i);
      if (currstr == "{" || currstr == "}" || currstr == "[" || currstr == "]") 
           new_string = strcat(new_string, "\\", currstr);
      else new_string = strcat(new_string, currstr);
      }

    box = win[4];
    thickness = (win[14] != 0) ? DFhighlight : 0;
    bdwidth = win[12] + thickness;

    /* bordercolour isn't implemented in Tk :(.  But Tk does implement
       a coloured highlight.  Each widget seems to be surrounded by a
       coloured, sizable highlight, then inside that a sizable border with
       appearance options (raised, relief etc), then the widget itself.
       So here we'll use the coloured (but not relief) highlight if
       bordercolour has been specified, and the border otherwise.

       If bordercolour wasn't specified, it seems to default to "black":
       so there will be no way of specifying a black bordercolour at the
       moment.

       [Ash, Jan 2003] */

    bordercolour = win[15];

    /* display boxes */
    for (i = 1; i <= frame#; i++) {
	var = boxName//"_"//str(i);
	command = "."//screen//"."//var;
	width = box_width(frame[i]);
	height = box_height(frame[i]);
	/* xoutput("pack", command); */

	xoutput("place", command,
	    "-x", int(frame[i][1]-bdwidth),
	    "-y", int(frame[i][2]-bdwidth));

	xoutput(command, "configure",
	    "-width", int(frame[i][3]),
	    "-height", int(frame[i][4]),
	    "-bg", win[10],
	    "-fg", win[11],
	    "-bd", (bordercolour == "black") ? int(win[12]) : 0,
            "-highlightbackground", win[15],
            "-highlightcolor", win[15],
            "-highlightthickness",
               (bordercolour == "black") ? thickness : int(win[12]),
	    "-relief", win[17],
	    "-font", win[16]
	);
    /*    switch (win[13]) {   /*  TEXTBOX alignment always is left */
	  case 3: { alignment="center"; break; }
	  case 2: { alignment="right"; break; }
	  default: { alignment="left"; break; }
	}
	xoutput(command, "tag add everywhere 1.0 end -justify", alignment);
    */
    /*
	if (win# >= 18) {
	xoutput("set", var//"_name", win[18]);
	xoutput("set", var//"_box", i);
	}
    */
	if (win[18] != "") {
	xoutput("set", win[18]//"_boxName", "\""//command//"\"");
	}
	xoutput("refresh", command);

	if (win[14] != 0) {
	    xoutput("bind", command, "<Button> { interface \""//
		"~"//win[18]//"_mouse_"//
		str(i)//" = \\[%b,%T,%s,%x,%y];\\n\" }");
	    /* supposed no drag, so we don't need consider Button Release */
	     xoutput("bind", command, "<ButtonRelease> { interface \""//
		"~"//win[18]//"_mouse_"//
		str(i)//" = \\[%b,%T,%s,%x,%y];\\n\" }"); 
	    command_string = "\\\""//command//" get 1.0 end"//"\\\"";
	    xoutput("bind", command, "<KeyRelease> { interfaceTEXT \""//
		"~"//win[18]//"_TEXT_"//
		str(i)//" = tcl("//command_string//");\\n\" }");
	} else {
	    xoutput("bind", command, "<Button> {}");
	    xoutput("bind", command, "<ButtonRelease> {}");
	    xoutput("bind", command, "<Key> {}");
	    xoutput("bind", command, "<KeyRelease> {}");
	}
    }
}
