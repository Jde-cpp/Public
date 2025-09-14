drop procedure if exists log_app_instance_insert;
GO
#DELIMITER $$;
CREATE PROCEDURE log_app_instance_insert( _app_name varchar(255), _host_name varchar(255), _pid int unsigned )
begin
	declare _host_id int unsigned;
	declare _app_id int unsigned;
	declare _instance_id int unsigned;
	select app_id into _app_id from log_apps where name=_app_name;
	if _app_id is null then
		call log_app_insert( _app_name, _app_id );
	end if;

	select host_id into _host_id from log_hosts where name=_host_name;
	 if( _host_id is null ) then
		insert into log_hosts( name ) values( _host_name );
		select LAST_INSERT_ID() into _host_id;
	 end if;

	update log_app_instances set end_time=NOW() where app_id=_app_id and host_id=_host_id and end_time is null;
	insert into log_app_instances(app_id,end_time,host_id,pid,start_time) values( _app_id,null,_host_id,_pid,NOW() );
	select LAST_INSERT_ID() into _instance_id;
	select _app_id, _instance_id;
end
#$$
#DELIMITER ;