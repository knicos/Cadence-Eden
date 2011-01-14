%eden #########################################
##
## AnGel - Ant's Graphical Environment Language
## ( A prototype for Gel )
##
###############################################


gel_debug = 0;

proc gel_debug_trigger : gel_debug {
   if (gel_debug)
      writeln("gel: debugging is on");
}

##
## Global constants
##

GEL_WINDOW = 1;
GEL_FRAME = 2;
GEL_BUTTON = 3;
GEL_TEXTBOX = 4;
GEL_LABEL = 5;
GEL_CHECKBUTTON = 6;
GEL_RADIOBUTTON = 7;
GEL_SCROLLBAR = 8;
GEL_SCALE = 9;
GEL_SCOUT = 10;
GEL_HTML = 11;
GEL_IMAGE = 12;
GEL_LISTBOX = 13;

GEL_LIST = 32;
GEL_OTHER = 33;


##
## A limited symbol table
##

gel_window_list=[];
gel_frame_list=[];

##
## Escaping procedures (tcl and eden)
##

tcl("proc escapequotes {mystr} { regsub -all {\\\"} $mystr {\\\\\"} mystr\nreturn $mystr }");

func escape_tcl_chars {
  para str_in;
  auto i, str_out;
  str_out = "";
  for (i=1; i<=str_in#; i++) {
    switch (str_in[i]) {
      case '\\':
        str_out = str_out // "\\\\";
        break;
      case '"':
      case '$':
      case ';':
      case '{':
      case '}':
      case '[':
      case ']':
        str_out = str_out // '\\';
      default:
        str_out = str_out // str_in[i];
    }
  }
  return str_out;
}

##
## Procedures for creating/removing components
##

proc gel_create_window {
   para name;
   append gel_window_list, name;
   tcl("if {![winfo exists ."//name//"]} {
           toplevel ."//name//"
           wm protocol ."//name//" WM_DELETE_WINDOW {interface {"//name//"_visible = 0;} }
	   bind ."//name//" <Configure> {interface \""//name//"_width_bak=%w;"//name//"_width=%w;"//name//"_height_bak=%h;"//name//"_height=%h;\n\"} }");
   `name//"_tk"` = "."//name;
}

proc gel_remove_window {
   para name;
   auto i;
   for (i=1; i<=gel_window_list; i++) {
      if (name == gel_window_list[i]) {
         delete gel_window_list, i;
	 break;
      }
   }
   tcl(" if {[winfo exists ."//name//"]} { destroy ."//name//" }");
   `name//"_tk"` = @;
}

proc gel_create_frame {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @)
      tcl("if {![winfo exists "//component//"]} {
         frame "//component//" }");
   `name//"_tk"` = component;   
}

proc gel_remove_component {
   para name;
   auto component;
   component = `name//"_tk"`;
   if (component != @)
      tcl("if {[winfo exists "//component//"]} {
         pack forget "//component//" }");
   `name//"_tk"` = @;   
}

proc gel_create_button {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @) {
      tcl("if {![winfo exists "//component//"]} {
         button "//component//"
         bind "//component//" <Enter> {interface {"//name//"_mouseover=1;\n}}
         bind "//component//" <Leave> {interface {"//name//"_mouseover=0;\n}}
         bind "//component//" <ButtonPress> {interface {"//name//"_mouseclick=1;\n}}
         bind "//component//" <ButtonRelease> {interface {"//name//"_mouseclick=0;\n}} }");
   }
   `name//"_tk"` = component;   
}

proc gel_create_textbox {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @) {
      tcl("if {![winfo exists "//component//"]} {
         text "//component//"
         bind "//component//" <KeyRelease> {interface \""//name//"_text_bak=\\\"[escapequotes [%W get 1.0 end-1c]]\\\";"//name//"_text="//name//"_text_bak;\n\"} }");
   }
   `name//"_tk"` = component;
}

proc gel_create_label {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @) {
      tcl("if {![winfo exists "//component//"]} {
         label "//component//" }");
   }
   `name//"_tk"` = component;   
}

proc gel_create_checkbutton {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @) {
      tcl("if {![winfo exists "//component//"]} {
         checkbutton "//component//" -command {interface { if("//name//"_value) {"//name//"_value=0;} else {"//name//"_value=1;}\n}}
         bind "//component//" <Enter> {interface {"//name//"_mouseover=1;\n}}
         bind "//component//" <Leave> {interface {"//name//"_mouseover=0;\n}}
          }");
   }
   `name//"_tk"` = component;   
}

proc gel_create_radiobutton {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @) {
      tcl("if {![winfo exists "//component//"]} {
         radiobutton "//component//" -command {interface {`"//name//"_variable`="//name//"_value;\n}}
         bind "//component//" <Enter> {interface {"//name//"_mouseover=1;\n}}
         bind "//component//" <Leave> {interface {"//name//"_mouseover=0;\n}} }");
   }
   `name//"_tk"` = component;   
}

proc gel_create_scrollbar {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @) {
      tcl("if {![winfo exists "//component//"]} {
         scrollbar "//component//" }");
   }
   `name//"_tk"` = component;   
}

tcl("proc gel_update_scale_value {name newvalue} {interface \"${name}_value_bak=$newvalue; ${name}_value=${name}_value_bak;\n\"}");

proc gel_create_scale {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @) {
      tcl("if {![winfo exists "//component//"]} {
         scale "//component//" -command {gel_update_scale_value "//name//"} }");
   }
   `name//"_tk"` = component;   
}

proc gel_create_scout {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @) {
      tcl("if {![winfo exists "//component//"]} {
         frame "//component//" }");
   }
   `name//"_tk"` = component;   
}

proc gel_create_html {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @) {
      tcl("if {![winfo exists "//component//"]} {
         html "//component//" -fontcommand FontCmd -imagecommand ImgCmd -hyperlinkcommand HrefCmd -unvisitedcolor blue -visitedcolor purple}");
   }
   `name//"_tk"` = component;   
}

proc gel_create_image {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @)
      tcl("if {![winfo exists "//component//"]} {
         canvas "//component//" }");
   `name//"_tk"` = component;   
}

proc gel_create_listbox {
   para name, parent;
   auto component;
   component = `parent//"_tk"` // "." // name;
   if (component != @)
      tcl("if {![winfo exists "//component//"]} {
         listbox "//component//" -exportselection 0
         bind "//component//" <<ListboxSelect>> {interface \""//name//"_selecteditems_bak=gel_listbox_getselecteditems(\\\"["//component//" curselection]\\\","//name//"_items); "//name//"_selecteditems = "//name//"_selecteditems_bak;\n\"} }");
   `name//"_tk"` = component;   
}

func gel_listbox_getselecteditems {
   para idxstr, items;
   auto selecteditems,j,item,itemint;
   selecteditems = [];
   item="";
   for (j=1; j<=idxstr#; j++) {
      if (idxstr[j] == ' ') {
         itemint = int(item) + 1;
         if (itemint > 0 && itemint <= items#)
            append selecteditems, items[itemint];
         item = "";
      }
      else
         item = item // str(idxstr[j]);
   }
   if (item != "") {
      itemint = int(item) + 1;
      if (itemint > 0 && itemint <= items#)
         append selecteditems, items[itemint];
   }
   return selecteditems;
}

##
## Procedures for updating the component
##

proc gel_update_property {
   para name, property;
   auto component;
   component = `name//"_tk"`;
   if (component != @) {
      if (`name//"_"//property` != @)
         tcl(component//" configure -"//property//" {"//str(`name//"_"//property`)//"}");
   }
}

proc gel_update_window_title {
   para name;
   auto component;
   component = `name//"_tk"`;
   if (component != @) {
      tcl("wm title ."//name//" {"//`name//"_title"`//"}");
   }
}

proc gel_update_window_geometry {
   para name;
   auto component, width, height;
   component = `name//"_tk"`;
   width = `name//"_width"`;
   height = `name//"_height"`;
   if (component != @ &&
       width > 0 && height > 0 &&
       (width != `name//"_width_bak"` || height != `name//"_height_bak"`)) {
      tcl("wm geometry ."//name//" "//str(width)//"x"//str(height));
      `name//"_width_bak"` = width;
      `name//"_height_bak"` = height;
   }
}

proc gel_update_window_visible {
   para name;
   auto component, visible;
   component = `name//"_tk"`;
   visible = `name//"_visible"`;
   if (component != @ && visible != @) {
      if (visible == 0)
         tcl("if {[winfo exists "//component//"]} {
            wm withdraw ."//name//" }");
      else
         tcl("if {[winfo exists "//component//"]} {
            wm deiconify ."//name//" }");
   }
}

proc gel_update_content {
   para name;
   auto content, i, oldparent, oldparentcontent, elementno;
   oldcontent = `name//"_content_bak"`;
   content = `name//"_content"`;
   
   if (content != @ && type(content)=="list") {

      ## remove components from other containers
      for (i=1; i<=content#; i++) {
         oldparent = &`content[i]//"_parent"`;

         ## if a member is found which has not yet been put in the container
	 if (*oldparent != name) {

            ## delete it from its previous container
	    oldparentcontent = &`*oldparent//"_content"`;
	    for (j=1; j<=*oldparentcontent#; j++)
	       if ((*oldparentcontent)[j] == content[i])
	          delete *oldparentcontent, j;
		  
            ## remove it from the screen
	    *oldparent = @;
	 } 
      }

      ## remove previous contents of this container
      if (oldcontent != @ && type(oldcontent)=="list") {
         for (i=1; i<=oldcontent#; i++) {
            `oldcontent[i]//"_parent"` = @;
	 }
      }
      
      eager();
      
      ## add all the components to this container
      for (i=1; i<=content#; i++) {
         `content[i]//"_parent"` = name;
      }

   }
 
   ## make a copy of the contents (to be used next update)
   `name//"_content_bak"` = content;
   
}

proc gel_update_pack {
   para name;
   auto parentcontent, i, component, packstr;
   parentcontent = &`name//"_content"`;
   
   if (*parentcontent != @ && type(*parentcontent)=="list" && `name//"_tk"` != @) {
      if (gel_debug)
         writeln("repacking "//str(*parentcontent));
      for (i=1; i<=*parentcontent#; i++) {
         component = `(*parentcontent)[i]//"_tk"`;
	 if (component != @)
	    tcl("pack forget "//component);
      }
      for (i=1; i<=*parentcontent#; i++) {
         component = `(*parentcontent)[i]//"_tk"`;
	 if (component != @) {
	    packstr = "pack " // component;
	    if (`(*parentcontent)[i]//"_side"` != @)
	       packstr = packstr // " -side " // str(`(*parentcontent)[i]//"_side"`);
	    if (`(*parentcontent)[i]//"_anchor"` != @)
	       packstr = packstr // " -anchor " // str(`(*parentcontent)[i]//"_anchor"`);
	    if (`(*parentcontent)[i]//"_expand"` != @)
	       packstr = packstr // " -expand " // str(`(*parentcontent)[i]//"_expand"`);
	    if (`(*parentcontent)[i]//"_fill"` != @)
	       packstr = packstr // " -fill " // str(`(*parentcontent)[i]//"_fill"`);
	    if (`(*parentcontent)[i]//"_ipadx"` != @)
	       packstr = packstr // " -ipadx " // str(`(*parentcontent)[i]//"_ipadx"`);
	    if (`(*parentcontent)[i]//"_ipady"` != @)
	       packstr = packstr // " -ipady " // str(`(*parentcontent)[i]//"_ipady"`);
	    if (`(*parentcontent)[i]//"_padx"` != @)
	       packstr = packstr // " -padx " // str(`(*parentcontent)[i]//"_padx"`);
	    if (`(*parentcontent)[i]//"_pady"` != @)
	       packstr = packstr // " -pady " // str(`(*parentcontent)[i]//"_pady"`);
	    tcl(packstr);
	 }
      }
   }
}


proc gel_update_textbox_text {
   para name;
   auto component;
   if (`name//"_text_bak"` != `name//"_text"`) {
      component = `name//"_tk"`;
      if (component != @) {
         tcl("if {[winfo exists "//component//"]} {"//component//" delete 1.0 end
         "//component//" insert end \""//escape_tcl_chars(`name//"_text"`)//"\"}");
         `name//"_text_bak"` = `name//"_text"`;
      }
   }
}


proc gel_update_scrollbar {
   para name;
   auto component, scrolls_component, orient;
   component = `name//"_tk"`;
   scrolls_component = ``name//"_scrolls"`//"_tk"`;
   scroll_direction = `name//"_orient"`=="horizontal" ? "x" : "y";
   if (! AOP_isMemberOf(name//"_scrolls", symboldetail(&``name//"_scrolls"`//"_tk"`)[4]))
      execute(`name//"_scrolls"`//"_tk ~> ["//name//"_scrolls];");
   if (component != @ && scrolls_component != @) {
      tcl("if {[winfo exists "//component//"] && [winfo exists "//scrolls_component//"]} {
      "//component//" configure -command {"//scrolls_component//" "//scroll_direction//"view}
      "//scrolls_component//" configure -"//scroll_direction//"scrollcommand {"//component//" set} }");
   }
}


proc gel_update_scale_value {
   para name;
   auto component;
   if (`name//"_value_bak"` != `name//"_value"`) {
      component = `name//"_tk"`;
      if (component != @) {
         tcl("if {[winfo exists "//component//"]} {
         "//component//" set "//str(`name//"_value"`)//"}");
         `name//"_value_bak"` = `name//"_value"`;
      }
   }
}


proc gel_update_scout_display {
   para name;
   auto component, display;
   component = `name//"_tk"`;
   display = `name//"_display"`;
   
   if (component != @ && display != @) {
      execute("
         proc gel_trigger_"//name//"_update_display : "// display //" {
            DisplayScreen(&"// display //",\""//substr(component,2,component#)//"\");
         }
      ");
   }
}


proc gel_update_html_text {
   para name;
   auto component;
   component = `name//"_tk"`;
   if (component != @) {
      tcl("if {[winfo exists "//component//"]} {
      "//component//" clear
      "//component//" parse \""//escape_tcl_chars(`name//"_text"`)//"\" }");
   }
}


proc gel_update_image {
   para name;
   auto component, file;
   component = `name//"_tk"`;
   file = `name//"_file"`;
   /* check whether path is relative */
   if (file[1] != '/') {
      `name//"_file"` = cwd() // "/" // file;
      return;
   }
   if (component != @ && file != @) {
      tcl("if {[winfo exists "//component//"]} {
      "//component//" delete all
      image create photo "//component//".photo -file {"//file//"}
      "//component//" create image 0.0 0.0 -image "//component//".photo -anchor nw }");
      `name//"_width"` = tcl("image width "//component//".photo");
      `name//"_height"` = tcl("image height "//component//".photo");
   }
}

proc gel_update_listbox_items {
   para name;
   auto component, items, items_str, i;
   component = `name//"_tk"`;
   items = `name//"_items"`;
   if (component != @) {
      items_str = "";
      for (i=1; i<=items#; i++)
         items_str = items_str // " {"//str(items[i])//"}";
      tcl("if {[winfo exists "//component//"]} {
         "//component//" delete 0 end
         "//component//" insert end "//items_str//" }");
   }
}

proc gel_update_listbox_selecteditems {
   para name;
   auto component, items, selecteditems, i, j;
   component = `name//"_tk"`;
   items = `name//"_items"`;
   selecteditems = `name//"_selecteditems"`;
   if (component != @ && selecteditems != `name//"_selecteditems_bak"`) {
      if (type(selecteditems) == "list" && type(items) == "list") {
      
         tcl("if {[winfo exists "//component//"]} {
            "//component//" selection clear 0 end }");
         
         for (i=1; i<=selecteditems#; i++) {
            for (j=1; j<=items#; j++)
               if (selecteditems[i] == items[j])
                  tcl("if {[winfo exists "//component//"]} {
                     "//component//" selection set "//str(j-1)//" }");
         }
      }
      `name//"_selecteditems_bak"` = selecteditems;
   }
}

proc gel_update_bind {
   para name;
   auto component, bindings, bindings_bak, i;
   component = `name//"_tk"`;
   bindings = `name//"_bindings"`;
   bindings_bak = `name//"_bindings_bak"`;
   if (component != @ && bindings != bindings_bak) {
      for (i=1; i<=bindings_bak#; i++) {      
         tcl("if {[winfo exists "//component//"]} { bind "//component//" "//bindings_bak[i][1]//" {} }");
      }
      for (i=1; i<=bindings#; i++) {
         tcl("if {[winfo exists "//component//"]} { bind "//component//" "//bindings[i][1]//" {interface \""//escape_tcl_chars(bindings[i][2])//"\"} }");
      }
      `name//"_bindings_bak"` = `name//"_bindings"`;
   }
}
