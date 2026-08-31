//The OpcServer's address space is defined by the NodeSet2 xml files at /opcServer/configFiles, not by the database.
//The tables that used to persist it (node_ids, browse_names, objects, object_types, variables, variants, refs,
//data_types, servers, …) and the ServerConfigAwait chain that read them are gone;  git history has them if the
//DB-backed address space is ever revived.  The schema itself stays, for the one thing that outlived the tables:
//`nodeIds` is the access resource every node-level acl hangs off (OpcAuthorize::AssignRights matches
//resource.Target=="nodeIds"; node-access.ts writes to it).  It used to exist only because the node_ids *table* had no
//`ops` key and so took DefaultOps, which is what made ResourceSyncAwait create the row - declared outright here now.
{
	tables:{},
	resources:{
		nodeIds:{ ops:["Create","Read","Update","Delete","Purge","Administer"] } //DB::DefaultOps, the rights the table carried.
	}
}
