create or alter proc [dbo].access_user_insert( @identity_id int, @login_name varchar(255), @password varbinary(2047), @modulus varchar(1024), @exponent int ) as begin
	set nocount on;
	insert into access_users( identity_id, login_name, password, modulus, exponent ) values( @identity_id, @login_name, @password, @modulus, @exponent  );
	select @identity_id;
end