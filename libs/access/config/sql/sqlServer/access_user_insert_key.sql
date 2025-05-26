create or alter procedure access_user_insert_key( @modulus varchar(1024), @exponent int, @provider_id int, @name varchar(255), @target varchar(255), @description varchar(2047) ) as
begin
	set nocount on;
	declare @identity_id int;
	exec [access_identity_insert] @name, @provider_id, @target, 0, @description, 0;
	set @identity_id = @@IDENTITY;

	insert into access_users( identity_id, modulus, exponent ) values( @identity_id, @modulus, @exponent );
	select @identity_id;
end