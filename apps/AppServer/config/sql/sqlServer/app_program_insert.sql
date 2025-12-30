create or alter proc [dbo].app_program_insert( @name varchar(255), @programId int out ) as
	set nocount on;
	insert into [dbo].app_programs(name) values(@name);
	select @programId=scope_identity();
go