create or replace view opc_server_browse_names as
  with node_broswses as(
		select node_id, browse_id
		from opc_variables
		union
		select node_id, browse_id
		from opc_objects
		union
		select node_id, browse_id
		from opc_object_types
	)
	select n.server_id, b.browse_id, b.ns, b.name
	from node_broswses nb
		join opc_browse_names b using(browse_id)
		join opc_server_node_ids n using(node_id)