drop procedure if exists opc_server_insert;
GO

create procedure opc_client_insert( _name varchar(255),_target varchar(255),_attributes smallint,_description varchar(2047), _is_default bit, _certificate_uri varchar(2047),_url varchar(2047), out _client_id int unsigned ) begin
	if( _is_default ) then
		update opc_clients set is_default=0;
	end if;
	insert into opc_clients( name,attributes,created,target,description,certificate_uri, is_default, url )
		values( _name,_attributes,CURRENT_TIMESTAMP(),_target,_description, _certificate_uri, _is_default,_url );
	set _client_id = LAST_INSERT_ID();
end
GO
drop trigger if exists opc_client_update;
GO
create trigger opc_client_update before update on opc_clients for each row
  if( NEW.is_default and (select count(*) from opc_clients where client_id!=NEW.client_id and is_default)>0 ) then
    set NEW.is_default = 0;
  end if;
