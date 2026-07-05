create or replace view access_users_ql as
select base.identity_id, base.name, base.provider_id, base.target, base.attributes, base.created, base.updated, base.deleted, base.description, base.is_group, 
       users.login_name, users.password, users.modulus, users.exponent 
from access_identities base
	left join access_users users using(identity_id);