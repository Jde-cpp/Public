create or replace view access_group_members as
select map.identity_id group_id, parents.target group_target, map.member_id,
	members.name, members.attributes, members.target, members.description, members.created, members.updated, members.deleted, members.is_group
from access_groupings map
	join access_identities parents using(identity_id)
	join access_identities members on map.member_id=members.identity_id