drop procedure if exists opc_variant_insert;
go
create procedure opc_variant_insert( _data_type_id bigint(20) unsigned,_array_dims varchar(256), out _variant_id int unsigned ) begin
	if( (select count(*) from opc_node_ids where node_id=_data_type_id)=0 && _data_type_id<=32750 ) then
		call opc_node_id_insert( 0, _data_type_id, null, null, null, null, null, null, _data_type_id );
	end if;
	insert into opc_variants( data_type_id,array_dims )
		values( _data_type_id,_array_dims );

	set _variant_id = last_insert_id();
end