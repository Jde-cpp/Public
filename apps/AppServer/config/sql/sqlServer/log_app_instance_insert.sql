create or alter proc [dbo].log_app_instance_insert( @app_name varchar(255), @host_name varchar(255), @pid bigint )
as
	set nocount on;
	declare @host_id int;
	declare @app_id int;
	declare @instance_id int;
	select @app_id=app_id from log_apps where name=@app_name;
	if( @app_id is null )
		exec [dbo].log_app_insert @app_name, @app_id out;

	select @host_id=host_id from log_hosts where name=@host_name;
	if @host_id is null
	begin
		insert into log_hosts( name ) values( @host_name );
		set @host_id = @@identity;
	end
	update log_app_instances set end_time=getutcdate() where app_id=@app_id and host_id=@host_id and end_time is null;
	insert into log_app_instances(app_id,end_time,host_id,pid,start_time) values( @app_id,null,@host_id,@pid,getutcdate() );
	set @instance_id = @@IDENTITY;
   select @app_id, @instance_id;
go