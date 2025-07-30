create or replace view opc_server_node_ids as
select
	server_id,
	ids.node_id, ids.ns, ids.number, ids.string, ids.guid, ids.bytes, ids.namespace_uri, ids.server_index, ids.is_global
from opc_node_ids ids
	left join opc_server_node_map map using(node_id)
	left join opc_servers servers using(server_id)
where (ids.ns=0 or coalesce(is_global, 0)=1 or ids.node_id=map.node_id)