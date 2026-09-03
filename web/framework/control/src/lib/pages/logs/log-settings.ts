import {Sort} from '@angular/material/sort';
//import { IAssignable } from '../../utils/settings';

import * as FromServer from 'jde-proto/App.FromServer';
import * as Log from 'jde-proto/Log';
import { Guid } from '../../model/guid';

export class LogSettings{
	constructor( params:LogSettings|undefined=undefined ){
		if( !params )
			return;
		if( params.autoScroll )
			this.autoScroll = params.autoScroll;
		if( params.applicationId )
			this.applicationId = params.applicationId;
		if( params.level!=undefined )//Trace is 0 - a truthy test dropped it and silently restored the default
			this.level = params.level;
		if( params.start )
			this.start = params.start;
		if( params.hiddenMessages )
			this.hiddenMessages = params.hiddenMessages.map( g=>Guid.fromJson(g) ).filter( g=>g!=undefined );
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
	//Trace, ie show everything: the level is only applied as a filter now, and nothing filtered the first page before.
	level:Log.ELogLevel=Log.ELogLevel.Trace;
	hiddenMessages:Guid[]=[];
	get start():Date{ return this._start || LogSettings.defaultDate; } set start( value:Date ){ this._start=value==LogSettings.defaultDate ? undefined : value;} private _start:Date|undefined;
	static get defaultDate():Date{ var start = new Date(); start.setHours( 0, 0, 0, 0 ); start.setDate( start.getDate()-1 ); return start; }
}
