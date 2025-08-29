drop procedure if exists log_app_insert;
GO
#DELIMITER $$;
CREATE PROCEDURE log_app_insert( _name varchar(255), out _appId int unsigned )
begin
	insert into log_apps(name) values(_name);
	select LAST_INSERT_ID() into _appId;
end;
#$$
#DELIMITER ;