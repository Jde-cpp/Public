drop procedure if exists app_instance_insert;
GO
#DELIMITER $$;
CREATE PROCEDURE app_instance_insert( _programId int unsigned, _name varchar(255), _hostName varchar(255), out _instanceId int unsigned )
begin
	declare _host_id int unsigned;
	 select host_id into _host_id from app_hosts where name=_hostName;
	 if( _host_id is null ) then
		insert into app_hosts( name ) values( _hostName );
		select LAST_INSERT_ID() into _host_id;
	 end if;
	insert into app_instances(program_id,name,host_id) values(_programId,_name,_host_id);
	select LAST_INSERT_ID() into _instanceId;
end;
#$$
#DELIMITER ;