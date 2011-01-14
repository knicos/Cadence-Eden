#
# $Id: server.tcl,v 1.5 2001/08/01 18:00:40 cssbz Exp $
#

# Just the server specific bits of tcl.  This is sourced when appropriate
# from edenio.tcl [Ash]


set dtkedendebugtcl 0

# Client-Server & Socket

# display the message of client connection on client window

proc appendWindows {win text} {
    $win config -state normal
    $win insert end $text
    $win see end
    $win config -state disabled
}

# display the scripts sent by client on client window

# send scripts to each client

proc sendClientsSock {wsock msg} {
     global EOF clientsReply EOS synchronize dtkedendebugtcl

     if ($dtkedendebugtcl) { puts "server sendClientsSock msg $msg\n" }
     foreach sock $wsock {
        if {$sock == "Own"} {
            appendHist "$msg\n"
            evaluate "$msg\n"
        } else {
          # puts "test$sock"
          #if {$synchronize > 0} { # comment it due to the possibility of dead-lock
            # don't change the name of synchronize which comes from Eden/main.server.c
            # incr clientsReply -1
            # puts $sock "$msg\n$EOS"
            puts $sock "$msg\n$EOF"
          #} else {puts $sock "$msg\n$EOF"}
        }
     }
     #if {$synchronize > 0} { # comment it due to the possibility of dead-lock
     #   while {$clientsReply < 0} {
     #      vwait clientsReply
     #   }
     #}
}

proc sendOtherClients {exceptUsr msg} {
     #Don't change procedure's name which is connected to C programs of Eden.   
     global clientSock sockName dtkedendebugtcl

     if ($dtkedendebugtcl) { puts "server sendOtherClients msg $msg\n" }
  if {[llength $clientSock] != 0 } {
     set otherClients $clientSock
     if {[llength $exceptUsr] != 0 && [info exists sockName($exceptUsr)]} {
         set exceptClient [lsearch $clientSock $sockName($exceptUsr)]
         if {$exceptClient >= 0 } {
             set otherClients [lreplace $clientSock $exceptClient $exceptClient]
         }
     }
     sendClientsSock $otherClients $msg
  }
}


proc sendClient {wusr msg} {
     #Don't change procedure's name which is connected to C programs of Eden.
     global sockName dtkedendebugtcl

     if ($dtkedendebugtcl) { puts "server sendClient msg $msg\n" }
     if {[info exists sockName($wusr)]} {
        sendClientsSock \{$sockName($wusr)\} $msg
     }
}


# read scripts from tkeden input window and send them to each client

proc sendScripts {} {
     global clientSock dtkedendebugtcl

     set text [.text get 1.0 end]
#     .menu.accept invoke
#     puts "send $text"

     if ($dtkedendebugtcl) { puts "server sendScripts text $text\n" }

     sendClientsSock $clientSock $text
}

# read each definition into send window until EOF is found and then 
# send all to each client

proc doService {sock addr msg} {
    global EOF EOU clientSock proType clientName sockName receivedMessage
    global oldNotation oldAgentName oldAgentType
    global EOS clientsReply synchronize ECS serverJobID JOBID
    set msg [string trimright $msg]
    if {$msg == "" || $msg == "\n"} return
    set msg1 [string rang $msg 0 10]
    if {$msg1 == $EOS} {
       #if {$synchronize > 0} {incr clientsReply}
       return
    }
    if {$msg1 != $EOF && $msg1 !=$EOU && $msg1 != $ECS}  {
       append receivedMessage($sock) "$msg\n"
       return
    }
    if {$msg1 == $EOU} { ;# agent's name input by a client
        set loginClientName [string rang $msg 11 end]
#       getOldNotation
#       if {[llength $clientSock] == 0} {
#          evaluate ">>\n%eden\nCLIENT_LIST = \[\];\n$oldAgentType$oldAgentName\n$oldNotation\n"
#       } CLIENT_LIST has been declared in Scout.init.e  */
       evaluate ">>\n%eden\nappend CLIENT_LIST, \"$loginClientName\";\n$oldAgentType$oldAgentName\n$oldNotation\n"
       lappend clientSock $sock 
       set clientName($sock) $loginClientName
       set sockName($loginClientName) $sock
       set receivedMessage($sock) ""
       return
    }
    set text $receivedMessage($sock)
    set sendClientName $clientName($sock)
    if {$msg1 == $EOF || $msg1 == $ECS} {
       if {$proType == "2" || $proType == "0"} {
         # getOldNotation
         incr JOBID
         appendHist ">~$sendClientName\n"
         appendHist $text
         appendHist "$oldAgentType$oldAgentName\n"
         appendHist "$oldNotation\n"
         # cannot change oldNotation, oldAgentType & oldAgentName. it comes from EX/EXinit().
         # puts ">~$sendClientName\n$text\n%eden\ntcl(\"setServerJobID $JOBID\");\n$oldAgentType$oldAgentName\n$oldNotation\n"
         evaluate ">~$sendClientName\n$text\n%eden\ntcl(\"setServerJobID $JOBID\");\n$oldAgentType$oldAgentName\n$oldNotation\n"
         if {$proType == "2"} {
            set otherClients $clientSock
            set delClient [lsearch $clientSock $sock]
            if {$delClient >= 0 } {
               set otherClients [lreplace $clientSock $delClient $delClient]
            }
            sendClientsSock $otherClients $text
         }
       } elseif {$proType == "1" } {
               set text ">>$sendClientName\n$text\n>>\n"
              .text insert end $text
       } elseif {$proType == "3" } {
             set text ">>$sendClientName\n$text\n>>\n"
             # puts $text
             # getOldNotation
             appendHist $text
             evaluate "$text\n$oldAgentType$oldAgentName\n$oldNotation\n"; # cannot change oldNotation. it comes from EX/EXinit().
       }
       if {$msg1 == $ECS} { 
          #puts "5000"
          #after 50000 # slow down client's speed to let server do service
          #puts "$serverJobID  $JOBID"
          while {$serverJobID != $JOBID} {
             vwait  serverJobID
          }
          puts $sockName($sendClientName) $ECS
       }
       set receivedMessage($sock) ""
       return
    } 
}


# Handles the input from the client and  client shutdown

proc  svcHandler {sock addr} {
  global clientSock clientName sockName receivedMessage
  global oldNotation oldAgentName oldAgentType
  set msg [gets $sock]    ;# get the client packet
  if {[eof $sock]} {    ;# client gone or finished
     appendWindows .client.t.text "[fconfigure $sock -peername] disconnected \
                    at [clock format [clock seconds]]\n"
     close $sock        ;# release the servers client channel
     set delClient [lsearch $clientSock $sock]
     if {$delClient >= 0 } {
       set clientSock [lreplace $clientSock $delClient $delClient]
#       getOldNotation
       evaluate ">>\n%eden\ndelete CLIENT_LIST, $delClient+1;\
                 \n$oldAgentType$oldAgentName\n$oldNotation\n"
       unset sockName($clientName($sock))
       unset clientName($sock)
       unset receivedMessage($sock)
     }
     
  } else {
    doService $sock $addr $msg
  }
}

proc setServerJobID { value } {
   global serverJobID
   set serverJobID $value
}

# Accept-Connection handler for Server. 
# called When client makes a connection to the server
# Its passed the channel we're to communicate with the client on, 
# The address of the client and the port we're using
#
# Setup a handler for (incoming) communication on 
# the client channel - send connection Reply and log connection

proc connect {sock addr port} {
  
  # if {[badConnect $addr]} {
  #     close $sock
  #     return
  # }

  # Setup handler for future communication on client socket
  global clientSock 
  set sockPeerName [fconfigure $sock -peername]
  # lappend clientSock $sock
  #  evaluate "append CLIENT_LIST, \"$sock\";"
  fileevent $sock readable [list svcHandler $sock $addr]

  # Note we've accepted a connection (show how get peer info fm socket)
  appendWindows .client.t.text "Accept from $sockPeerName\n\
                  Accepted connection from $addr at [clock format [clock seconds]]\n"

  # Read client input in lines, disable blocking I/O
  fconfigure $sock -buffering line -blocking 0 

  # Send Acceptance string to client
  #  puts $sock "$addr:$port, You are connected to the echo server."
  #  puts $sock "It is now [clock format [clock seconds]]"

}

# Create a server socket on port $svcPort. 
# Call proc connect when a client attempts a connection.
set clientSock {}
set sockName(Own) "Own"
set EOF "@#$%EOF%$#@"
set EOU "@#$%EOU%$#@"
set EOS "@#$%EOS%$#@"
set ECS "@#$%ECS%$#@"
set clientsReply 0
set JOBID 0
set serverJobID 0
socket -server connect $svcPort ;#  cannot change the name of svcPort as it comes from EXinit() in EX/ex.c
puts "$variantversion: server socket setup: channel <[expr $svcPort-9000]> on host <$eshost>"
#puts "so far so good..."
#update
#vwait events    ;# handle events till variable events is set
