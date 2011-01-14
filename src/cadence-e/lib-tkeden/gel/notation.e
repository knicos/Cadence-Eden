%eden

##
## %angel notation
##


## Initialisation (split on semicolon, ignore within braces)

angel_init = [ ";", "angel_start", [ "bras", "curly_bras", "quotes", "comment" ] ];


## Blocks that need to be ignored

curly_bras = [["{", "}"], ["curly_bras","quotes"]];
quotes = [["\"", "\""], []]; 
addblocks ("curly_bras","quotes");

%eden

## General regular expressions

angel_re_name = "[a-zA-Z][a-zA-Z0-9_]*";


## Start of parser rules

angel_start =
  [ "suffix", ";", [ "angel_comment" ],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "autocalc=0;" ],
      [ "later", "autocalc=1;" ] ] ];


## Skip comments

angel_comment =
  [ "prefix", "/*", [ "angel_comment2" ],
    [ "fail", "angel_statement" ] ];

angel_comment2 =
  [ "pivot", "*/", [ "angel_nothing", "angel_comment" ],
    [ "ignore", [ "comment", "quotes" ] ],
    [ "fail", "angel_error" ] ];

angel_nothing =
  [ "read_all", [] ];

## Query statement

angel_statement =
  [ "prefix", "?", [ "angel_name" ],
    [ "fail", "angel_statement2" ],
    [ "action",
      [ "later", "gel_query($p1);" ] ] ];
      
angel_name =
  [ "literal_re", angel_re_name,
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v = $j;" ] ] ];


## Definition statement    

angel_statement2 =
  [ "pivot", "=", [ "angel_observable_name", "angel_definition" ],
    [ "fail", "angel_error" ],
    [ "action",
      [ "later", "gel_parse_definition($p1,$p2[1],$p2[2],$p2[3]);" ] ] ];

angel_observable_name =
  [ "pivot", ".", [ "angel_name", "angel_name" ],
    [ "fail", "angel_name" ],
    [ "action",
      [ "now", "$v is [$p1,$p2];" ] ] ];
      
angel_definition =
  [ "prefix", "window", [ "angel_definition_bras" ],
    [ "fail", "angel_definition2" ],
    [ "action",
      [ "now", "$v is [GEL_WINDOW] // $p1;" ] ] ];

angel_definition2 =
  [ "prefix", "frame", [ "angel_definition_bras" ],
    [ "fail", "angel_definition3" ],
    [ "action",
      [ "now", "$v is [GEL_FRAME] // $p1;" ] ] ];

angel_definition3 =
  [ "prefix", "button", [ "angel_definition_bras" ],
    [ "fail", "angel_definition4" ],
    [ "action",
      [ "now", "$v is [GEL_BUTTON] // $p1;" ] ] ];

angel_definition4 =
  [ "prefix", "textbox", [ "angel_definition_bras" ],
    [ "fail", "angel_definition5" ],
    [ "action",
      [ "now", "$v is [GEL_TEXTBOX] // $p1;" ] ] ];

angel_definition5 =
  [ "prefix", "label", [ "angel_definition_bras" ],
    [ "fail", "angel_definition6" ],
    [ "action",
      [ "now", "$v is [GEL_LABEL] // $p1;" ] ] ];

angel_definition6 =
  [ "prefix", "checkbutton", [ "angel_definition_bras" ],
    [ "fail", "angel_definition7" ],
    [ "action",
      [ "now", "$v is [GEL_CHECKBUTTON] // $p1;" ] ] ];

angel_definition7 =
  [ "prefix", "radiobutton", [ "angel_definition_bras" ],
    [ "fail", "angel_definition8" ],
    [ "action",
      [ "now", "$v is [GEL_RADIOBUTTON] // $p1;" ] ] ];

angel_definition8 =
  [ "prefix", "scrollbar", [ "angel_definition_bras" ],
    [ "fail", "angel_definition9" ],
    [ "action",
      [ "now", "$v is [GEL_SCROLLBAR] // $p1;" ] ] ];

angel_definition9 =
  [ "prefix", "scale", [ "angel_definition_bras" ],
    [ "fail", "angel_definition10" ],
    [ "action",
      [ "now", "$v is [GEL_SCALE] // $p1;" ] ] ];

angel_definition10 =
  [ "prefix", "scout", [ "angel_definition_bras" ],
    [ "fail", "angel_definition11" ],
    [ "action",
      [ "now", "$v is [GEL_SCOUT] // $p1;" ] ] ];

angel_definition11 =
  [ "prefix", "html", [ "angel_definition_bras" ],
    [ "fail", "angel_definition12" ],
    [ "action",
      [ "now", "$v is [GEL_HTML] // $p1;" ] ] ];

angel_definition12 =
  [ "prefix", "image", [ "angel_definition_bras" ],
    [ "fail", "angel_definition13" ],
    [ "action",
      [ "now", "$v is [GEL_IMAGE] // $p1;" ] ] ];

angel_definition13 =
  [ "prefix", "listbox", [ "angel_definition_bras" ],
    [ "fail", "angel_definition_expr" ],
    [ "action",
      [ "now", "$v is [GEL_LISTBOX] // $p1;" ] ] ];

angel_definition_expr =
  [ "prefix_re", "(\\[)\\s*[a-zA-Z][a-zA-Z0-9_]*\\s*,", [ "angel_content_property4" ],
    [ "fail", "angel_definition_expr2" ],
    [ "action",
      [ "now", "$v is [GEL_LIST, 0, $p1];" ] ] ];

angel_definition_expr2 =
  [ "prefix", "[", [ "angel_bindings_property4" ],
    [ "fail", "angel_definition_expr3" ],
    [ "action",
      [ "now", "$v = [GEL_OTHER, 0, $j];" ] ] ];

angel_definition_expr3 =
  [ "read_all", [],
    [ "action",
      [ "now", "$v = [GEL_OTHER, 0, $j];" ] ] ];

angel_definition_bras =
  [ "prefix", "(", [ "angel_definition_bras2" ],
    [ "fail", "angel_definition_curlybras" ],
    [ "action",
      [ "now", "$v is [1,$p1];" ] ] ];

angel_definition_bras2 =
  [ "suffix", ")", [ "angel_definition_arguments" ],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is $p1;" ] ] ];

angel_definition_curlybras =
  [ "prefix", "{", [ "angel_definition_curlybras2" ],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is [2,$p1];" ] ] ];

angel_definition_curlybras2 =
  [ "suffix", "}", [ "angel_definition_properties" ],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is $p1;" ] ] ];
      
angel_definition_arguments =
  [ "pivot", ",", [ "angel_argument", "angel_definition_arguments" ],
    [ "ignore", [ "bras", "curly_bras", "quotes" ] ],
    [ "fail", "angel_argument" ],
    [ "action",
      [ "now", "$v is $p1 // $p2;" ] ] ];
      
angel_argument =
  [ "read_all", [],
    [ "action",
      [ "now", "$v = [$j];" ] ] ];

angel_definition_properties =
  [ "pivot", ";", [ "angel_content_property", "angel_definition_properties" ],
    [ "ignore", [ "bras", "curly_bras", "quotes" ] ],
    [ "fail", "angel_content_property" ],
    [ "action",
      [ "now", "$v is $p1 // $p2;" ] ] ];

angel_content_property =
  [ "prefix", "content", ["angel_content_property2"],
    [ "fail", "angel_bindings_property" ],
    [ "action",
      [ "now", "$v is [[\"content\",$p1]];" ] ] ];

angel_content_property2 =
  [ "prefix", "=", ["angel_content_property3"],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is $p1;" ] ] ];

angel_content_property3 =
  [ "prefix", "[", ["angel_content_property4"],
    [ "fail", "angel_property_definition" ],
    [ "action",
      [ "now", "$v is $p1;" ] ] ];

angel_content_property4 =
  [ "suffix", "]", ["angel_content_property5"],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is $p1;" ] ] ];

angel_content_property5 =
  [ "pivot", ",", ["angel_content_property6","angel_content_property5"],
    [ "fail", "angel_content_property6" ],
    [ "action",
      [ "now", "$v is $p1 // $p2;"] ] ];

angel_content_property6 =
  [ "literal_re", angel_re_name,
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v = [$j];" ] ] ];

angel_bindings_property =
  [ "prefix", "bindings", ["angel_bindings_property2"],
    [ "fail", "angel_property" ],
    [ "action",
      [ "now", "$v is [[\"bindings\",$p1]];" ] ] ];

angel_bindings_property2 =
  [ "prefix", "=", ["angel_bindings_property3"],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is $p1;" ] ] ];

angel_bindings_property3 =
  [ "prefix", "[", ["angel_bindings_property4"],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is $p1;" ] ] ];

angel_bindings_property4 =
  [ "suffix", "]", ["angel_bindings_property5"],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is $p1;" ] ] ];

angel_bindings_property5 =
  [ "pivot", ",", ["angel_bindings_property6","angel_bindings_property5" ],
    [ "ignore", [ "curly_bras" ] ],
    [ "fail", "angel_bindings_property6" ],
    [ "action",
      [ "now", "$v is $p1 // $p2;" ] ] ];

angel_bindings_property6 =
  [ "pivot", "{", ["angel_bindings_property8","angel_bindings_property7"],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is [[$p1,$p2]];" ] ] ];

angel_bindings_property7 =
  [ "suffix", "}", ["angel_bindings_property8"],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is $p1;" ] ] ];

angel_bindings_property8 =
  [ "read_all", [],
    [ "action",
      [ "now", "$v = escapequotes($j);" ] ] ];

angel_property =
  [ "pivot", "=", [ "angel_name", "angel_property_definition" ],
    [ "fail", "angel_error" ],
    [ "action",
      [ "now", "$v is [[$p1,$p2]];" ] ] ];
      
angel_property_definition =
  [ "read_all", [],
    [ "action",
      [ "now", "$v = $j;" ] ] ];


## Error message

angel_error =
  [ "read_all", [],
    [ "action",
      [ "now", "writeln(\"angel: parse error\");" ] ] ];


## Add notation to environment

installAOP("%angel", "angel_init");
