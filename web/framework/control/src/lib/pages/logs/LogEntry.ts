import {ApplicationStrings} from './Application';
/*
export class LogEntry
{
	static parse( tokens:string[], startingIndex:number, instances:Map<number, ApplicationInstance> ):[LogEntry, number,RequestStrings]
	{
		var entry = new LogEntry();
		var i = startingIndex;
		var flags:LogEntry.EFields = parseInt(tokens[i++]);
		var applicationId = parseInt( tokens[i++] );
		entry.applicationInstance = instances.get( applicationId );
		if( !entry.applicationInstance )
			throw new Error( "Do not have application instance"+applicationId.toString() );
		var application = entry.applicationInstance.application;
		var requestStrings = new RequestStrings( applicationId );
		if( flags & LogEntry.EFields.Level )
			entry.level = LogEntry.ELogLevel[tokens[i++]];
		if( flags & LogEntry.EFields.MessageId )
			entry.messageId = parseInt(tokens[i++]);
		if( flags & LogEntry.EFields.FileId )
			entry.fileId = parseInt(tokens[i++]);
		if( flags & LogEntry.EFields.FunctionId )
			entry.functionId = parseInt(tokens[i++]);
		if( flags & LogEntry.EFields.LineNumber )
			entry.lineNumber = parseInt(tokens[i++]);
		if( flags & LogEntry.EFields.UserId )
			entry.userId = parseInt(tokens[i++]);
		if( flags & LogEntry.EFields.ThreadId )
			entry.threadId = parseInt(tokens[i++]);
		if( flags & LogEntry.EFields.Timestamp )
			entry.timestamp = new Date( parseInt(tokens[i++]) );
		if( flags & LogEntry.EFields.Thread )
			entry.applicationInstance.threads.set( entry.threadId, tokens[i++] );
		if( flags & LogEntry.EFields.User )
			application.users.set( entry.userId, tokens[i++] ); //entry.user = tokens[i++];
		else if( entry.userId && !application.users.has(entry.userId) )
			requestStrings.requests.push( [LogEntry.EFields.User, entry.userId] );
		if( flags & LogEntry.EFields.Message )
			application.messages.set( entry.messageId, tokens[i++] ); //entry.user = tokens[i++];
		else if( entry.messageId && !application.messages.has(entry.messageId) )
			requestStrings.requests.push( [LogEntry.EFields.Message, entry.messageId] );
		if( flags & LogEntry.EFields.File )
			application.files.set( entry.fileId, tokens[i++] ); //entry.user = tokens[i++];
		else if( entry.fileId && !application.files.has(entry.fileId) )
			requestStrings.requests.push( [LogEntry.EFields.File, entry.fileId] );
		if( flags & LogEntry.EFields.Function )
			application.functions.set( entry.functionId, tokens[i++] );
		else if( entry.functionId && !application.functions.has(entry.functionId) )
			requestStrings.requests.push( [LogEntry.EFields.Function, entry.functionId] );

		if( flags & LogEntry.EFields.VariableCount )
		{
			entry.variables = new Array<string>();
			let variableCount = parseInt(tokens[i++]);
			for( let variableIndex=0; variableIndex<variableCount; ++variableIndex )
				entry.variables.push( tokens[i++] );
		}

		return [entry,i,requestStrings];
	}
	applicationInstance:ApplicationInstance;
	foo: string="Test Message";
	timestamp: Date;
	messageId: number;
	level:LogEntry.ELogLevel;
	fileId: number;
	//file:string;
	functionId: number;
	lineNumber: number;
	userId: number;
	threadId: number;
	variables:string[];

	get application():Application{ return this.applicationInstance.application; }
	get functionName():string{ return this.functionId ? this.application.functions.get(this.functionId) : null; }
	get user():string{ return this.userId ? this.application.users.get(this.userId) : null; }

	get thread():string{ return this.threadId ? this.applicationInstance.threads.get(this.threadId) : null; }
	get file():string{ return this.fileId ? this.application.files.get(this.fileId) : null; }
	get message():string
	{ 
		var message = this.messageId ? this.application.messages.get(this.messageId) : null;
		if( message )
		{
			if( this.variables )
			{
				let i; let j=0;
				while( (i=message.indexOf("{}"))!=-1 && j<this.variables.length )
				{
					const foo = message.substring(0, i)+this.variables[j++] + ((i<message.length-2) ? message.substring(0, i+2) : "");
					message = foo;
				}
			}
		}
		return message;
	}
}
export namespace LogEntry
{
	export enum ELogLevel
	{
		Trace = 0,
		Debug = 1,
		Information = 2,
		Warning = 3,
		Error = 4,
		Critical = 5,
		None = 6
	}
	export enum EFields
	{
		None=0,
		Timestamp=0x1,
		MessageId=0x2,
		Message=0x4,
		Level=0x8,
		FileId=0x10,
		File=0x20,
		FunctionId=0x40,
		Function=0x80,
		LineNumber=0x100,
		UserId=0x200,
		User=0x400,
		ThreadId=0x800,
		Thread=0x1000,
		VariableCount=0x2000
	}
}

export class RequestStrings
{
	constructor( applicationInstanceId ){this.applicationInstanceId=applicationInstanceId;}
	applicationInstanceId:number;
	requests:Array<[LogEntry.EFields,number]> = new Array<[LogEntry.EFields,number]>();
	toArray( strings:Array<string> ):void
	{
		strings.push( this.applicationInstanceId.toString() );
		strings.push( this.requests.length.toString() );
		this.requests.forEach(entryId => 
		{
			strings.push( entryId[0].toString() );
			strings.push( entryId[1].toString() );
		});
	}
}
*/
