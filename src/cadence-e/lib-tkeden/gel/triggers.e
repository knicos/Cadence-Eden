%eden

##
## Procedures to create triggers on all the potential observables
## belonging to a component
##

proc gel_create {
   para name, geltype;
   
   auto i;
   
   `name//"_type"` = geltype;
   
   ## Standard property triggers for all components   
   for (i=1; i<=gel_standard_properties#; i++) {
      gel_create_property_trigger(name,gel_standard_properties[i][1]);
   }

   ## Bind triggers for all components
   gel_create_custom_trigger(name,"bindings","gel_update_bind");
      
   ## Special container triggers
   if (geltype == 1 || geltype == 2) {
      gel_create_content_packlist_trigger(name);
   }
   
   ## Specific component triggers
   for (i=1; i<=gel_components[geltype][2]#; i++) {
      if (type(gel_components[geltype][2][i][3]) == "string") {
         gel_create_custom_trigger(name,gel_components[geltype][2][i][1],gel_components[geltype][2][i][3]);
      }
      else {
         gel_create_property_trigger(name,gel_components[geltype][2][i][1]);
      }
   }

   ## Create/delete triggers by component
   if (geltype == 1)
      gel_create_window(name);
   else
      gel_create_parent_trigger(name, gel_type_name(geltype));

}

proc gel_create_property_trigger {
   para name, property;
   execute("proc gel_trigger_"//name//"_"//property//" : "//name//"_"//property//", "//name//"_tk { gel_update_property(\""//name//"\",\""//property//"\"); }");
}

proc gel_create_custom_trigger {
   para name, property, procname;
   execute("proc gel_trigger_"//name//"_"//property//" : "//name//"_"//property//", "//name//"_tk { "//procname//"(\""//name//"\"); }");
}

proc gel_create_parent_trigger {
   para name, geltype;
   execute("
      proc gel_trigger_"//name//"_parent : "//name//"_parent {
         if ("//name//"_tk != `"//name//"_parent//\"_tk\"`//\"."//name//"\") {
            gel_remove_component(\""//name//"\");
            gel_create_"//geltype//"(\""//name//"\","//name//"_parent);
            gel_update_pack("//name//"_parent);
	 }
      }");          
}

proc gel_create_content_packlist_trigger {
   para name;
   execute("
      proc gel_trigger_"//name//"_content_packlist : "//name//"_content {
         auto i, mystr;
	 if (gel_debug)
            writeln(\"trigger_"//name//"_content_packlist\");
         for (i=1; i<="//name//"_content#; i++) {
            execute(\"proc gel_trigger_\"//"//name//"_content[i]//\"_pack_side : \"//"//name//"_content[i]//\"_side { if (gel_debug) writeln(\\\"trigger_"//name//"_pack_side\\\"); gel_update_pack(\\\""//name//"\\\"); }\");
            execute(\"proc gel_trigger_\"//"//name//"_content[i]//\"_pack_packanchor : \"//"//name//"_content[i]//\"_packanchor { if (gel_debug) writeln(\\\"trigger_"//name//"_pack_packanchor\\\"); gel_update_pack(\\\""//name//"\\\"); }\");
            execute(\"proc gel_trigger_\"//"//name//"_content[i]//\"_pack_expand : \"//"//name//"_content[i]//\"_expand { if (gel_debug) writeln(\\\"trigger_"//name//"_pack_expand\\\"); gel_update_pack(\\\""//name//"\\\"); }\");
            execute(\"proc gel_trigger_\"//"//name//"_content[i]//\"_pack_fill : \"//"//name//"_content[i]//\"_fill { if (gel_debug) writeln(\\\"trigger_"//name//"_pack_fill\\\"); gel_update_pack(\\\""//name//"\\\"); }\");
            execute(\"proc gel_trigger_\"//"//name//"_content[i]//\"_pack_ipadx : \"//"//name//"_content[i]//\"_ipadx { if (gel_debug) writeln(\\\"trigger_"//name//"_pack_ipadx\\\"); gel_update_pack(\\\""//name//"\\\"); }\");
            execute(\"proc gel_trigger_\"//"//name//"_content[i]//\"_pack_ipady : \"//"//name//"_content[i]//\"_ipady { if (gel_debug) writeln(\\\"trigger_"//name//"_pack_ipady\\\"); gel_update_pack(\\\""//name//"\\\"); }\");
            execute(\"proc gel_trigger_\"//"//name//"_content[i]//\"_pack_padx : \"//"//name//"_content[i]//\"_padx { if (gel_debug) writeln(\\\"trigger_"//name//"_pack_padx\\\"); gel_update_pack(\\\""//name//"\\\"); }\");
            execute(\"proc gel_trigger_\"//"//name//"_content[i]//\"_pack_pady : \"//"//name//"_content[i]//\"_pady { if (gel_debug) writeln(\\\"trigger_"//name//"_pack_pady\\\"); gel_update_pack(\\\""//name//"\\\"); }\");
         }
      }");
}

proc gel_create_content_packlist_trigger_old {
   para name;
   execute("
      proc gel_trigger_"//name//"_content_packlist : "//name//"_content {
         auto i, mystr;
	 if (gel_debug)
            writeln(\"trigger_"//name//"_content_packlist\");
	 mystr=\"\";
         for (i=1; i<="//name//"_content#; i++) {
            if (i==1)
               mystr = mystr // \": \";
	    else
               mystr = mystr // \", \";
            mystr = mystr // macro(\"?1_side, ?1_packanchor, ?1_expand, ?1_fill, ?1_ipadx, ?1_ipady, ?1_padx, ?1_pady\", "//name//"_content[i]);
         }
         execute(\"proc gel_trigger_"//name//"_pack \"//mystr//\" { 	
	 if (gel_debug)
            writeln(\\\"trigger_"//name//"_pack\\\");
         gel_update_pack(\\\""//name//"\\\"); }\");
      }");
}
