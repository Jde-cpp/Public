# Twin of the trigger half of ../mysql/gateway_server_connection_insert.sql - own file: SyncScripts skips
# files named after registered procs, and sqlite triggers can't assign NEW columns, so this is `after update`
# with a corrective update (recursive_triggers is off by default, so it doesn't re-fire itself).
drop trigger if exists gateway_server_connection_update;
GO
create trigger gateway_server_connection_update after update of is_default on gateway_server_connections for each row
when new.is_default and (select count(*) from gateway_server_connections where server_connection_id!=new.server_connection_id and is_default)>0
begin
	update gateway_server_connections set is_default=0 where server_connection_id=new.server_connection_id;
end;
