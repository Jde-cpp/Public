create or alter proc [dbo].log_app_insert( @name varchar(255), @id int out ) as
	set nocount on;
	insert into log_apps( name, attributes )
		values( @name, 0 );
	set @id=@@identity;