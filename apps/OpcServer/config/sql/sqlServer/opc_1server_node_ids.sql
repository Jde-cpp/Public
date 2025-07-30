create or alter view [dbo].opc_server_node_ids as
select
	map.server_id,
	ids.node_id, ids.ns, ids.number, ids.string, ids.guid, ids.bytes, ids.namespace_uri, ids.server_index, ids.is_global
from [dbo].opc_node_ids ids
	left join [dbo].opc_server_node_map map on ids.node_id = map.node_id
	left join [dbo].opc_servers servers on map.server_id = servers.server_id
where (ids.ns=0 or coalesce(ids.is_global, 0)=1 or ids.node_id=map.node_id)