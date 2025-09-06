import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as CommonProto from '../../proto/Common'; import ELogLevel = CommonProto.Jde.Proto.ELogLevel;

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
