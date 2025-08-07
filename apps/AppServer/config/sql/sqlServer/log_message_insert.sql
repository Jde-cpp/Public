create or alter proc [dbo].log_message_insert_out( @app_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @tags bigint, @time datetime2,  @userId int, @id bigint out )
as
	INSERT INTO log_entries(app_instance_id,message_id,line_number,severity,source_file_id,source_function_id, tags, time,thread_id,user_id)
	VALUES( @app_instance_id, @messageId, @lineNumber, @level, @fileId, @functionId, @tags, @time, @threadId, @userId );

	set @id=@@IDENTITY;
	if @id%10000=0 begin
		if @id>1000000  begin
			delete from log_entries where entry_id<@id-1000000;
			delete from log_entry_map where entry_id<@id-1000000;
			delete from log_args where arg_id not in (select arg_id from log_entry_map);
			delete from log_source_files where source_file_id not in (select source_file_id from log_entries);
			delete from log_source_functions where source_function_id not in (select source_function_id from log_entries);
			delete from log_messages where message_id not in (select message_id from log_entries);
		end
	end
	select @id;
go

create or alter proc [dbo].log_message_insert( @app_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @tags bigint, @time datetime2,  @userId int ) as
	set nocount on;
	declare @id int;
	exec log_message_insert_out @app_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @time, @userId, @id;
go

create or alter proc [dbo].log_message_insert1( @app_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @tags bigint, @time datetime2,  @userId int, @variable0 varchar(4095) )
as
	set nocount on;
	declare @id int;
	exec log_message_insert_out @app_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @tags, @time, @userId, @id;
	insert into log_variables values(@id,0,@variable0);
go

create or alter proc [dbo].log_message_insert2( @app_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @tags bigint, @time datetime2,  @userId int, @variable0 varchar(4095), @variable1 varchar(4095) )
as
	set nocount on;
	declare @id int;
	exec log_message_insert_out @app_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @tags, @time, @userId, @id;
	insert into log_variables values(@id,0,@variable0);
	insert into log_variables values(@id,1,@variable1);
go


create or alter proc [dbo].log_message_insert3( @app_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @tags bigint, @time datetime2,  @userId int, @variable0 varchar(4095), @variable1 varchar(4095), @variable2 varchar(4095) )
as
	set nocount on;
	declare @id int;
	exec log_message_insert_out @app_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @tags, @time, @userId, @id;
	insert into log_variables values(@id,0,@variable0);
	insert into log_variables values(@id,1,@variable1);
	insert into log_variables values(@id,2,@variable2);
go

create or alter proc [dbo].log_message_insert4( @app_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @tags bigint, @time datetime2,  @userId int, @variable0 varchar(4095), @variable1 varchar(4095), @variable2 varchar(4095), @variable3 varchar(4095) )
as
	set nocount on;
	declare @id int;
	exec log_message_insert_out @app_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @tags, @time, @userId, @id;
	insert into log_variables values(@id,0,@variable0);
	insert into log_variables values(@id,1,@variable1);
	insert into log_variables values(@id,2,@variable2);
	insert into log_variables values(@id,3,@variable3);
go

create or alter proc [dbo].log_message_insert5( @app_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @tags bigint, @time datetime2,  @userId int, @variable0 varchar(4095), @variable1 varchar(4095), @variable2 varchar(4095), @variable3 varchar(4095), @variable4 varchar(4095) )
as
	set nocount on;
	declare @id int;
	exec log_message_insert_out @app_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @tags, @time, @userId, @id;
	insert into log_variables values(@id,0,@variable0);
	insert into log_variables values(@id,1,@variable1);
	insert into log_variables values(@id,2,@variable2);
	insert into log_variables values(@id,3,@variable3);
	insert into log_variables values(@id,4,@variable4);
go