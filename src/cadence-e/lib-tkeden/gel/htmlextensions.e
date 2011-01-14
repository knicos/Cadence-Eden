%eden

## Make sure the html widget is loaded into
## the interpreter

tcl("
	if {[info command html]==\"\"} {
	  foreach f {./tkhtml.so ./tkhtml.dll} {
	    if {[file exists $f]} {
	      if {[catch {load $f Tkhtml}]==0} break
	    }
	  }
	}
");

tcl("proc FontCmd {size attrs} {
   set result \"\"
   if { [lsearch $attrs fixed] >=0 } { lappend result Courier } else { lappend result Helvetica }
   set pointsize [expr {int(18*pow(1.2,$size-4))}]
   lappend result $pointsize
   if { [lsearch $attrs bold] >=0 } { lappend result bold }
   if { [lsearch $attrs italic] >=0 } { lappend result italic }
   return $result
}
");

tcl("proc RandomString {} { return \"img[expr {int(rand()*255)}]\" }");

tcl("proc ImgCmd {src width height attrs} {
   set imagename [RandomString]
   image create photo $imagename -file $src
   return $imagename }");

tcl("proc HrefCmd {url} { interface $url }");

tcl("bind HtmlClip <Button-1> {
        set parent [winfo parent %W]
        set url [$parent href %x %y]
        if {[string length $url] > 0} {
                eval [concat [$parent cget -hyperlinkcommand] \" {$url}\"]
        }
 }
");

tcl("bind HtmlClip <Motion> {
  set parent [winfo parent %W]
  set url [$parent href %x %y]
  if {[string length $url] > 0} {
    $parent configure -cursor hand2
  } else {
    $parent configure -cursor {}
  }

 }
");

tcl(" if {[string equal [tk windowingsystem] \"classic\"]
        || [string equal [tk windowingsystem] \"aqua\"]} {
    bind HtmlClip <MouseWheel> {
        %W yview scroll [expr {- (%D)}] units
    }
    bind HtmlClip <Option-MouseWheel> {
        %W yview scroll [expr {-10 * (%D)}] units
    }
    bind HtmlClip <Shift-MouseWheel> {
        %W xview scroll [expr {- (%D)}] units
    }
    bind HtmlClip <Shift-Option-MouseWheel> {
        %W xview scroll [expr {-10 * (%D)}] units
    }
 } else {

    bind HtmlClip <MouseWheel> {
        %W yview scroll [expr {- (%D / 120) * 4}] units
    }
 }

 if {[string equal \"x11\" [tk windowingsystem]]} {
    bind HtmlClip <4> {
        if {!$tk_strictMotif} {
            %W yview scroll -5 units
        }
    }
    bind HtmlClip <5> {
        if {!$tk_strictMotif} {
            %W yview scroll 5 units
        }
    }
 }
");
