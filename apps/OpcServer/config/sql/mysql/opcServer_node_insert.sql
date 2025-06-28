drop procedure if exists opc_node_insert;
GO
create procedure opc_node_insert(
		_node_id bigint unsigned, _parent_node_id bigint unsigned, _ref_id bigint unsigned, _type_id bigint unsigned,
		_obj_attr_id bigint unsigned, _object_type_attr_id bigint unsigned, _variable_attr_id bigint unsigned,
		_browse_id int unsigned) begin
	insert into opc_nodes(node_id, parent_node_id, reference_type_id, type_definition_id, object_attr_id, object_type_attr_id, variable_attr_id, browse_id)
	values( _node_id, _parent_node_id, _ref_id, _type_id, _obj_attr_id, _object_type_attr_id, _variable_attr_id, _browse_id );
end;