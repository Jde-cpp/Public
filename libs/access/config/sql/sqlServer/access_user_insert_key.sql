create or alter procedure [dbo].access_user_insert_key( @modulus varchar(1024), @exponent int, @provider_id int, @name varchar(255), @target varchar(255), @description varchar(2047), @identity_id int output ) as
begin
	set nocount on;
	exec [dbo].[access_identity_insert] @name, @provider_id, @target, 0, @description, 0, @identity_id output;

	insert into access_users( identity_id, modulus, exponent ) values( @identity_id, @modulus, @exponent );
end