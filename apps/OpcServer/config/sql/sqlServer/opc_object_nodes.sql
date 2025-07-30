create or alter view [dbo].opc_object_nodes as
select ids.server_id, ids.node_id, ids.ns, ids.number, ids.string, ids.guid, ids.bytes, ids.namespace_uri, ids.server_index, ids.is_global,
	objs.parent_node_id, objs.ref_type_id, objs.browse_id, objs.type_def_id, objs.specified, objs.name, objs.description, objs.write_mask, objs.user_write_mask, objs.created, objs.deleted, objs.updated,
	objs.event_notifier
from [dbo].opc_server_node_ids ids
	join [dbo].opc_objects objs on ids.node_id = objs.node_id