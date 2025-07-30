create or alter view [dbo].opc_server_browse_names as
  with node_browses as(
		select node_id, browse_id
		from [dbo].opc_variables
		union
		select node_id, browse_id
		from [dbo].opc_objects
		union
		select node_id, browse_id
		from [dbo].opc_object_types
	)
	select n.server_id, b.browse_id, b.ns, b.name
	from node_browses nb
		join [dbo].opc_browse_names b on nb.browse_id = b.browse_id
		join [dbo].opc_server_node_ids n on nb.node_id = n.node_id;