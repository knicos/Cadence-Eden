/*
doste_update();

func _doste_query {
	para def_ls;
	auto curr_obj, obname;

	curr_obj = (type(def_ls[1])=="string") ? doste_oid(def_ls[1]) : def_ls[1];
	shift def_ls;
	while (def_ls != []) {
		if (type(def_ls[1])!="list")
			obname = def_ls[1];
		else
			obname = _doste_query(def_ls[1]);
		obname = (type(obname)=="string") ? doste_oid(obname) : obname;
		curr_obj = doste_lookup(curr_obj, obname);
		shift def_ls;
	}

	return curr_obj;
}
*/

proc dasmSwitch {};
newNotation("%dasm",&dasmSwitch,&doste_parse);

/*
_doste_tick = 0;
setedenclock(&_doste_tick, 1);
proc updatedoste : _doste_tick {
	doste_update();
};

func doste_root {
	return doste_edenoid(1,0,1,0);
};

func doste_int {
	para i;
	return doste_edenoid(0,1,0,i);
};

func doste_convert {
	para oid;
	auto a;
	auto b;
	a = doste_a(oid);
	b = doste_b(oid);

	if (a == 0 && b == 1) {
		return doste_d(oid);
	}

	return 0;
};

func doste_mklist {
        para listOID;
        auto size, i, result;
        result = [];
        size = doste_convert(doste_lookup(listOID, doste_oid("size")));
        for (i=0; i<size; i++) {
                result = result // [doste_convert(doste_lookup(listOID, doste_int(i)))];
        }
        return result;
};

proc doste_objfromlist {
        para OID, ls;
        auto i;
        doste_set(OID, doste_oid("size"), doste_int(ls#));
	doste_set(OID, doste_oid("type"), doste_oid("list"));
        for (i=1; i<=ls#; i++) {
                doste_set(OID, doste_int(i-1), doste_int(ls[i]));
        }
};

*/

