create or alter view [dbo].access_users_ql as
select base.identity_id, base.name, base.provider_id, base.target, base.attributes, base.created, base.updated, base.deleted, base.description, base.is_group, 
       users.login_name, users.password, users.modulus, users.exponent 
from [dbo].access_identities base
	left join [dbo].access_users users on base.identity_id = users.identity_id;