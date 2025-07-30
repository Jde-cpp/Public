drop procedure if exists opc_object_insert;
GO
create procedure opc_object_insert(
		_ns smallint unsigned, _number int unsigned, _string varchar(256), _guid binary(16), _bytes varbinary(256),
		_parent_node_id bigint unsigned, _ref_type_id bigint unsigned, _type_def_id bigint unsigned, _browse_id int unsigned,
		_specified int unsigned, _name varchar(256), _description varchar(2048), _write_mask int unsigned, _user_write_mask int unsigned,
		_event_notifier tinyint unsigned,
		out _node_id bigint unsigned) begin
	call opc_node_id_insert( _ns, _number, _string, _guid, _bytes, null, null, true, _node_id );

	insert into opc_objects( node_id, parent_node_id, ref_type_id, type_def_id, browse_id, specified, name, description, write_mask, user_write_mask, event_notifier )
	values( _node_id, _parent_node_id, _ref_type_id, _type_def_id, _browse_id, _specified, _name, _description, _write_mask, _user_write_mask, _event_notifier );
end;