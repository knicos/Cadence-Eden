#
# $Id: client.tcl,v 1.6 2001/08/01 17:59:47 cssbz Exp $
#

# Just the client specific bits of tcl.  This is sourced when appropriate
# from edenio.tcl [Ash]

# Client-Server & Socket

# display the message of client connection on client window

# read script line-by-line sent by the server from socket and
# evoke eden evaluation 

proc read_sock {sock} {
     global receiveScripts EOF esvrSock 
     global EOS ECS serverReply variantversion
     set lineScript [gets $sock]
     set lineScript [string trimright $lineScript]
     if {[eof $sock]} {
        close $sock
        set esvrSock ""
#        tk_dialog .message "$variantversion: Network Message" \
#                   "The dtkeden server has been shut down and the connection has been closed." \
#                   warning 0 OK
        set choice [tk_dialog .message "$variantversion: Network Message" \
                   "The dtkeden server has been shut down and the connection has been closed." \
                   warning 1 "Stop" "Restart"]
        if {$choice == 0} {
          quit
        } else {
          if {$choice == 1} {
            restart
          } else {
            puts "Internal error - choice unknown - stopping"
            quit
          }
        }
     } else {
       if {$lineScript == $ECS} { 
          incr serverReply 
          return
       }
       if {$lineScript == $EOF || $lineScript == $EOS} {
         appendHist $receiveScripts
         # set receiveScripts "ScoutWinTrigged = 0;\n$receiveScripts\n"
         set receiveScripts "$receiveScripts\n"
         evaluate $receiveScripts
         if {$lineScript == $EOS} {
             #puts $esvrSock $EOS
             sendServer $EOS
         }
         set receiveScripts ""
       } else {
         set receiveScripts "$receiveScripts\n$lineScript"
       }
     }
}

proc sendUsrName {} {
  global usrName EOU getUsrName variantversion _tkeden_version
  set getUsrName 1
  sendServer1 "$EOU"
  set variantversion "dtkeden $_tkeden_version (client:$usrName)"
  wm title . "$variantversion: Input"
} 

proc loginUsrName {} {
   global esport eshost variantversion usrName

   toplevel .login
   wm title .login "$variantversion: Login"
   label .login.mess -text "You have connected to the dtkeden server \n\
                            with channel <[expr $esport-9000]>\
                            on host <$eshost>.\n\
                            Please login with your agent name"
   entry .login.usrName -relief sunken -textvariable usrName
   pack .login.mess .login.usrName -side top -padx 1m -pady 2m
   button .login.ok -text "OK" -command { 
         if {[string trim $usrName] != ""} {
            sendUsrName; destroy .login
         } else { bell }
   }
#   button .login.cancel -text "Cancel" -command { notLogin; destroy .login }
   button .login.clear -text "Clear" -command { set usrName ""; focus .login.usrName }
   pack .login.ok .login.clear -side left -expand 1
   bind .login.usrName <Return> { sendUsrName; destroy .login }
   focus .login.usrName
   grab .login
}

proc setupSocket {} {
   global usrName getUsrName
   global esvrSock eshost esport variantversion dtkedenloginname

   puts "$variantversion: connecting to dtkeden server channel <[expr $esport-9000]> on host <$eshost>..."
         
  # this is a synchronous connection: 
  # The command does not return until the server responds to the 
  #  connection request
   set errCode [catch {set esvrSock [socket $eshost $esport]} string]
  # puts $errCode
   

  # Setup monitoring on the socket so that when there is data to be 
  # read the proc "read_sock" is called
  if {$errCode == 0 } {

  #   puts "You are connected to tkServer in 'gem'...\n"
     fileevent $esvrSock readable [list read_sock $esvrSock]

  # configure channel modes
  # ensure the socket is line buffered so we can get a line of text 
  # at a time (Cos thats what the server expects)...
  # Depending on your needs you may also want this unbuffered so 
  # you don't block in reading a chunk larger than has been fed 
  #  into the socket
  # i.e fconfigure $esvrSock -blocking off

    fconfigure $esvrSock -buffering line -translation {crlf crlf}

  # set up our keyboard read event handler: 
  #   Vector stdin data to the socket
  #fileevent stdin readable [list read_stdin $esvrSock]

  # message indicating connection accepted and we're ready to go 

  # wait for and handle either socket or stdin events...
  #vwait eventLoop

   if {$dtkedenloginname != ""} {
      # login name was set using an EDEN command line argument
      set usrName $dtkedenloginname
      #after 3000
      sendUsrName
   } else {
      # need to ask the user what the login name should be
      loginUsrName
      vwait getUsrName
   }

  } else {
    puts "Fail to connect dtkeden server channel <[expr $esport-9000]> on host <$eshost>."
    puts "Using dtkeden as a stand-alone environment"
    .radios.send config -state disabled
    bell
  }
}

# read scripts from tkeden input window and send them to server

set sendServerDebug 0

proc sendServer { text } {
     global EOF ECS synchronize serverReply sendServerDebug
     # don't change synchronize which comes from ../Eden/main.client.c 
    if ($sendServerDebug) { puts "sendServerDebug: sendServer1" }
       if {$synchronize > 0} {
          while {$serverReply < 0} {
             vwait serverReply
          }
       }
    if ($sendServerDebug) { puts "sendServerDebug: sendServer2" }
     if {$synchronize > 0} { 
        set text "$text\n$ECS" 
     } else { 
        set text "$text\n$EOF" 
     }
    if ($sendServerDebug) { puts "sendServerDebug: sendServer3" }
     sendServer1 $text
}
proc sendServer1 {text} {
     global esvrSock usrName EOF currentNotation
     global synchronize serverReply sendServerDebug
    if ($sendServerDebug) { puts "sendServerDebug: sendServer4" }
     set isCancel -1
     set text "$text$usrName"
     if {$esvrSock == "" } { set isCancel [connectServer] }
     if {$isCancel == "0"} return
     if {$esvrSock != ""} {
#       set errCode [catch {.menu.accept invoke} string] ; # may need to handle error control???
#       puts "text $text"
       set errCode -1
       while {$errCode !=0} {
          getCurrentNotation
          # cannot change currentNotation below. It comes from EX/Exinit()
          set errCode [catch {puts $esvrSock "$currentNotation\n$text"} string]
          if {$errCode != 0} { 
            set isCancel [connectServer] 
            if {$isCancel == "0"} break
          }
       }
       if {$synchronize > 0} {
          incr serverReply -1
          while {$serverReply < 0} {
             vwait serverReply
          }
       }
     }
}

proc connectServer {} {
     global esvrSock variantversion
     set selectButton -1
     while { $selectButton != 0 } {
       set selectButton [tk_dialog .message "$variantversion: Connection Failed" \
          "Cannot connect to server" warning 0 Cancel Retry ]
          if {$selectButton == "1"} { 
             set esvrSock ""
             setupSocket 
             if {$esvrSock != ""}  break 
          }          
     }
     return $selectButton 
}

set EOF "@#$%EOF%$#@"
set EOU "@#$%EOU%$#@"
set EOS "@#$%EOS%$#@"
set ECS "@#$%ECS%$#@"
set serverReply 0

set receiveScripts ""
set usrName ""
set getUsrName 0

#set eshost "gem"
#set esport 7000
#puts "eshost $eshost esport $esport"
set esvrSock ""
setupSocket
