drop procedure if exists opc_variable_insert;
go
create procedure opc_variable_insert(
	_ns smallint unsigned, _number int unsigned, _string varchar(256), _guid binary(16), _bytes varbinary(256),
	_parent_node_id bigint unsigned, _ref_type_id bigint unsigned, _type_id bigint unsigned, _browse_id int unsigned,
	_specified int unsigned, _name varchar(256),_description varchar(2048), _write_mask int unsigned,_user_write_mask int unsigned,
	_variant_id int unsigned,_data_type_id bigint(20) unsigned,_value_rank int,_array_dims varchar(256),_access_level tinyint unsigned,_user_access_level tinyint unsigned,_minimum_sampling_interval float,_historizing bit(1),
	out _node_id bigint unsigned )
begin
	declare _is_node bit;
	set _is_node = _number is not null or _string is not null or _guid is not null or _bytes is not null;
	if( _is_node ) then
		call opc_node_id_insert( _ns, _number, _string, _guid, _bytes, null, null, null, _node_id );
	else
		set _node_id = null;
	end if;
	if( _data_type_id<=32750 and (select count(*) from opc_node_ids where node_id=_data_type_id)=0 ) then
		call opc_node_id_insert( 0, _data_type_id, null, null, null, null, null, null, _data_type_id );
	end if;
	insert into opc_variables( node_id, parent_node_id, ref_type_id, type_def_id, browse_id, specified, name,description,write_mask,user_write_mask,
		variant_id,data_type_id,value_rank,array_dims,access_level,user_access_level,minimum_sampling_interval,historizing )
		values( _node_id, _parent_node_id, _ref_type_id, _type_id, _browse_id, _specified, _name, _description, _write_mask, _user_write_mask,
		_variant_id,_data_type_id,_value_rank,_array_dims,_access_level,_user_access_level,_minimum_sampling_interval,_historizing );
end