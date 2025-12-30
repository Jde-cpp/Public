create or alter proc [dbo].app_instance_insert( @programId int, @name varchar(255), @hostName varchar(255), @instanceId int out ) as
	set nocount on;
	declare @host_id int;
	select @host_id=host_id from [dbo].app_hosts where name=@hostName;
	if( @host_id is null )begin
		insert into [dbo].app_hosts( name ) values( @hostName );
		select @host_id=scope_identity();
	end
	insert into [dbo].app_instances(program_id,name,host_id) values(@programId,@name,@host_id);
	select @instanceId=scope_identity();
go