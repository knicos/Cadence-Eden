# $Id: edenio.tcl,v 1.24 2002/07/10 19:31:18 cssbz Exp $
#
set variantversion "WebEDEN - $_tkeden_version"

set notation "%eden"



# This is used in scout.init.e for TEXTBOX [Ash]
proc interfaceTEXT {statement} {
    todo $statement
}


# Used to 'show/hide' a window in TCL																		# [TO_MAKE_WebEdenDecision]
proc show {w yes} {																																	
	
    if $yes {
		wm deiconify .$w
		raise .$w
		weden_SendCompleteRealTimeMessage "<s><item type=\"showhidescoutwindow\"><window id=\".$w\"><visible>1</visible></window></item></s>"
		case $w {
		    scout	{dumpscout; writeln("Doing a scout dump");}
		    donald	{dumpdonald; writeln("Doing a donald dump");}
		    eden	{viewOption}
		    lsd     {dumpLSD}
		}
		# the lsd above should never be matched in client
		# or plain tkeden [Ash]
	
    } else {
    	puts "Showing a window"
		wm withdraw .$w
		weden_SendCompleteRealTimeMessage "<s><item type=\"showhidescoutwindow\"><window id=\".$w\"><visible>0</visible></window></item></s>"
    }
}






# Web EDEN Network Service Provider
# Richard Myers (c) 2007-2008 RJM Solutions


# Processes client commands
proc weden_ProcessCommandBlock { command } {

	# Process the command(s). Note: weden has been added to the EDEN command run options like queue, todo etc.
	
	# We first replace all carriage returns with a new line as EDEN does not seem to like them having
	# tried a number of models including : roomviewerYung1991 which failed to run before doing this [Richard]
	regsub -all {\r} "$command" "\n" newcommand
	
	weden $newcommand
	
}

# Quit the EDEN instance (ending a user session)
proc weden_CloseEDENSession {} {
	
	# Close EDEN
	quit
}


proc weden_startRealTimeMessage { } {

	global weden_NetworkSocket

	puts -nonewline $weden_NetworkSocket "<?xml version=\"1.0\"?>\n<weden>"

}

proc weden_appendRealTimeMessage { msg } {

	# Get global variables
	global weden_NetworkSocket

	# Do not append additional new lines!
	puts -nonewline $weden_NetworkSocket "$msg"
}

proc weden_endRealTimeMessage { } {

	global weden_NetworkSocket

	puts -nonewline $weden_NetworkSocket "</weden>\0"
   	flush $weden_NetworkSocket

}

proc weden_SendCompleteRealTimeMessage { msg } {

	global weden_NetworkSocket

	puts -nonewline $weden_NetworkSocket "<?xml version=\"1.0\"?>\n<weden>"
	puts -nonewline $weden_NetworkSocket "$msg"
	puts -nonewline $weden_NetworkSocket "</weden>\0"
   	flush $weden_NetworkSocket
}



# ---------------------------------------------------------
#              Network Operations Start Here
# ---------------------------------------------------------



# Handles incoming commands from client and client disconnects
proc weden_NetworkSocketReadHandler { sock } {

	global weden_bufferedCommands
	
	# See if the client has disconnected
	if {[eof $sock]} {
  
		# weden_LogAction "Client: Disconnected"
  
		# Release the client socket channel
		close $sock
			
		weden_CloseEDENSession

	} else {
	
		# Append the new socket data to the bufferedCommands recived
		append weden_bufferedCommands [read $sock]
		
		# Enable below line to test we always execute every xml block
		# append weden_bufferedCommands "<weden><usercommand>%donald</usercommand></weden>"
		
		# See if we have a full message (ie. One ending in </usercommand></weden>) and store its location (-1 if none)
		set endOfXMLLocation [string first "</weden>" $weden_bufferedCommands [string first "</usercommand>" $weden_bufferedCommands]]
		
		# Did we have a complete message? (if so location should be should be > -1)
		while { $endOfXMLLocation != -1 } {
			
			# We may have multiple <weden>...</weden>messages...we extract and process each xml block one at a time
			# We add 8 to the endOfXML Location as the end is start point of closing </weden> + number of chars (ie. 8)
			set singleXMLBlock [string range $weden_bufferedCommands 0 [expr $endOfXMLLocation + 8]] 
			
			# We also remove this block from the bufferedCommands so its not processed again
			set nextXMLBlockStart [string first "<weden>" $weden_bufferedCommands $endOfXMLLocation]
			if { $nextXMLBlockStart != -1 } {
				set weden_bufferedCommands [string replace $weden_bufferedCommands 0 [expr $nextXMLBlockStart - 1]]
			} else {
				set weden_bufferedCommands ""
			}
	
			# Using a regexpression to determin and also extract valid eden commands
			set validCmd [regexp {^[\n\r\t\f ]*<weden>[ \n\r\t\f]*<usercommand><!\[CDATA\[(.*)\]\]></usercommand>[ \n\r\t\f]*</weden>[ \n\r\t\f\0]*$} "$singleXMLBlock" originalCmd extractedUsersCmd]
		
			if {$validCmd} {
		
				# Log the command recived
				# weden_LogAction "Users Command : $extractedUsersCmd"

				# Process the client command
				weden_ProcessCommandBlock $extractedUsersCmd

			} else {
			
				# Invalid format. Display whats up
				# weden_LogAction "ERROR: Users command was in an invalid format"
				# weden_LogAction "Raw User Command:\n $singleXMLBlock"
				
			}
			
			# See if we have another full message so we can try and process another
			set endOfXMLLocation [string first "</weden>" $weden_bufferedCommands [string first "</usercommand>" $weden_bufferedCommands]]
			
		}
	}
}

# ----------------------------------------------------------
#     SETUP THE NETWORK SOCKET (connect to relay server) 
# ----------------------------------------------------------

# Display connection info notice
# weden_LogAction "Starting Web EDEN Service. Connecting to $WedenRelayServerHostName : $WedenRelayServerHostPort"

# Create new buffer to hold the commands recieved and not yet processed
# A Complete command is in a block of <xml><usercommand>...</usercommand></xml>
set weden_bufferedCommands ""

# Create the client socket to connect to relay server
# Note: The hostname and port are passed in from EDEN in main.c - main()
set weden_NetworkSocket [socket $WedenRelayServerHostName $WedenRelayServerHostPort]

# Monitor the socket for data being avalaible and handel it
fileevent $weden_NetworkSocket readable [list weden_NetworkSocketReadHandler $weden_NetworkSocket]

# Set up buffering and disable it being a blocking i/o channel. Also use binary encoding and translation
fconfigure $weden_NetworkSocket -buffering full -blocking 0 -encoding binary -translation binary







if {$_tkeden_win32_version == "0.0"} {
    if {[expr !$_tkeden_apple]} {
	tk_focusFollowsMouse
    }
}

if {$_tkeden_variant == "dtkeden"} {
    if {$_dtkeden_isServer} {
	set variantversion "dtkeden $_tkeden_version (server)"
    } else {
	set variantversion "dtkeden $_tkeden_version (client)"
    }
} else {
    # tkeden
    set variantversion "tkeden $_tkeden_version"
}

wm title . "$variantversion: Input"

set radiosBg grey60
set radiosButtonBg grey50

frame .radios -background $radiosBg -borderwidth 0
button .radios.accept -text "Accept" -underline 0 \
	-background $radiosButtonBg -command {accept}
pack .radios.accept -side left

if {$_tkeden_variant == "dtkeden"} {
    if {$_dtkeden_isServer} {
	button .radios.send -text "Send" -underline 0 \
		-background $radiosButtonBg -command {selectClients}
    } else {
	button .radios.send -text "Send" -underline 0 \
		-background $radiosButtonBg -command {
	    set text [.text get 1.0 end]
	    set text [string trim $text]
	    if {$text != ""} {
		set errCode [catch {.menu.accept invoke} string]
		sendServer $text 
	    }
	}
    }
    pack .radios.send -side left
}

set notation "%eden"

proc addNotationRadioButton {notation} {
    global radiosBg

    # Tk widgets must start with a lowercase letter
    set widget ".radios.[string tolower $notation]"

    # Ignore errors that occur if the radiobutton is already defined
    catch {
	radiobutton $widget -variable notation \
	    -highlightbackground $radiosBg -background $radiosBg \
	    -value "%$notation" -text "%$notation" \
	    -command {
		# the % at the start doesn't seem to be required here
		appendHist "$notation\n"; evaluate "$notation\n"
	    }
    }

    pack $widget -side left
}

# provided so people can customise the tkeden interface a little
proc removeNotationRadioButton {notation} {
    set widget ".radios.[string tolower $notation]"
    destroy $widget
}

addNotationRadioButton eden
addNotationRadioButton donald
addNotationRadioButton scout

if {$_tkeden_sasamiAvail == "1"} { addNotationRadioButton sasami }
if {$_tkeden_variant == "dtkeden"} { addNotationRadioButton lsd }

button .radios.interrupt -text "Interrupt" \
	-background $radiosButtonBg -command {interrupt}
pack .radios.interrupt -side right

menu .menu
. config -menu .menu

# This is similar to the background colour used for the menus on Solaris
# but not quite (unfortunately)
set bg grey60

frame .labelframe -borderwidth 0 -background $bg

# The prompt shows the current notation [Ash]
label .prompt -anchor w -text "Enter EDEN statements:" -background $bg

# The labelframe also contains the current virtual agent [Ash]
label .agentName -anchor e -text "" -background $bg

pack .prompt  -side left -fill x -in .labelframe
pack .agentName -side right -fill x -in .labelframe

# ideally we should use this font whereever Eden code is displayed
font create edencode -family courier -size 10

text .text -width 80 -height 15 -yscrollcommand ".scroll set" \
	-background white -foreground black -insertbackground blue \
	-insertofftime 80 -insertontime 1000 -insertwidth 2p \
	-font edencode
# set tabs to width of two characters (have to set it in pixels) [Ash]
.text configure -tabs [font measure [.text cget -font] 00]
scrollbar .scroll -command ".text yview"

pack .radios -side top -fill x
pack .labelframe -side top -fill x
pack .scroll -side right -fill y
pack .text -side right -fill both -expand 1

set m [menu .menu.file -tearoff 0]
.menu add cascade -label "File" -underline 0 -menu .menu.file
if {$_tkeden_apple} {
	$m add command -label "Open..." -command {include Open} -accelerator Command-O
} else {
	$m add command -label "Open..." -command {include Open} -underline 0
}
$m add command -label "Execute..." -command {include Execute} -underline 0
$m add separator
set saveAsReusable 1
$m add checkbutton -label "Save as reusable definitions" \
	-variable saveAsReusable -offvalue 0 -onvalue 1
if {$_tkeden_apple} {
	$m add command -label "Save all definitions..." \
		-command {save all} -accelerator Command-S
} else {
	$m add command -label "Save all definitions..." \
		-command {save all} -underline 5
}
$m add command -label "Save Scout definitions..." \
	-command {save scout} -underline 5
$m add command -label "Save DoNaLD definitions..." \
	-command {save donald} -underline 5
$m add command -label "Save Eden definitions..." \
	-command {save eden} -underline 5
if {$_tkeden_variant == "dtkeden"} {
	if {$_dtkeden_isServer} {
	$m add command -label "Save LSD description..." \
		-command {save lsd} -underline 5
	}
}
$m add command -label "Save history..." \
	-command {save hist} -underline 5

if {$_tkeden_apple} {
	# The Application menu (left-most, next to the Apple menu)
	set mapp [menu .menu.apple -tearoff 0]
	.menu add cascade -menu .menu.apple
	# Strangely, the About option doesn't seem to have an ellipsis in
	# other Apple apps, despite it leading to a dialogue...
	$mapp add command -label "About $_tkeden_variant" -command about
	# add a Preferences item for conformity with other apps, but don't
	# put anything here (perhaps we will in the distant future!)
	$mapp add separator
	$mapp add command -label "Preferences..." -state disabled
    $mapp add command -label "Restart" -command { close $histfile; restart; }
	# "Quit tkeden" appears in the Apple menu automatically
} else {
    # If we aren't on the Mac, add File->Restart and File->Quit -- on the Mac,
    # we put these in the Apple menu...
	$m add separator
    $m add command -label "Restart" -command { close $histfile; restart; } -underline 0
	$m add separator
    $m add command -label "Quit" -command { close $histfile; quit; } -underline 0
}


# don't use tearoffs on the Apple platform
set m [menu .menu.edit -tearoff [expr !$_tkeden_apple]]
.menu add cascade -label "Edit" -underline 0 -menu .menu.edit
if {$_tkeden_apple} {
	$m add command -label "Accept" \
		-accelerator "Command-Return" \
		-command { acccept }
	$m add separator
	# copying ordering of options from TextEdit menu...
	$m add command -label "Cut" \
		-accelerator "Command-X" \
		-command {event generate .text <<Cut>>}
	$m add command -label "Copy" \
		-accelerator "Command-C" \
		-command {event generate .text <<Copy>>}
	$m add command -label "Paste" \
		-accelerator "Command-V" \
		-command {event generate .text <<Paste>>}
	$m add command -label "Select all" \
		-accelerator "Command-A" \
		-command {.text tag add sel 1.0 end}
#	$m add command -label "Select none" \
#		-accelerator "Control-\\" \
#		-command {.text tag add sel}
#		-command {.text tag delete sel}
#		-command {event generate .text <Control-backslash>}
	$m add separator
	$m add command -label "Previous" \
		-accelerator "Command-Up" \
		-command {previous}
	$m add command \
		-label "Next" \
		-accelerator "Command-Down" \
		-command {next}
} else {
	# (I found the keysym names using xmodmap -pk as I don't know the
	# virtual event names for select all and select none).
	$m add command -label "Select all" -underline 7 \
		-accelerator "Control-/" \
		-command {event generate .text <Control-slash>}
	$m add command -label "Select none" \
		-accelerator "Control-\\" \
		-command {event generate .text <Control-backslash>}
	$m add command -label "Copy" -underline 0 \
		-command {event generate .text <<Copy>>}
	$m add command -label "Cut" -underline 2 \
		-command {event generate .text <<Cut>>}
	$m add command -label "Paste" -underline 0 \
		-command {event generate .text <<Paste>>}
	$m add separator
	$m add command -label "Previous" \
		-accelerator "Control-Alt-Up or Meta-Up" \
		-command {previous} -underline 1
	$m add command \
		-label "Next" \
		-accelerator "Control-Alt-Down or Meta-Down" \
		-command {next} -underline 0
}
$m add separator
$m add command -label "Clear" \
	-accelerator "Control-Alt-0 or Meta-0" \
	-command {clearInputWindow} -underline 1

set m [menu .menu.show -tearoff [expr !$_tkeden_apple]]
if {$_tkeden_apple} {
	.menu add cascade -label "Window" -underline 0 -menu .menu.show
	# need to figure out how to implement the following... -- disabled for now
	$m add command -label "Close Window" -accelerator "Command-W" -state disabled
	$m add command -label "Minimise" -accelerator "Command-M" -state disabled
	$m add command -label "Zoom"  -state disabled
	$m add separator
	$m add command -label "Bring All to Front" -command { bringToTop }
	$m add separator
} else {
	.menu add cascade -label "View" -underline 0 -menu .menu.show
}
$m add checkbutton -label "View history..." \
	-variable showhist -command {show hist $showhist} -underline 5
$m add checkbutton -label "View errors..." \
	-variable showerr -command {show err $showerr} -underline 6
$m add checkbutton -label "View Scout definitions..." \
	-variable showscout -command {show scout $showscout} -underline 5
$m add checkbutton -label "View DoNaLD definitions..." \
	-variable showdonald -command {show donald $showdonald} -underline 5
$m add checkbutton -label "View Eden definitions..." \
	-variable showeden -command {show eden $showeden} -underline 5
if {$_tkeden_variant == "dtkeden"} {
    if {$_dtkeden_isServer} {
	$m add checkbutton -label "View LSD descriptions..." \
		-variable showlsd -command {show lsd $showlsd} -underline 5
	$m add checkbutton -label "View client connections..." \
		-variable showclient -command {show client $showclient} \
		-underline 5
	
	set m2 [menu .menu.type]
	.menu add cascade -label "Type" -menu .menu.type -underline 0
	set proType 0
	$m2 add radiobutton -label "Normal mode" \
		-variable proType -value 0
	$m2 add radiobutton -label "Interference mode" \
		-variable proType -value 1
	$m2 add radiobutton -label "Broadcast mode" \
		-variable proType -value 2 \
		-command { appendHist ">>\n"; evaluate ">>\n" }
	$m2 add radiobutton -label "Private mode" \
		-variable proType -value 3
    }
}

set m [menu .menu.help -tearoff 0]
.menu add cascade -label "Help" -underline 0 -menu .menu.help
# About is in the Apple menu on the Apple platform
if {$_tkeden_apple == "0"} {
    $m add command -label "About $_tkeden_variant..." -command about \
	    -underline 0
}
$m add command -label "Credits..." -command credits \
	-underline 0
$m add command -label "Key shortcuts..." -command keys \
	-underline 0
$m add command -label "Eden quick reference..." \
	-command edenQuickRef -underline 0
$m add command -label "Scout quick reference..." \
	-command scoutQuickRef -underline 0
$m add command -label "Donald quick reference..." \
	-command donaldQuickRef -underline 0
$m add command -label "Sasami quick reference..." \
	-command sasamiQuickRef -underline 4
$m add command -label "Aop quick reference..." \
	-command aopQuickRef -underline 2
$m add command -label "Colour names..." \
	-command colourNames -underline 7
$m add command -label "ChangeLog..." -command changeLog \
	-underline 1

if {$_tkeden_apple} {
	# the following useful for debugging...
	#bind . <Key> { puts "Key %K" }

	# seems that Alt/Meta may not be accepted in bindings on the Mac...
	bind all <Command-q> { exit }
	bind all <Command-Q> { exit }
	bind .text <Command-a> { .text tag add sel 1.0 end }
	bind .text <Command-A> { .text tag add sel 1.0 end }
	bind .text <Command-Up> { previous }
	bind .text <Command-Down> { next }
	bind .text <Control-u> { controlU }
	bind .text <Control-U> { controlU }
	bind .text <Command-Left> { .text mark set insert {insert linestart} }
	bind .text <Command-Right> { .text mark set insert {insert lineend} }

	# on the Mac, Command-A is Select All as standard, and I don't want
	# to conflict with that, so using this for Accept instead...
	bind .text <Command-Return> {
		accept;
		# now stop the return from also having effect on the text...
		break;
	}
	
	bind all <Command-o> {include Open}
	bind all <Command-O> {include Open}
	bind all <Command-s> {save all}
	bind all <Command-S> {save all}
	
	# still need to figure out
	# - command-w for closing current window
	# - command-h for hiding the application (this is in the app menu)
	# - alt-command-h for hiding other apps (this is in the app menu)
} else {
	bind .text <Alt-a> { accept }
	bind .text <Alt-A> { accept }
	bind .text <Control-Alt-Up> { previous }
	bind .text <Meta-Up> { previous }
	bind .text <Alt-p> { previous }
	bind .text <Alt-P> { previous }
	bind .text <Control-Alt-Down> { next }
	bind .text <Meta-Down> { next }
	bind .text <Alt-n> { next }
	bind .text <Alt-N> { next }
	bind .text <Control-Alt-0> { clearInputWindow }
	bind .text <Meta-0> { clearInputWindow }
	bind .text <Control-u> { controlU }
	bind .text <Control-U> { controlU }
}


proc bringToTop {} {
    set wins "[winfo children .] .";
    foreach w $wins {
    	set tlw [winfo toplevel $w];
    	if {[wm state $tlw] == "iconic"} { wm deiconify $tlw; }
	if {$w != ".menu"} { raise $w }
    }
}

# bring all our windows to the top if this combination of keys is pressed
bind all <Shift-Control-Tab> { bringToTop }

bindtags .text {all .text Text}

update


# History window

toplevel .hist
wm title .hist "$variantversion: Command History"

frame .hist.menu -relief raised -borderwidth 2
pack .hist.menu -side top -fill x
button .hist.menu.save -text "Save" -underline 0 -command {save hist} \
	-relief flat -highlightthickness 0
	bind .hist <Alt-S> { .hist.menu.save invoke }
	bind .hist <Alt-s> { .hist.menu.save invoke }
button .hist.menu.find -text "Find" -underline 0 -command {find hist} \
	-relief flat -highlightthickness 0
	bind .hist <Alt-F> { .hist.menu.find invoke }
	bind .hist <Alt-f> { .hist.menu.find invoke }
button .hist.menu.close -text "Close" -underline 0 \
	-command {global showhist; set showhist 0; show hist 0} \
	-relief flat -highlightthickness 0
	bind .hist <Alt-C> { .hist.menu.close invoke }
	bind .hist <Alt-c> { .hist.menu.close invoke }
pack .hist.menu.save .hist.menu.find .hist.menu.close -side left

frame .hist.t
pack .hist.t -fill both -expand 1
text .hist.t.text -state disabled -width 80 -height 10 \
	-yscrollcommand ".hist.t.scroll set" -font edencode
scrollbar .hist.t.scroll -command ".hist.t.text yview" 
pack .hist.t.scroll -side right -fill y
pack .hist.t.text -side right -fill both -expand 1

wm withdraw .hist
wm protocol .hist WM_DELETE_WINDOW ".hist.menu.close invoke;"
update


# Error window

toplevel .err
wm title .err "$variantversion: Errors"

frame .err.menu -relief raised -borderwidth 2
pack .err.menu -side top -fill x
button .err.menu.save -text "Save" -underline 0 -command {save err} \
	-relief flat -highlightthickness 0
	bind .err <Alt-S> { .err.menu.save invoke }
	bind .err <Alt-s> { .err.menu.save invoke }
button .err.menu.find -text "Find" -underline 0 -command {find err} \
	-relief flat -highlightthickness 0
	bind .err <Alt-F> { .err.menu.find invoke }
	bind .err <Alt-f> { .err.menu.find invoke }
button .err.menu.close -text "Close" -underline 0 \
	-command {global showerr; set showerr 0; show err 0} \
	-relief flat -highlightthickness 0
	bind .err <Alt-C> { .err.menu.close invoke }
	bind .err <Alt-c> { .err.menu.close invoke }
pack .err.menu.save .err.menu.find .err.menu.close -side left

frame .err.t
pack .err.t -fill both -expand 1
text .err.t.text -state disabled -width 80 -height 10 \
	-yscrollcommand ".err.t.scroll set" -font edencode
scrollbar .err.t.scroll -command ".err.t.text yview" 
pack .err.t.scroll -side right -fill y
pack .err.t.text -side right -fill both -expand 1

wm withdraw .err
wm protocol .err WM_DELETE_WINDOW ".err.menu.close invoke;"
update


if {$_tkeden_variant == "dtkeden"} {
    if {$_dtkeden_isServer} {
	toplevel .client
	wm title .client "$variantversion: Client Connections"

	frame .client.menu -relief raised -borderwidth 2
	pack .client.menu -side top -fill x
	button .client.menu.close -text "Close" -underline 0 \
		-relief flat -highlightthickness 0 \
		-command {global showclient; set showclient 0; show client 0;}
	bind .client <Alt-C> { .client.menu.close invoke }
	bind .client <Alt-c> { .client.menu.close invoke }
	pack .client.menu.close -side left

	frame .client.t
	pack .client.t -fill both -expand 1
	text .client.t.text -state disabled -width 80 -height 10 \
		-yscrollcommand ".client.t.scroll set"
	scrollbar .client.t.scroll -command ".client.t.text yview" 
	pack .client.t.scroll -side right -fill y
	pack .client.t.text -side right -fill both -expand 1

	wm withdraw .client
	wm protocol .client WM_DELETE_WINDOW ".client.menu.close invoke;"
	update
    }
}

toplevel .scout
wm title .scout "$variantversion: Scout Definitions"

frame .scout.menu -relief raised -borderwidth 2
pack .scout.menu -side top -fill x
button .scout.menu.save -text "Save" -underline 0 -command {save scout} \
	-relief flat -highlightthickness 0
	bind .scout <Alt-S> { .scout.menu.save invoke }
	bind .scout <Alt-s> { .scout.menu.save invoke }
button .scout.menu.find -text "Find" -underline 0 -command {find scout} \
	-relief flat -highlightthickness 0
	bind .scout <Alt-F> { .scout.menu.find invoke }
	bind .scout <Alt-f> { .scout.menu.find invoke }
button .scout.menu.rebuild -text "Rebuild" -underline 0 \
	-command {dumpscout} \
	-relief flat -highlightthickness 0
	bind .scout <Alt-R> { .scout.menu.rebuild invoke }
	bind .scout <Alt-r> { .scout.menu.rebuild invoke }
button .scout.menu.close -text "Close" -underline 0 \
	-command {global showscout; set showscout 0; show scout 0} \
	-relief flat -highlightthickness 0
	bind .scout <Alt-C> { .scout.menu.close invoke }
	bind .scout <Alt-c> { .scout.menu.close invoke }
pack .scout.menu.save .scout.menu.find .scout.menu.rebuild .scout.menu.close \
	-side left

frame .scout.t
pack .scout.t -fill both -expand 1
text .scout.t.text -state disabled -width 80 -height 20 \
	-yscrollcommand ".scout.t.scroll set" -font edencode
scrollbar .scout.t.scroll -command ".scout.t.text yview" 
pack .scout.t.scroll -side right -fill y
pack .scout.t.text -side right -fill both -expand 1
wm withdraw .scout
wm protocol .scout WM_DELETE_WINDOW ".scout.menu.close invoke;"
update


toplevel .donald
wm title .donald "$variantversion: DoNaLD Definitions"

frame .donald.menu -relief raised -borderwidth 2
pack .donald.menu -side top -fill x
button .donald.menu.save -text "Save" -underline 0 -command {save donald} \
	-relief flat -highlightthickness 0
	bind .donald <Alt-S> { .donald.menu.save invoke }
	bind .donald <Alt-s> { .donald.menu.save invoke }
button .donald.menu.find -text "Find" -underline 0 -command {find donald} \
	-relief flat -highlightthickness 0
	bind .donald <Alt-F> { .donald.menu.find invoke }
	bind .donald <Alt-f> { .donald.menu.find invoke }
button .donald.menu.rebuild -text "Rebuild" -underline 0 \
	-command {dumpdonald} \
	-relief flat -highlightthickness 0
	bind .donald <Alt-R> { .donald.menu.rebuild invoke }
	bind .donald <Alt-r> { .donald.menu.rebuild invoke }
button .donald.menu.close -text "Close" -underline 0 \
	-command {global showdonald; set showdonald 0; show donald 0} \
	-relief flat -highlightthickness 0
	bind .donald <Alt-C> { .donald.menu.close invoke }
	bind .donald <Alt-c> { .donald.menu.close invoke }
pack .donald.menu.save .donald.menu.find .donald.menu.rebuild \
	.donald.menu.close -side left

frame .donald.t
pack .donald.t -fill both -expand 1
text .donald.t.text -state disabled -width 80 -height 20 \
	-yscrollcommand ".donald.t.scroll set" -font edencode
.donald.t.text tag config viewport -background #efd4b4
.donald.t.text tag config master -background #efd4b4
scrollbar .donald.t.scroll -command ".donald.t.text yview" 
pack .donald.t.scroll -side right -fill y
pack .donald.t.text -side right -fill both -expand 1
wm withdraw .donald
wm protocol .donald WM_DELETE_WINDOW ".donald.menu.close invoke;"
update

if {$_tkeden_variant == "dtkeden"} {
    if {$_dtkeden_isServer} {
	toplevel .lsd
	wm title .lsd "$variantversion: LSD Descriptions"

	frame .lsd.menu -relief raised -borderwidth 2
	pack .lsd.menu -side top -fill x
	button .lsd.menu.save -text "Save" -underline 0 -command {save lsd} \
		-relief flat -highlightthickness 0
	bind .lsd <Alt-S> { .lsd.menu.save invoke }
	bind .lsd <Alt-s> { .lsd.menu.save invoke }
	button .lsd.menu.find -text "Find" -underline 0 -command {find lsd} \
		-relief flat -highlightthickness 0
	bind .lsd <Alt-F> { .lsd.menu.find invoke }
	bind .lsd <Alt-f> { .lsd.menu.find invoke }
	button .lsd.menu.rebuild -text "Rebuild" -underline 0 \
		-command {dumpLSD} \
		-relief flat -highlightthickness 0
	bind .lsd <Alt-R> { .lsd.menu.rebuild invoke }
	bind .lsd <Alt-r> { .lsd.menu.rebuild invoke }
	button .lsd.menu.close -text "Close" -underline 0 \
		-command {global showlsd; set showlsd 0; show lsd 0} \
		-relief flat -highlightthickness 0
	bind .lsd <Alt-C> { .lsd.menu.close invoke }
	bind .lsd <Alt-c> { .lsd.menu.close invoke }
	pack .lsd.menu.save .lsd.menu.find .lsd.menu.rebuild .lsd.menu.close \
		-side left
	
	frame .lsd.t
	pack .lsd.t -fill both -expand 1
	text .lsd.t.text -state disabled -width 80 -height 20 \
		-yscrollcommand ".lsd.t.scroll set"
	scrollbar .lsd.t.scroll -command ".lsd.t.text yview" 
	pack .lsd.t.scroll -side right -fill y
	pack .lsd.t.text -side right -fill both -expand 1
	wm withdraw .lsd
	wm protocol .lsd WM_DELETE_WINDOW ".lsd.menu.close invoke;"
	update
    }
}

toplevel .eden
wm title .eden "$variantversion: Eden Definitions"

menu .eden.menu
.eden config -menu .eden.menu

set m [menu .eden.menu.edit -tearoff [expr !$_tkeden_apple]]
.eden.menu add cascade -label "Edit" -underline 0 -menu .eden.menu.edit
$m add command -label "Copy" -underline 0 \
	-command {event generate .eden.t.text <<Copy>>}
$m add command -label "Cut" -underline 2 \
	-command {event generate .eden.t.text <<Cut>>}
$m add command -label "Paste" -underline 0 \
	-command {event generate .eden.t.text <<Paste>>}

.eden.menu add command -label "Save" -underline 0 -command {save eden}
.eden.menu add command -label "Find" -underline 0 -command {find eden}
.eden.menu add command -label "Rebuild" -underline 0 -command {viewOption}
.eden.menu add command -label "Update" -underline 0 -command {edenUpdate}
.eden.menu add command -label "Close" -underline 0 \
	-command {global showeden; set showeden 0; show eden 0}

frame .eden.t
pack .eden.t -fill both -expand 1
text .eden.t.text -state disabled -width 80 -height 20 \
	-yscrollcommand ".eden.t.scroll set" -font edencode
.eden.t.text tag config masteragent -background #efd4b4
.eden.t.text tag config scout -foreground red
.eden.t.text tag config donald -foreground blue
scrollbar .eden.t.scroll -command ".eden.t.text yview" 
pack .eden.t.scroll -side right -fill y
pack .eden.t.text -side right -fill both -expand 1
wm withdraw .eden
wm protocol .eden WM_DELETE_WINDOW "set showeden 0; show eden 0"
update

if {$_tkeden_win32_version == "0.0"} {
  # we're on UNIX
  set histfilename $env(HOME)/.tkeden-history
} else {
  # cygwin seems to require filenames in DOS (C:\blah) format
  set histfilename \
    [cygwin_conv_to_full_win32_path "$env(HOME)/.tkeden-history"]
}

# Keep a few backups around as people don't seem to look for the
# history file until they've restarted tkeden once or twice.  Ideally
# this would preserve file dates to make it easier to find data when
# grubbing around in the history files, but there isn't an option for
# this in Tcl file copy.  We could use file rename, but this causes
# problems with multiple concurrent instances of tkeden running in the
# same user account with an NFS mounted home directory (machine 1
# shifts history files, starts to use tkeden-history, machine 2 shifts
# history files, machine 1 gives stale NFS handle error).  [Ash]
catch {file copy -force ${histfilename}.2 ${histfilename}.3}
catch {file copy -force ${histfilename}.1 ${histfilename}.2}
catch {file copy -force $histfilename ${histfilename}.1}
set histfile [open $histfilename w]

proc appendHist {text} {
    global histfile
    .hist.t.text config -state normal
    .hist.t.text insert end $text
    puts $histfile $text nonewline
    flush $histfile
    .hist.t.text see end
    .hist.t.text config -state disabled
}

set errorNo 0
set errorAppendNo 0
set error ""

proc appendErr {text} {
    global errorNo errorAppendNo error

    .err.t.text config -state normal

    if {$errorAppendNo == 0} {
	.err.t.text insert end "## ERROR number [incr errorNo]:\n"
    }
    incr errorAppendNo
    append error $text

    .err.t.text insert end $text
    .err.t.text see end
    .err.t.text config -state disabled
}    

proc errorComplete {beep} {
    global errorNo errorAppendNo error

    set errorInitialText [string range $error 0 55]
    if {[string length $error] > 55} {append errorInitialText "..."}

    appendHist "## ERROR number $errorNo: $errorInitialText\n"
    set errorAppendNo 0
    set error ""

    set showerr 1
    show err 1
    raise .err
    if {$beep} {bell}

    # Force .err to refresh, meaning the new error information is
    # shown on the screen even if we are in a tight loop.  This call
    # causes any <Configure> events on windows to trigger, which may then
    # cause some Eden to execute - this was the cause of "bug42".
    update idletasks
}

# Previous text
set pentries ""

# Number of entries of previous text to keep
set pmax 20

# Current (per-entry session) position in history
set ppos 0

proc accept {} {
    global pentries pmax ppos

    set text [.text get 1.0 end]
    appendHist $text

    # 1) remove the spurious \n that comes from Tcl's text widget
    # 2) append this entry to the list we are keeping
    # 3) remove an old entry from the front of the list if necessary

    # 3)
    if {[llength $pentries] >= $pmax} {
	set pentries [lrange $pentries 1 end]
    }

    # 2), 1)
    lappend pentries [string range $text 0 [expr [string length $text]-2]]

    evaluate $text

    clearInputWindow
    set ppos [llength $pentries];
}

proc previous {} {
    global pentries pmax ppos

    set text [.text get 1.0 end]
    clearInputWindow

    set ppos [expr $ppos - 1]
    if {$ppos < 0} {
	set ppos 0
	bell
    }

    .text insert end [lindex $pentries $ppos]
}

proc next {} {
    global pentries pmax ppos

    set text [.text get 1.0 end]
    clearInputWindow

    incr ppos
    if {$ppos > [llength $pentries]} {
	set ppos [llength $pentries]
	bell
    }

    .text insert end [lindex $pentries $ppos]
}

proc clearInputWindow {} {
    .text delete 1.0 end
}


proc controlU {} {
    # delete the text to the left of the cursor
    .text delete {insert linestart} insert
}

proc interface {statement} {
    global _tkeden_variant _dtkeden_isServer

    appendHist $statement
    if {$_tkeden_variant == "dtkeden"} {
	if {! ($_dtkeden_isServer)} {
	    # Patrick's change to client only - dunno why [Ash]
	    set statement "$statement\n"
	}
    }
    todo $statement
}

# This is used in scout.init.e for TEXTBOX [Ash]
proc interfaceTEXT {statement} {
    todo $statement
}

proc cleanup {w} {
    .$w.t.text config -state normal
    .$w.t.text delete 1.0 end
    .$w.t.text config -state disabled
}

proc Review {} {
    global viewToBeDefined viewOption viewScout viewDoNaLD viewSasami
    set viewOption 0
    if {$viewScout} {
	set viewOption [expr $viewOption + 1]
    }
    if {$viewDoNaLD} {
	set viewOption [expr $viewOption + 2]
    }
    if {$viewSasami} {
	set viewOption [expr $viewOption + 4]
    }
    dumpeden $viewOption $viewToBeDefined
}

proc edenDefn {v n d} {
    set r [.eden.t.text tag ranges eden%$v]
    .eden.t.text config -state normal
    if [llength $r] {
	.eden.t.text delete eden%$v.first eden%$v.last
	.eden.t.text insert [lindex $r 0] $d [list $n eden%$v]
    } else {
	.eden.t.text insert end $d [list $n eden%$v]
    }
    .eden.t.text config -state disabled
}

proc scoutDefn {v d} {
    set r [.scout.t.text tag ranges scout%$v]
    .scout.t.text config -state normal
    if [llength $r] {
	.scout.t.text delete scout%$v.first scout%$v.last
	.scout.t.text insert [lindex $r 0] $d scout%$v
    } else {
	.scout.t.text insert end $d scout%$v
    }
    .scout.t.text config -state disabled
}

# This based on mkDialogue below... [Ash]
proc fileDialogue {fileName w winTitle} {
    global variantversion env

    catch {destroy $w}
    toplevel $w -class Dialog
    wm title $w "$variantversion: $winTitle"
    wm iconname $w "$winTitle"

    # Create two frames in the main window. The top frame will hold the
    # message and the bottom one will hold the buttons.  Arrange them
    # one above the other, with any extra vertical space split between
    # them.

    # Change $w.top to $w.t so that generic find diagloue can be used.
    # The find dialogue refences $w.t.text. [Charlie]

    frame $w.t -relief raised -border 1
    frame $w.bot -relief raised -border 1
    pack $w.t $w.bot -side top -fill both -expand yes

    text $w.t.text -state disabled -width 78 -height 31 \
	    -yscrollcommand "$w.t.scroll set" -background white \
	    -foreground black -font edencode
    scrollbar $w.t.scroll -command "$w.t.text yview"

    pack $w.t.scroll -side right -fill y
    pack $w.t.text -side top -expand yes -padx 3 -pady 3

    if [catch {open "$env(TKEDEN_LIB)/$fileName" r} fileId] {
	puts stderr "Cannot open $env(TKEDEN_LIB)/$fileName: $fileId"
    } else {
	$w.t.text config -state normal
	$w.t.text insert end [read $fileId]
	close $fileId
	$w.t.text config -state disabled
    }

    # Create as many buttons as needed and arrange them from left to right
    # in the bottom frame.  Embed the left button in an additional sunken
    # frame to indicate that it is the default button, and arrange for that
    # button to be invoked as the default action for clicks and returns in
    # the dialog.

    set windowName [string range $w 1 [string length $w]]

    set args "OK {Find {find $windowName}}" 

    if {[llength $args] > 0} {
	set arg [lindex $args 0]
	frame $w.bot.0 -relief sunken -border 1
	pack $w.bot.0 -side left -expand yes -padx 10 -pady 10
	button $w.bot.0.button -text [lindex $arg 0] \
		-command "[lindex $arg 1]; destroy $w"
	pack $w.bot.0.button -expand yes -padx 6 -pady 6
	bind $w <Return> "[lindex $arg 1]; destroy $w"
	focus $w

	set i 1
	foreach arg [lrange $args 1 end] {
	    # Changed the following to stop every button destroying 
            # the window [Charlie]

	    # button $w.bot.$i -text [lindex $arg 0] \
		   # -command "[lindex $arg 1]; destroy $w"
	     button $w.bot.$i -text [lindex $arg 0] \
		   -command "[lindex $arg 1]"
	    pack $w.bot.$i -side left -expand yes -padx 10
	    set i [expr $i+1]
	}
    }
    bind $w <Any-Enter> [list focus $w]
    focus $w
}

# Create the About key shortcuts dialogue box containing info... [Ash]
# See the Tcl text(n) man page for some of the information in the file
proc keys {} {
    fileDialogue "keys.txt" .keys "Key shortcuts"
}

proc changeLog {} {
    fileDialogue "change.log" .changeLog "ChangeLog"
}

proc credits {} {
    fileDialogue "credits.txt" .credits "Credits"
}

# this from Ousterhout "Tcl and the Tk toolkit" page 219
proc forAllMatches {w pattern script} {
    scan [$w index end] %d numLines
    for {set i 1} {$i < $numLines} {incr i} {
	$w mark set last $i.0
	while {[regexp -indices $pattern \
		[$w get last "last lineend"] indices]} {
	    $w mark set first \
		    "last + [lindex $indices 0] chars"
	    $w mark set last "last + 1 chars \
		    + [lindex $indices 1] chars"
	    uplevel $script
	}
    }
}

# Translate text file markup into formatted text by adding appropriate tags
# [Ash]
proc setTags {w} {
    $w.t.text config -state normal

    # Surround text denoting optional stuff with !@O and !@P.  See
    # Ousterhout "Tcl and the Tk toolkit" page 91 for information
    # about Tcl regular expressions
    forAllMatches $w.t.text {!@O[^!@]*!@P} {
	$w.t.text delete first "first + 3 char"
	$w.t.text delete "last - 3 char" last
	$w.t.text tag add optional first last
    }

    $w.t.text tag configure optional -foreground blue

    $w.t.text config -state disabled
}

proc edenQuickRef {} {
    fileDialogue "eden.txt" .edenQuickRef "Eden Quick Reference"
    setTags .edenQuickRef
}

proc scoutQuickRef {} {
    fileDialogue "scout.txt" .scoutQuickRef "Scout Quick Reference"
}

proc donaldQuickRef {} {
    fileDialogue "donald.txt" .donaldQuickRef "DoNaLD Quick Reference"
}

proc sasamiQuickRef {} {
    fileDialogue "sasami.txt" .sasamiQuickRef "Sasami Quick Reference"
}

proc aopQuickRef {} {
    fileDialogue "aop.txt" .aopQuickRef "Aop Quick Reference"
}

proc colourNames {} {
    fileDialogue "rgb.txt" .colourNames "Colour Names"
}

proc reinit {} {
    global env

    #set wins "[winfo children .] .";
    #foreach w $wins { destroy $w; }

    destroy .

    # This almost works: $_tkeden_win32_version is undefined tho :(
    source $env(TKEDEN_LIB)/edenio.tcl
}


# Create the About dialogue box containing version and other information [Ash]
proc about {} {
    global _tkeden_variant _tkeden_version _eden_svnversion _tkeden_web_site \
	    _dtkeden_isServer tcl_patchLevel tk_patchLevel \
	    _tkeden_win32_version env variantversion haveImg

    toplevel .about -class Dialog
    wm title .about "$variantversion: About"
    label .about.variant -text "This is $_tkeden_variant, version $_tkeden_version ($_eden_svnversion)"
    if {$_tkeden_variant == "dtkeden"} {
	if {$_dtkeden_isServer} {
	    label .about.isserver -text "Invoked in super-agent (server) mode"
	} else {
	    label .about.isserver -text "Invoked in agent (client) mode"
	}
    }

    label .about.copyright -text "Copyright (C) The University of Warwick.  All rights reserved"

    label .about.separator1 -text "--------------------------------"

    label .about.usage -text "Invoke $_tkeden_variant with the -u option for details of command line options usage"
    label .about.website -text "See $_tkeden_web_site for more information"

    label .about.separator2 -text "--------------------------------"
    label .about.diagnosis -text "This information may be useful when diagnosing problems:"

    if {$_tkeden_win32_version != "0.0"} {
      label .about.win32version -text "Win32 version V$_tkeden_win32_version"
    } else {
      label .about.win32version -text "Unix version"
    }

    label .about.libfiles -text "Library files are located in \n$env(TKEDEN_LIB)"
    label .about.tclversion -text "Tcl is version $tcl_patchLevel, Tk is version $tk_patchLevel\nTk Img package (PNG, JPEG...) is [expr {$haveImg ? {available} : {not available}}]"

    button .about.ok -text OK -command {destroy .about}

    if {$_tkeden_variant == "dtkeden"} {
	pack .about.variant .about.isserver .about.copyright \
		.about.separator1 .about.usage .about.website \
		.about.separator2 .about.diagnosis .about.win32version \
		.about.libfiles .about.tclversion \
		.about.ok -pady 5
    } else {
	pack .about.variant .about.copyright \
		.about.separator1 .about.usage .about.website \
		.about.separator2 .about.diagnosis .about.win32version \
		.about.libfiles .about.tclversion \
		.about.ok -pady 5
    }
}

# These long extensions have been re-thought from the original .e, .d,
# .s practice.  [Ash]
set fileTypes {
    {{All files} *}
    {{Eden files} {.eden}}
    {{DoNaLD files} {.donald}}
    {{Scout files} {.scout}}
    {{Sasami files} {.sasami}}
    {{Eddi files} {.eddi}}
    {{Script (multi-notation) files} {.script}}
}

proc include style {
    global variantversion fileTypes notation _tkeden_win32_version

    if {$_tkeden_win32_version == "0.0"} {
	# tk_getOpenFile -multiple true isn't possible until 8.4a2 on UNIX
	# and I can't find where to download that version (and it's alpha)
	set fileNames [tk_getOpenFile \
		-filetypes $fileTypes -parent . \
		-title "$variantversion: $style" ]
    } else {
	# we're on Windows: -multiple is possible
	set fileNames [tk_getOpenFile \
		-filetypes $fileTypes -parent . \
		-multiple true \
		-title "$variantversion: $style"]
    }

    foreach file $fileNames {
	# Change directory so that Eden include(...) is more likely to
	# work.  'cd [file dirname $file]' is the Tcl version, but
	# I've rewritten it in Eden so that the cwd() function will be
	# correctly re-evaluated
	eden "cd(dirname(\"$file\"));"

	if {$style == "Open"} {
	    set errCode [catch {set incFile [open $file r]} string]
	    if {$errCode == 0} {
		while {[gets $incFile line] >= 0} {
		    .text insert end "$line\n"
		}
		.text see end
		close $incFile
	    } else {
		tk_dialog .message "$variantversion: Warning" \
			"Cannot open file \"$file\"" warning 0 OK
	    }
	} elseif {$style == "Execute"} {
	    appendHist "%eden\n"
	    appendHist "include(\"$file\");\n"

	    # want to do this: todo "include(\"$file\");"
	    # but at evaluation level 0 (so that global variables such as
	    # $radiosBg can be found).  $file needs to be evaluated
	    # now, but the rest must not be.
	    # Using todo and not eden, as the Tcl eden command restores
	    # the currently active notation after use, and that isn't the
	    # semantics of include().
	    set cmd {todo "include(\"}
	    append cmd "$file"
	    append cmd {\");"}
	    uplevel #0 $cmd

	    # now switch back to current notation
	    appendHist "$notation\n"
	} else {
	    error {internal error: include style unknown}
	}

    }
}

proc save w {
    global variantversion saveAsReusable fileTypes

    set fileName [tk_getSaveFile -initialfile untitled.$w -parent . \
	    -title "$variantversion: Save $w As" -defaultextension $w \
	    -filetypes $fileTypes]
    if {$fileName != ""} {
	SaveToFile $w $fileName $saveAsReusable
    }
}

proc SaveToFile {w file executable} {
    global viewOption viewToBeDefined _tkeden_variant _dtkeden_isServer

    if {[catch {open $file w} fid]} {
	mkDialog .error "-aspect 300 -text \{$fid\}" {OK {}}
	tkwait visibility .error
	grab .error
    } else {
	case $w {
	all	{
	    dumpeden 63 0
	    dumpscout
	    dumpdonald
	    if {$_tkeden_variant == "dtkeden"} {
		if {$_dtkeden_isServer} {
		    dumpLSD
		}
	    }
	    if {$executable} {
		edenDefn autocalc eden ""
		puts $fid {autocalc = 0;}
		eden {tcl("set vp_in_use {"//vp_in_use(DFscreen)//"}");}
		global vp_in_use
		foreach vp $vp_in_use {
			edenDefn $vp eden ""
		}
		puts $fid %scout
		SaveScout $fid $executable
		if {$_tkeden_variant == "dtkeden"} {
		    if {$_dtkeden_isServer} {
			puts $fid %lsd
			Savelsd $fid $executable
		    }
		}
		puts $fid %donald
		SaveDonald $fid $executable
		puts $fid %eden
		SaveEden $fid $executable -omit masteragent scout donald system
	    } else {
		puts $fid %scout
		SaveScout $fid $executable
		if {$_tkeden_variant == "dtkeden"} {
		    if {$_dtkeden_isServer} {
			puts $fid %lsd
			Savelsd $fid $executable
		    }
		}
		puts $fid %donald
		SaveDonald $fid $executable
		puts $fid %eden
		SaveEden $fid $executable
	    }
	    if {$executable} {
		puts $fid {autocalc = 1;}
	    }
	    dumpeden $viewOption $viewToBeDefined
	}
	hist	{
	    puts $fid [.hist.t.text get 1.0 end] nonewline
	}
	eden	{
	    if {$executable} {
		dumpeden 63 0
		edenDefn autocalc eden ""
		puts $fid {autocalc = 0;}
		eden {tcl("set vp_in_use {"//vp_in_use(DFscreen)//"}");}
		global vp_in_use
		foreach vp $vp_in_use {
			edenDefn $vp eden ""
		}
		SaveEden $fid $executable -omit masteragent system
	    } else {
		dumpeden $viewOption $viewToBeDefined
		SaveEden $fid $executable
	    }
	    if {$executable} {
		puts $fid {autocalc = 1;}
		dumpeden $viewOption $viewToBeDefined
	    }
	}
	scout	{
	    dumpscout
	    puts $fid "%scout"
	    SaveScout $fid $executable
	}
	lsd	{
	    # This code should never happen in client and plain tkeden
	    dumpLSD
	    puts $fid "%lsd"
	    Savelsd $fid $executable
	}
	donald	{
	    dumpdonald
	    puts $fid "%donald"
	    SaveDonald $fid $executable
	}
	}
	close $fid
    }
}

proc SaveEden {fid executable args} {
    if {[lsearch $args -omit] == 0} {
	set args [lrange $args 1 end]
    }
    set lastline [lindex [split [.eden.t.text index end] "."] 0]
    for {set i 1} {$i <= $lastline} {incr i} {
	set in 1
	set tags [.eden.t.text tag names $i.0]
	foreach filter $args {
	    if {[lsearch $tags $filter] != -1} {
		set in 0
		break
	    }
	}
	if {$in} {
	    puts $fid [.eden.t.text get $i.0linestart $i.0lineend]
	}
    }
}

proc SaveScout {fid executable} {
    if $executable {
	foreach t [.scout.t.text tag names] {
	    if [string match scout%* $t] {
		set text [.scout.t.text get $t.first $t.last]
		set eq [string first = $text]
		if {$eq == -1} {
		    puts $fid $text nonewline
		} else {
		    puts $fid [string range $text 0 [expr $eq - 2]] nonewline
		    puts $fid {;}
		}
	    }
	}
	foreach t [.scout.t.text tag names] {
	    if [string match scout%* $t] {
		set text [.scout.t.text get $t.first $t.last]
		set eq [string first = $text]
		if {$eq != -1} {
		    puts $fid $text nonewline
		}
	    }
	}
    } else {
	puts $fid [.scout.t.text get 1.0 end] nonewline
    }
}

proc SaveDonald {fid executable} {
	for {set i 1} {$i <= [.donald.t.text index end]} {incr i} {
		set line [.donald.t.text get $i.0 "$i.0 lineend"]
		if {![string match AGENT* $line]} {
			puts $fid $line
		}
	}
}

if {$_tkeden_variant == "dtkeden"} {
    if {$_dtkeden_isServer} {
	proc Savelsd {fid executable} {
	    for {set i 1} {$i <= [.lsd.t.text index end]} {incr i} {
		set line [.lsd.t.text get $i.0 "$i.0 lineend"]
		puts $fid $line
	    }
	}
    }
}

proc TextSearch {w direction caseSensitive string} {
    if {[expr [string compare [.$w.t.text tag nextrange found 1.0] ""] \
        && [string compare $direction -forwards] == 0]} {
	if {[expr [string compare [.$w.t.text index insert] \
	           [.$w.t.text index found.first]] == 0]} {
	    .$w.t.text mark set insert [.$w.t.text index found.last]
	}
    }
    .$w.t.text tag remove found 1.0 end
    if {$caseSensitive} {
	set caseSwitch "-exact"
    } else {
	set caseSwitch "-nocase"
    }
    set index [.$w.t.text search $direction $caseSwitch -regexp \
	    -count len -- $string insert]
    if {[string length $index] > 0} {
	.$w.t.text mark set insert $index
	.$w.t.text see $index

	# This doesn't work on Linux as \c is an escape - rewritten [Ash]
	#.$w.t.text tag add found $index $index+$len\chars
	.$w.t.text mark set first $index
	.$w.t.text mark set last "$index + $len chars"
	.$w.t.text tag add found first last

	.$w.t.text tag configure found -background blue
    } else {
	bell
    }
}

proc find w {
    global variantversion

    catch {destroy .find}
    toplevel .find -class Dialog
    wm title .find "$variantversion: Find in $w"
    frame .find.top
    pack .find.top -fill both
    entry .find.top.e -relief sunken -textvariable searchString
    checkbutton .find.top.case -variable caseSensitive -text "case sensitive"

    pack .find.top.e .find.top.case -side left -padx 5
    frame .find.bot
    pack .find.bot -fill both 
    button .find.bot.forward -text "Forward" -underline 0 -width 8 \
	    -command "TextSearch $w -forwards \$caseSensitive \$searchString"
    button .find.bot.backward -text "Backward" -underline 0 -width 8 \
	    -command "TextSearch $w -backwards \$caseSensitive \$searchString"
    button .find.bot.cancel -text Cancel -command "destroy .find" -width 8
    bind .find <Alt-f> { .find.bot.forward invoke }
    bind .find <Alt-F> { .find.bot.forward invoke }
    bind .find <Alt-b> { .find.bot.backward invoke }
    bind .find <Alt-B> { .find.bot.backward invoke }
    pack .find.bot.forward -side left -expand yes -padx 5 -pady 5
    pack .find.bot.backward -side left -expand yes -padx 5 -pady 5
    pack .find.bot.cancel -side left -expand yes -padx 5 -pady 5
    tkwait visibility .find
    grab .find
}

set viewOption	0
set viewToBeDefined 0
set viewScout	0
set viewDoNaLD	0
set viewSasami	0

proc edenUpdate {} {
    global viewOption viewToBeDefined

    # save the position of the vertical scrollbar
    set yscroll [lindex [.eden.t.text yview] 0]
    # dumpeden is a tkeden Tcl command created in EX/ex.c
    dumpeden [expr $viewOption + 8] $viewToBeDefined
    .eden.t.text yview moveto $yscroll
}

# viewOption is called when the Rebuild button is pressed [Ash]
proc viewOption {} {
    global variantversion _tkeden_sasamiAvail

    catch {destroy .view}
    toplevel .view -class Dialog
    wm title .view "$variantversion: View Options"
    wm transient .view .eden
    frame .view.left
    pack .view.left -fill both -side left -expand yes
    label .view.left.name -justify left \
	    -text "Highlight to view:\ncontrol-click: individual items,\nshift-click: a range:"
    pack .view.left.name -side top -fill none -anchor nw
    scrollbar .view.left.scroll -command ".view.left.list yview"
    pack .view.left.scroll -side right -fill y
    listbox .view.left.list -yscroll ".view.left.scroll set" \
	-selectmode extended -relief sunken -width 20 -height 20 -setgrid yes
    pack .view.left.list -side left -fill both -expand yes
    frame .view.right
    pack .view.right -side right
    checkbutton .view.right.yet \
	    -text "with yet-to-be-defined variables" \
	    -variable viewToBeDefined
    checkbutton .view.right.scout \
	    -text "with translated Scout definitions" \
	    -variable viewScout
    checkbutton .view.right.donald \
	    -text "with translated DoNaLD definitions" \
	    -variable viewDoNaLD
    if {$_tkeden_sasamiAvail == "1"} {
	checkbutton .view.right.sasami \
		-text "with translated Sasami definitions" \
		-variable viewSasami
    }
    button .view.right.all -text "Select All" -width 12 -underline 0 \
	    -command { .view.left.list selection set 0 end }
    button .view.right.none -text "Clear All" -width 12 \
	    -command { .view.left.list selection clear 0 end }
    frame .view.right.ok -relief sunken -border 1
    button .view.right.ok.button -text OK -width 12 \
	    -command { Review; destroy .view; raise .eden }
    pack .view.right.ok.button -padx 10 -pady 10
    button .view.right.cancel -text Cancel -command "destroy .view" -width 12
    pack .view.right.all .view.right.none -side top -padx 5 -pady 5
    pack .view.right.yet .view.right.scout .view.right.donald \
	-side top -anchor sw
    if {$_tkeden_sasamiAvail == "1"} {
	pack .view.right.yet .view.right.sasami -side top -anchor sw
    }
    pack .view.right.ok .view.right.cancel -side top -padx 5 -pady 5
    bind .view <Return> { .view.right.ok.button invoke }
    bind .view <Alt-s> { .view.right.all invoke }
    bind .view <Alt-S> { .view.right.all invoke }
    # setupViewOptions is a tkeden Tcl command created in EX/ex.c
    setupViewOptions
    tkwait visibility .view
    grab .view
}

if {$_tkeden_variant == "dtkeden"} {
    if {$_dtkeden_isServer} {

	proc selectClients {} {
	    global clientSock sockName clientName variantversion
	    if {[llength $clientSock] <= 0} {
		tk_dialog .message "$variantversion: Message" "No connected client" warning -1 OK
		return
	    } else {    
		catch {destroy .select}
		toplevel .select -class Dialog
		wm title .select "$variantversion: Select Clients"
		frame .select.left
		pack .select.left -fill both -side left -expand yes
		label .select.left.name -text "Select clients to receive:"
		pack .select.left.name -side top -fill none -anchor nw
		scrollbar .select.left.scroll -command ".select.left.list yview"
		pack .select.left.scroll -side right -fill y
		listbox .select.left.list -yscroll ".select.left.scroll set" \
			-selectmode multiple -relief sunken -width 20 -height 10 -setgrid yes
		pack .select.left.list -side left -fill both -expand yes
		frame .select.right
		pack .select.right -side right
		# Something may be wrong here - emacs gets the formatting
		# wrong [Ash]
		button .select.right.all -text "Select All" -command {
		    .select.left.list selection set 0 end
		} -width 12
		button .select.right.none -text "Clear All" -command {
		    .select.left.list selection clear 0 end
		} -width 12
		button .select.right.ok -text OK -command {
		    global sockName
		    set text [.text get 1.0 end]
		    set selectedClients {}
		    foreach i [.select.left.list curselection] {
			set currClient [.select.left.list get $i]
			lappend selectedClients $sockName($currClient)
		    }
		    sendClientsSock $selectedClients $text
		    .text delete 1.0 end
		    destroy .select
		} -width 12
		button .select.right.cancel -text Cancel -command "destroy .select" -width 12
		pack .select.right.all .select.right.none -side top -padx 5 -pady 5
		pack .select.right.ok .select.right.cancel -side top -padx 5 -pady 5
		.select.left.list insert end "Own"
		foreach wsock $clientSock {
		    #          puts $clientName($wsock)
		    .select.left.list insert end $clientName($wsock)
		}
		tkwait visibility .select
		grab .select
	    }
	}

    }
}

# mkDialog w msgArgs list list ...
#
# Create a dialog box with a message and any number of buttons at
# the bottom.
#
# Arguments:
#    w -	Name to use for new top-level window.
#    msgArgs -	List of arguments to use when creating the message of the
#		dialog box (e.g. text, justifcation, etc.)
#    list -	A two-element list that describes one of the buttons that
#		will appear at the bottom of the dialog.  The first element
#		gives the text to be displayed in the button and the second
#		gives the command to be invoked when the button is invoked.
#
# @(#) mkDialog.tcl 1.1 94/08/10 15:35:00

proc mkDialog {w msgArgs args} {
    global variantversion

    catch {destroy $w}
    toplevel $w -class Dialog
    wm title $w "$variantversion: Dialog Box"
    wm iconname $w "Dialog"

    # Create two frames in the main window. The top frame will hold the
    # message and the bottom one will hold the buttons.  Arrange them
    # one above the other, with any extra vertical space split between
    # them.

    frame $w.top -relief raised -border 1
    frame $w.bot -relief raised -border 1
    pack $w.top $w.bot -side top -fill both -expand yes

    # Create the message widget and arrange for it to be centered in the
    # top frame.
    eval message $w.top.msg -justify center $msgArgs
    pack $w.top.msg -side top -expand yes -padx 3 -pady 3

    # Create as many buttons as needed and arrange them from left to right
    # in the bottom frame.  Embed the left button in an additional sunken
    # frame to indicate that it is the default button, and arrange for that
    # button to be invoked as the default action for clicks and returns in
    # the dialog.

    if {[llength $args] > 0} {
	set arg [lindex $args 0]
	frame $w.bot.0 -relief sunken -border 1
	pack $w.bot.0 -side left -expand yes -padx 10 -pady 10
	button $w.bot.0.button -text [lindex $arg 0] \
		-command "[lindex $arg 1]; destroy $w"
	pack $w.bot.0.button -expand yes -padx 6 -pady 6
	bind $w <Return> "[lindex $arg 1]; destroy $w"
	focus $w

	set i 1
	foreach arg [lrange $args 1 end] {
	    button $w.bot.$i -text [lindex $arg 0] \
		    -command "[lindex $arg 1]; destroy $w"
	    pack $w.bot.$i -side left -expand yes -padx 10
	    set i [expr $i+1]
	}
    }
    bind $w <Any-Enter> [list focus $w]
    focus $w
}



# Called when the user does %sasami open_display: called from Sasami render.c
proc sasamiWindow {width height} {
    global variantversion

    toplevel .sasami -width $width -height $height
    wm title .sasami "$variantversion: Sasami"

    frame .sasami.f

    togl .sasami.f.togl -width $width -height $height -double true \
    -privatecmap false -depth true -rgba true

    pack .sasami.f.togl -fill both -expand t
    pack .sasami.f -fill both -expand t

    bind .sasami.f.togl <Button-1> {sasamiB %x %y}
    bind .sasami.f.togl <B1-Motion> {sasamiB1Motion %x %y}

    bind .sasami.f.togl <Button-2> {sasamiB %x %y}
    bind .sasami.f.togl <B2-Motion> {sasamiB2Motion %x %y}
    # also allow control-drag: by default, Apple machines have only
    # one button
    bind .sasami.f.togl <Control-B1-Motion> {sasamiB2Motion %x %y}

    bind .sasami.f.togl <Button-3> {sasamiB %x %y}
    bind .sasami.f.togl <B3-Motion> {sasamiB3Motion %x %y}
    # also allow Apple-drag (Mod1 seems to be the Apple/Command key)
    bind .sasami.f.togl <Mod1-B1-Motion> {sasamiB3Motion %x %y}

    eden "~sasami_display_open = 1;"
}

set sasamiOldX 0
set sasamiOldY 0

proc sasamiB { x y } {
    global sasamiOldX sasamiOldY
    set sasamiOldX $x
    set sasamiOldY $y
}

# This is called when mouse button 1 is pressed and moved in the Sasami
# window: rotate about the X & Y axes.
proc sasamiB1Motion { x y } {
    global sasamiOldX sasamiOldY

    set diffX [expr $x - $sasamiOldX]
    set diffY [expr $y - $sasamiOldY]

    #.sasami.f.togl setXrot [expr [sasami_getXrot] - $diffY]
    #.sasami.f.togl setYrot [expr [sasami_getYrot] - $diffX]
    #.sasami.f.togl render

    eden "~sasami_change_camera_rot(-($diffY), -($diffX), 0);"

    set sasamiOldX $x
    set sasamiOldY $y
}

# This is called when mouse button 3 is pressed and moved in the Sasami
# window: zoom in and out on the Z axis.
proc sasamiB3Motion { x y } {
    global sasamiOldY

    set diffY [expr $y - $sasamiOldY]

    #.sasami.f.togl setZpos [expr [sasami_getZpos] - $diffY]
    #.sasami.f.togl render

    eden "~sasami_change_camera_pos(0, 0, -($diffY));"

    set sasamiOldX $x
    set sasamiOldY $y
}

# This is called when mouse button 2 is pressed and moved in the Sasami
# window: move camera position X and Y
proc sasamiB2Motion { x y } {
    global sasamiOldX sasamiOldY

    set diffX [expr $x - $sasamiOldX]
    set diffY [expr $y - $sasamiOldY]

    eden "~sasami_change_camera_pos($diffX * 0.01, -($diffY) * 0.01, 0);"

    set sasamiOldX $x
    set sasamiOldY $y
}

proc sasamiWindowClose {} {
    eden "~sasami_display_open = 0;"
    destroy .sasami
}

# If the Img package (PNG etc) is available then load it
set haveImg [expr ! [catch {package require Img}]]

if {$_tkeden_variant == "dtkeden"} {
    if {$_dtkeden_isServer} {
	source $env(TKEDEN_LIB)/server.tcl
    } else {
	source $env(TKEDEN_LIB)/client.tcl
    }
}

# This causes the text field in the input window to get focus when the
# application is double-clicked on the Mac.  Unfortunately I can't seem
# to get it to grab focus when the app is run from the Terminal.
# other things tried...
#wm focusmodel . active
#raise .
#grab set -global .
focus .text

#----------------------------------------------------------------------------------




