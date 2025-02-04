-- Active: 1734133989127@@127.0.0.1@3306@test_access

select CONV('ff', 16, 10 )
select MD5('testing') --ae2b1fca515949e5 d54fb22b8ed95575
select substring( MD5('testing'), 17, 16 ) --d54fb22b8ed95575
select CONV( substring( MD5('testing'), 17, 16), 16, 10 ) --15370699953388737909
select cast( CONV( substring( MD5('testing'), 17, 16), 16, 10 ) as UNSIGNED ) --15370699953388737909
select  into _md5;
drop procedure if exists log_app_instance_insert;
drop procedure if exists log_app_insert;

call log_app_instance_insert('Main','PL2USWAA0030WS',79750)
select * from roles --5-8
select * from role_members where role_id=8

delete from role_members;
delete from roles;
select * from access_resources
select * from resources



select count(*)
from access_identities
where identities.identity_id=1
drop table log_severities

use test_access;
select * from identities
select * from acl where identity_id=4
select * from permissions where permission_id=10

select resources.name, resources.deleted, permission_rights.permission_id, permission_rights.allowed, permission_rights.denied
from acl
	join permission_rights using(permission_id)
	join resources using(resource_id)
where identity_id=3

select resources.resource_id, resources.schema_name, resources.target, resources.criteria, UNIX_TIMESTAMP(resources.deleted) deleted
from resources
where resources.schema_name in('access')

select access_providers_ql.provider_id, access_providers_ql.name from providers_ql order by provider_id

 SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'Custom error2';
select * from access_identities
delete from access_identities where identity_id=3
delete from access_users where identity_id=3
select version()
delete from resources

select * from rights
select * from resources where resource_id=2
update resources set deleted=now() where resource_id=1

select resources.resource_id, resources.schema_name, resources.allowed, resources.denied, resources.name, resources.attributes, UNIX_TIMESTAMP(resources.created) created, UNIX_TIMESTAMP(resources.deleted) deleted, UNIX_TIMESTAMP(resources.updated) updated, resources.target, resources.description
from resources
where resources.schema_name='access' and resources.target='identityGroups' and resources.criteria is null

select permission_id
into _permission_id
from role_members
	join permission_rights on role_members.member_id=permission_rights.permission_id
	join resources using(resource_id)
where role_members.role_id=_role_id
	and resources.target=_resourceTarget;


select * from roles;
select * from role_members where role_id=28
delete from role_members where role_id=28
select roles.role_id, roles.target, UNIX_TIMESTAMP(roles.deleted) deleted, acl.identity_id
from roles
        join acl on roles.role_id=acl.permission_id
where roles.role_id=11 and acl.identity_id=14

select * from providers

call user_insert_login('Login_New',1,null);
select name from access_provider_types where provider_id = 1;


select identities.*
from identities
        join users using(identity_id)
        join providers using(provider_id)
where users.login_name='Login_Existing_Opc' and identities.provider_id=7 and providers.target='AuthTests::OpcServer1'


select * from identities where identity_id=13; target='hierarchyUser'--15
select * from group_members where target='hierarchyAdmin'
select @id =29;
delete from identity_groups where identity_id=13;
delete from identity_groups where member_id=13;
delete from acl where identity_id=13;
delete from identities where identity_id=13;

select * from roles
delete from role_members;
delete from roles;
delete from acl where permission_id not in (select permission_id from permission_rights);

select identities.identity_id, identities.name, identities.attributes, UNIX_TIMESTAMP(identities.created) created, UNIX_TIMESTAMP(identities.updated) updated, identities.target, identities.description, UNIX_TIMESTAMP(identities.deleted) deleted
from identities
        left join identity_groups using(identity_id)
where identities.target='EnabledPermissions-Group3' and is_group


select r.target, p.allowed, p.denied
from acl a
	join permission_rights p using(permission_id)
	join resources r using(resource_id)
where identity_id=3
order by identity_id

select * from identities where target='HierarchyGroupAdmin' --31
select * from group_members where group_id=30
select * from acl where identity_id=30
select * from identity_groups where identity_id=31 --32
delete from acl where identity_id=30


select * from permission_rights where permission_id=14
select * from role_members where member_id=14
select * from roles where role_id=11
select * from role_members where role_id=11

select resources.name, rights.allowed, rights.denied from permission_rights rights
	join resources using(resource_id)
	join role_members on permission_id=member_id where role_id=11


select map.identity_id group_id, identityGroups.target group_target, map.member_id,
	members.name, members.attributes, members.target, members.description, members.created, members.updated, members.deleted, members.is_group
from identity_groups map
	join identities identityGroups using(identity_id)
	join identities members on map.member_id=members.identity_id


select identities.identity_id, identities.name, identities.attributes, UNIX_TIMESTAMP(identities.created) created, UNIX_TIMESTAMP(identities.updated) updated, identities.target, identities.description, UNIX_TIMESTAMP(identities.deleted) deleted
from identities
	join identity_groups using(identity_id)
where identities.target='HierarchyGroupUsers' and is_group

select * from roles

select *
from permission_rights
        join resources using(resource_id)
where resources.target='users' and resources.criteria is null

delete from users where identity_id > 0;
delete from identities where identity_id =14

select * from providers
select * from provider_types
select * from users
delete from users where identity_id>5

select identities.identity_id
from identities
where identities.is_group=false

select resources.resource_id,resources.schema_name,resources.target,resources.criteria,UNIX_TIMESTAMP(resources.deleted) delete
from resources
where resources.schema_name='access'


select * from acl
select * from role_members
select * from resources;

delete from acl;
delete from permissions;
delete from resources;

select permission_id
from role_members
	join permission_rights on role_members.member_id=permission_rights.permission_id
	join resources using(resource_id)
where role_members.role_id=7
	and resources.target='identityGroups';

insert into permissions( is_role ) values( false );
select LAST_INSERT_ID();
select resource_id from resources where target = 'identityGroups'

delete from users;
delete from identities;


select identities.identity_id, identities.name, identities.attributes, UNIX_TIMESTAMP(identities.created) created, UNIX_TIMESTAMP(identities.updated) updated, identities.target, identities.description, UNIX_TIMESTAMP(identities.deleted) deleted
from identities
        left join identity_groups using(identity_id)
where is_group


select * from rights
select * from resources

select resources.resource_id, criteria, resources.schema_name, resources.rights, resources.name, resources.attributes, UNIX_TIMESTAMP(resources.created) created, UNIX_TIMESTAMP(resources.deleted) deleted, UNIX_TIMESTAMP(resources.updated) updated, resources.target, resources.description
from resources
where resources.schema_name='access' and resources.target='identityGroups' and resources.criteria is null

delete from identities where identity_id=26
select * from identity_groups where identity_id=24

select * from identities

select identities.identity_id, identities.name, identities.attributes, UNIX_TIMESTAMP(identities.created) created, UNIX_TIMESTAMP(identities.updated) updated, identities.target, identities.description
from identities
        left join identity_groups using(identity_id)
where identities.target='groupTest' and identities.deleted is null and is_group

delete from identities where identity_id=8
delete from identity_groups where identity_id=13

call resource_insert('access','identity_groups','identityGroups',null,'From installation',null,63,null)
select * from rights

call identity_insert('root - name',1,'root',null,'root - description',false)
select * from providers
select * from provider_types

select access_resources.resource_id, access_resources.schema_name, access_resources.name, access_resources.target, access_resources.attributes, UNIX_TIMESTAMP(access_resources.created) created, UNIX_TIMESTAMP(access_resources.updated) updated, UNIX_TIMESTAMP(access_resources.deleted) deleted, access_resources.description
from access_resources