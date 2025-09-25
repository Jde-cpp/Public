import { Log } from "../IGraphQL";

export abstract class IErrorService
{
	//abstract show( e:any ):void;
	abstract error( e:string, log:Log ):void;
	abstract exception( e:any, log:Log ):void;
	abstract exceptionInfo( e:any, info:string, log:Log ):void;
	abstract assert( condition:boolean, log:Log ):void;
	abstract warn( message: string, log:Log ):void;
	abstract info( message:string, log:Log ):void;
}