drop procedure if exists opc_object_type_insert;
GO
create procedure opc_object_type_insert(
		_ns smallint unsigned, _number int unsigned, _string varchar(256), _guid binary(16), _bytes varbinary(256),
		_parent_node_id bigint unsigned, _ref_type_id bigint unsigned, _browse_id int unsigned, _specified int unsigned, _name varchar(256), _description varchar(2048),
		_write_mask int unsigned, _user_write_mask int unsigned, _is_abstract bit,
		out _node_id bigint unsigned) begin
	call opc_node_id_insert( _ns, _number, _string, _guid, _bytes, null, null, true, _node_id );

	insert into opc_object_types(node_id, parent_node_id, ref_type_id, browse_id, name, description, write_mask, user_write_mask, is_abstract)
	values( _node_id, _parent_node_id, _ref_type_id, _browse_id, _name, _description, _write_mask, _user_write_mask, _is_abstract );
end;