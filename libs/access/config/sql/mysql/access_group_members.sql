create or replace view access_group_members as
select map.identity_id group_id, map.member_id,
	members.name, members.attributes, members.target, members.description, members.created, members.updated, members.deleted, members.is_group
from identity_groups map
	join identities identityGroups using(identity_id)
	join identities members on map.member_id=members.identity_id