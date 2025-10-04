import {Sort} from '@angular/material/sort';
import { IAssignable } from '../../utilities/settings';

import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as LogProto from '../../proto/Log'; import Log = LogProto.Jde.App.Log.Proto;

export class LogSettings implements IAssignable<LogSettings>{
	constructor( params:LogSettings=null ){
		if( !params )
			return;
		if( params.sort )
			this.sort = params.sort;
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
		if( params.limit )
			this.limit = params.limit;
	}
	assign( other: LogSettings ){
		this.sort = other.sort;
		this.autoScroll = other.autoScroll;
		this.applicationId = other.applicationId;
		this.level = other.level;
		this.limit = other.limit;
		this.hiddenMessages = [...other.hiddenMessages];
		this.start = other.start;
	}

	sort:Sort = {active: "time", direction: "asc"};
	autoScroll:boolean=true;
	applicationId;
	level:Log.ELogLevel=Log.ELogLevel.Information;
	limit:number=5000;
	hiddenMessages:string[]=[];
	get start():Date{ return this._start || LogSettings.defaultDate; } set start( value:Date ){ this._start=value==LogSettings.defaultDate ? null : value;} private _start:Date;
	static get defaultDate():Date{ var start = new Date(); start.setHours( 0, 0, 0, 0 ); start.setDate( start.getDate()-1 ); return start; }
}
