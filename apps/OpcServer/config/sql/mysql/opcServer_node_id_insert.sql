drop procedure if exists opc_node_id_insert;
GO

create procedure opc_node_id_insert( _ns smallint unsigned, _number int unsigned, _string varchar(256), _guid binary(16), _bytes varbinary(256), _namespace_uri varchar(256), _server_index int unsigned, _is_global bit, out _node_id bigint unsigned ) begin
	set _node_id = (select _ns << 32);
	if( _number is not null ) then
		set _node_id = _node_id | _number;
	elseif( _string is not null ) then
		set _node_id = _node_id | CRC32(_string);
	elseif( _guid is not null ) then
		select _node_id | CRC32(BIN_TO_UUID(_guid)) into _node_id;
	elseif( _bytes is not null ) then
		select _node_id | CRC32(cast(_bytes as char)) into _node_id;
	end if;
	insert into opc_node_ids( node_id, ns, number, string, guid, bytes, namespace_uri, server_index, is_global )
	values( _node_id, _ns, _number, _string, _guid, _bytes, _namespace_uri, _server_index, _is_global );
end;