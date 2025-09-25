import Long from "long";
import * as AppFromServer from '../proto/App.FromServer'; import Google = AppFromServer.google.protobuf;
import { Guid } from "../model/Guid";

export class ProtoUtilities{
	static toNumber( value:number|Long ):number{
		return typeof(value)==='object' ? value.toNumber() : value;
	}
	static toDate( value:Google.ITimestamp ):Date|null{
		return value==null || value.seconds==0 ? null : new Date( ProtoUtilities.toNumber(value.seconds)*1000 + value.nanos/1000000 );
	}
}