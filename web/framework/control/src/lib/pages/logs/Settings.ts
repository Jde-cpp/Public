import {Sort} from '@angular/material/sort';
//import { IAssignable } from '../../utils/settings';

import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as LogProto from '../../proto/Log'; import Log = LogProto.Jde.App.Log.Proto;
import { Guid } from '../../model/Guid';

export class LogSettings{
	constructor( params:LogSettings=null ){
		if( !params )
			return;
		if( params.autoScroll )
			this.autoScroll = params.autoScroll;
		if( params.applicationId )
			this.applicationId = params.applicationId;
		if( params.level )
			this.level = params.level;
		if( params.start )
			this.start = params.start;
		if( params.hiddenMessages )
			this.hiddenMessages = params.hiddenMessages;
	}
	assign( other: LogSettings ){
		this.autoScroll = other.autoScroll;
		this.applicationId = other.applicationId;
		this.level = other.level;
		this.hiddenMessages = [...other.hiddenMessages];
		this.start = other.start;
	}

	autoScroll:boolean=true;
	applicationId:number|undefined;
	level:Log.ELogLevel=Log.ELogLevel.Information;
	hiddenMessages:Guid[]=[];
	get start():Date{ return this._start || LogSettings.defaultDate; } set start( value:Date ){ this._start=value==LogSettings.defaultDate ? null : value;} private _start:Date;
	static get defaultDate():Date{ var start = new Date(); start.setHours( 0, 0, 0, 0 ); start.setDate( start.getDate()-1 ); return start; }
}
