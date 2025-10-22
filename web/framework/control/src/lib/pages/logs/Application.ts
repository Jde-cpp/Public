//import { LogEntry } from "./LogEntry";
import { TraceEntry } from "./TraceEntry";
import { Guid } from "jde-framework";
import * as AppFromClient from '../../proto/App.FromServer'; import FromServer = AppFromClient.Jde.App.Proto.FromServer;
import * as AppCommon from '../../proto/App'; import App = AppCommon.Jde.App.Proto;

export class ApplicationStrings{
	constructor()
	{}
	requests( entry:TraceEntry ):App.StringMD5s
	{
		var request = new App.StringMD5s();
		if( !this.messages.has(entry.messageId) ){
			request.messages.push( new Guid(entry.messageId).value );
			this.messages.set( entry.messageId, null );
		}
		if( entry.fileId && !this.files.has(entry.fileId) ){
			request.files.push( new Guid(entry.fileId).value );
			this.files.set( entry.fileId, null );
		}
		if( entry.functionId && !this.functions.has(entry.functionId) ){
			request.functions.push( new Guid(entry.functionId).value );
			this.functions.set( entry.functionId, null );
		}
		if( entry.userId && !this.users.has(entry.userId) ){
			request.userPKs.push( entry.userId );
			this.users.set( entry.userId, null );
		}
		return request;
	}

	set( strings:FromServer.Strings ){
		Object.entries( strings.messages ).forEach( ([id,v])=>this.messages.set( new Guid(id).toString(), v ) );
		Object.entries( strings.files ).forEach( ([id,v])=>this.files.set( new Guid(id).toString(), v ) );
		Object.entries( strings.functions ).forEach( ([id,v])=>this.functions.set( new Guid(id).toString(), v ) );
		Object.entries( strings.userTargets ).forEach( ([id,v])=>this.users.set( Number(id), v ) );
	}

	files:Map<string, string> = new Map<string, string>();
	functions:Map<string, string> = new Map<string, string>();
	messages:Map<string, string> = new Map<string, string>();
	users:Map<number, string> = new Map<number, string>();
}
/*
export class ApplicationInstance
{
	static parse( tokens:string[], startingIndex:number ):[ApplicationInstance, number]
	{
		var i = startingIndex;
		var app = new ApplicationInstance();
		app.id = parseInt( tokens[i++] );
		app.version = parseInt( tokens[i++] );
		var name = tokens[i++];
		app.application = Application.instances.get( name );
		if( !app.application )
		{
			app.application = new Application( name );
			Application.instances.set( name, app.application );
		}
		app.processId = parseInt( tokens[i++] );
		app.description = tokens[i++];
		app.hostName = tokens[i++];
		return [app,i];
	}
	parseStrings( tokens:string[], startingIndex:number ):number
	{
		let i = startingIndex;
		const typeCount = parseInt( tokens[i++] );
		for( let typeIndex=0; typeIndex<typeCount; ++typeIndex )
		{
			const type:LogEntry.EFields = parseInt( tokens[i++] );
			let map:Map<number,string> = null;
			switch( type )
			{
			case LogEntry.EFields.File:
				map = this.application.files;
				break;
			case LogEntry.EFields.Function:
				map = this.application.functions;
				break;
			case LogEntry.EFields.Message:
				map = this.application.messages;
				break;
			case LogEntry.EFields.Thread:
				map = this.threads;
				break;
			case LogEntry.EFields.User:
				map = this.application.users;
				break;
			default:
			 	throw new Error( 'type '+ (<number>type).toString()+' has not been implemented' );
			}
			const stringCount = parseInt( tokens[i++] );
			for( let stringIndex=0; stringIndex<stringCount; ++stringIndex )
			{
				const id = parseInt( tokens[i++] );
				const value = tokens[i++];
				map.set( id, value );
			}
		}
		return i;
	}
	id:number;
	application:Application;
	//Name:string;
	version:number;
	processId:number;
	description:string;
	hostName:string;
	threads:Map<number, string> = new Map<number, string>();
}
*/