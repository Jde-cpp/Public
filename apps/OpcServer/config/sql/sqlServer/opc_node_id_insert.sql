create or alter proc [dbo].opc_node_id_insert( @ns smallint, @number int, @string varchar(256), @guid binary(16), @bytes varbinary(256), @namespace_uri varchar(256), @server_index int, @is_global bit, @node_id bigint output ) as begin
	set nocount on;
	set @node_id = ( select cast(@ns as bigint) * power(cast(2 as bigint), cast(32 as bigint)) );
	if( @number is not null )
		set @node_id = @node_id + @number;
	else if( @string is not null )
		set @node_id = @node_id + checksum(@string);
	else if( @guid is not null )
		set @node_id = @node_id + checksum(@guid);
	else if( @bytes is not null )
		set @node_id = @node_id + checksum(@bytes);
	insert into [dbo].opc_node_ids( node_id, ns, number, string, guid, bytes, namespace_uri, server_index, is_global )
	values( @node_id, @ns, @number, @string, @guid, @bytes, @namespace_uri, @server_index, @is_global );
end;
go
declare @node_id bigint;
exec opc.node_id_insert 0,78,null,null,Null,null,0,null,@node_id;
select @node_id as node_id;
delete from opc.node_ids