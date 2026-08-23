import * as FromServer from 'jde-proto/App.FromServer';
import { ELogLevel } from 'jde-proto/Log';

export interface Instance{
	application?:string;
	host:string;
	pid?:number;
	dbDefaultLogLevel?:ELogLevel;
	fileDefaultLogLevel?:ELogLevel;
	startTime?:Date;
	port?:number;
	instanceName?:string;
}
