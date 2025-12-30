drop procedure if exists app_program_insert;
GO
#DELIMITER $$;
CREATE PROCEDURE app_program_insert( _name varchar(255), out _programId int unsigned )
begin
	insert into app_programs(name) values(_name);
	select LAST_INSERT_ID() into _programId;
end;
#$$
#DELIMITER ;