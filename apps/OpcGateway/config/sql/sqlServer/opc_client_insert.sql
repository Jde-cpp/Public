create or alter proc [dbo].opc_client_insert @name varchar(255), @target varchar(255), @attributes smallint, @description varchar(2047), @is_default bit, @certificate_uri varchar(2047), @url varchar(2047) as begin
	set nocount on;
  if @is_default=1
    update [dbo].opc_clients set is_default=0;
  insert into [dbo].opc_clients( name, attributes, created, target, description, certificate_uri, is_default, url )
    values( @name, @attributes, getutcdate(), @target, @description, @certificate_uri, @is_default, @url );
  select SCOPE_IDENTITY() server_id;
end
go
create or alter trigger [dbo].opc_client_update on [dbo].opc_clients for update as begin
    declare @is_default bit, @id int;
    select @is_default=is_default, @id=server_id from inserted;
    if @is_default=1
        update [dbo].opc_clients set is_default=0 where server_id!=@id;
end
go