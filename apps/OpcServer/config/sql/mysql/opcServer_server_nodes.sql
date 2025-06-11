create or replace view opc_server_nodes as
select
	server_id, servers.name server_name, servers.target server_target, servers.created server_created, servers.updated server_updated, servers.deleted server_deleted, servers.description server_description,
	ids.node_id, ids.ns, ids.number, ids.string, ids.guid, ids.bytes, ids.namespace_uri, ids.server_index, ids.is_global,
	nodes.parent_node_id, nodes.reference_type_id, nodes.type_definition_id, nodes.object_attr_id, nodes.type_attr_id, nodes.variable_attr_id, nodes.name, nodes.created, nodes.deleted, nodes.updated
from opc_node_ids ids
  left join opc_nodes nodes using(node_id)
	left join opc_server_node_map map using(node_id)
	left join opc_servers servers using(server_id)
where (ids.ns=0 or coalesce(is_global, 0)=1 or ids.node_id=map.node_id)