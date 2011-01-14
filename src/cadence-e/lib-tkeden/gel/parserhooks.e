%eden

##
## Parser procedures
##


proc gel_query {
   para name;
   auto i, j;
   
   ## Print out information if name exists
   if (`name//"_type"` != @ ) {
      writeln(name // " = " // gel_type_name(`name//"_type"`) // "();");
      return;
   }
   
   if (name == "component" || name == "components") {
      write("gel components:");
      for (i=1; i<=gel_components#; i++) {
         write(" "//gel_components[i][1]);
      }
      write("\n");
      return;
   }

   if (name == "property" || name == "properties") {
      write("gel standard properties:");
      for (i=1; i<=gel_standard_properties#; i++) {
         write(" "//gel_standard_properties[i][1]);
      }
      write("\n");
      write("gel pack properties:");
      for (i=1; i<=gel_pack_properties#; i++) {
         write(" "//gel_pack_properties[i][1]);
      }
      write("\n");
      return;
   }
   
   ## Else look for information about specific type
   for (i=1; i<=gel_components#; i++) {
      if (name == gel_components[i][1]) {
         write("component "//name//" has special properties:");
         for (j=1; j<=gel_components[i][2]#; j++) {
	    write(" "//gel_components[i][2][j][1]);
	 }
	 write("\n");
	 return;
      }
   }
}


proc gel_parse_definition {
   para name, geltype, format, properties;
   auto i;

   if (format == 0 && type(name)=="list") {  ## single property definition
      if (gel_debug)
         writeln("defining "//name[1]//"."//name[2]//" as "//str(properties));
      execute(name[1]//"_"//name[2]//" is "//str(properties)//";");
   }
   else if (geltype == 33) {
      if (gel_debug)
         writeln("defining "//name//" as eden expression: "//str(properties));
      execute(name//" is "//str(properties)//";");
   }
   else {
      if (gel_debug)
         writeln("defining "//name//" of type "//str(geltype)//" with properties "//str(properties));
   
      gel_create(name,geltype);
   
      switch(format) {
      	  case 0: ## reserved for future funcionality
      	  	  break;
      	  case 1: ## properties defined in an argument list
      	  	  break;
          case 2: ## properties defined by name
              for (i=1; i<=properties#; i++)
	             execute(name//"_"//properties[i][1]//" is "//str(properties[i][2])//";");
	          break;
	  }
   }
}


## Get type name
func gel_type_name {
   para typeid;
   if (typeid > 0 && typeid <= gel_components#)
      return gel_components[typeid][1];
   else {
      switch (typeid) {
         case 32: return "list";
	 case 33:
	 default: return "other";
      }
   }
}


## Type information
gel_components = [

##GEL_WINDOW
[ "window", [
  ["title","","gel_update_window_title"],
  ["width","","gel_update_window_geometry"],
  ["height","","gel_update_window_geometry"],
  ["content","","gel_update_content"],
  ["visible","","gel_update_window_visible"] ] ],

##GEL_FRAME
[ "frame", [
  ["content","","gel_update_content"] ] ],
  
##GEL_BUTTON
[ "button", [
  ["text","",0] ] ],

##GEL_TEXTBOX
[ "textbox", [
  ["text","","gel_update_textbox_text"],
  ["state","Editable (\"normal\") or read-only (\"disabled\").",0],
  ["wrap","Line wrap mode: \"none\", \"char\" or \"word\".",0] ] ],

##GEL_LABEL
[ "label", [
  ["text","",0] ] ],

##GEL_CHECKBUTTON
[ "checkbutton", [
  ["text","",0],
  ["variable","",0],
  ["offvalue","",0],
  ["onvalue","",0],
  ["indicatoron","",0],
  ["selectcolor","",0] ] ],

##GEL_RADIOBUTTON
[ "radiobutton", [
  ["text","",0],
  ["variable","",0],
  ["value","",0],
  ["indicatoron","",0],
  ["selectcolor","",0] ] ],

##GEL_SCROLLBAR
[ "scrollbar", [
  ["scrolls","","gel_update_scrollbar"],
  ["activerelief","",0],
  ["elementborderwidth","",0],
  ["jump","",0],
  ["orient","",0],
  ["repeatdelay","",0],
  ["repeatinteval","",0],
  ["takefocus","",0],
  ["troughcolor","",0] ] ],

##GEL_SCALE
[ "scale", [
  ["bigincrement","",0],
  ["digits","",0],
  ["from","",0],
  ["label","",0],
  ["length","",0],
  ["orient","",0],
  ["repeatdelay","",0],
  ["repeatinteval","",0],
  ["resolution","",0],
  ["showvalue","",0],
  ["sliderlength","",0],
  ["sliderrelief","",0],
  ["state","",0],
  ["takefocus","",0],
  ["tickinteval","",0],
  ["to","",0],
  ["troughcolor","",0],
  ["value","","gel_update_scale_value"] ] ],

##GEL_SCOUT
[ "scout", [
  ["display","","gel_update_scout_display"] ] ],

##GEL_HTML
[ "html", [
  ["text","The html content of the widget.","gel_update_html_text"],
  ["appletcommand","",0],
  ["fontcommand","",0],
  ["formcommand","",0],
  ["framecommand","",0],
  ["hyperlinkcommand","",0],
  ["imagecommand","",0],
  ["isvisitedcommand","",0],
  ["rulerelief","",0],
  ["scriptcommand","",0],
  ["tablerelief","",0],
  ["unvisitedcolor","",0],
  ["underlinehyperlinks","",0],
  ["visitedcolor","",0] ] ],

##GEL_IMAGE
[ "image", [
  ["file","The image file defined as a string.","gel_update_image"] ] ],

##GEL_LISTBOX
[ "listbox", [
  ["selectmode","Specifies one of several styles for manipulating the selection. Accepted values: single, browse, multiple, or extended.",0],
  ["items","Specifies the items in the listbox. Must be a list of strings.","gel_update_listbox_items"],
  ["selecteditems","Specifies the selected items. Must be a subset of items.","gel_update_listbox_selecteditems"] ] ]
  
];

## Standard properties
gel_standard_properties = [
  ["activebackground","Specifies the background color to use when drawing active component. A component is active if the mouse cursor is positioned over the component and pressing a mouse button will cause some action to occur."],
  ["activeborderwidth","Specifies a non-negative value indicating the width of the 3-D border drawn around active elements."],
  ["activeforeground","Specifies the foreground color to use when drawing active elements."],
  ["anchor","Specifies how the text or bitmap is to be displayed in the component. Must be one of the values n, ne, e, se, s, sw, w, nw, or center."],
  ["background","Specifies the normal background color to use when displaying the component."],
  ["bitmap","Specifies a bitmap to display in the widget. [not implemented]"],
  ["borderwidth","Specifies the width in pixels of the border around the outside of the component."],
  ["cursor","Specifies the mouse cursor to be used for the component."],
  ["disabledforeground","Specifies the foreground color to use when drawing a disabled component."],
  ["font","Specifies the font to use when drawing text inside the component. For example: Times 10 bold"],
  ["foreground","Specifies the normal foreground color to use when displaying the component."],
  ["height","Specifies the component width in pixels."],
  ["highlightbackground","Specifies the color to display in the traversal highlight region when the component does not have the input focus."],
  ["highlightcolor","Specifies the color to use for the traversal highlight rectangle that is drawn around the component when it has the input focus."],
  ["highlightthickness","Specifies a non-negative value indicating the width of the highlight rectangle to draw around the outside of the component when it has the input focus."],
  ["justify","Specifies the justification of the text in the component. Acceptable values are left, right, or centre"],
  ["relief","Specifies the 3-D effect. Acceptable values are raised, sunken, flat, ridge, solid, and groove."],
  ["width","Specifies the component width in pixels."]
];

## Pack properties
gel_pack_properties = [
  ["side","Specifies which side of the master the slave(s) will be packed against. Must be left, right, top, or bottom. Defaults to top."],
  ["packanchor","Anchor must be a valid anchor position such as n or sw; it specifies where to position each slave in its parcel. Defaults to center."],
  ["expand","Specifies whether the slaves should be expanded to consume extra space in their master. Boolean may have any proper boolean value, such as 1 or no. Defaults to 0."],
  ["fill","If a slave's parcel is larger than its requested dimensions, this option may be used to stretch the slave. Acceptable values are none, x, y, and both. Defaults to none."],
  ["ipadx","Specifies how much horizontal internal padding to leave on each side of the component."],
  ["ipady","Specifies how much vertical internal padding to leave on each side of the component."],
  ["padx","Specifies how much horizontal external padding to leave on each side of the slave."],
  ["pady","specifies how much vertical external padding to leave on each side of the slave."]
];

