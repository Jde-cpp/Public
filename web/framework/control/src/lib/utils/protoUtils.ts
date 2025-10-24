import Long from "long";
import * as AppFromServer from '../proto/App.FromServer'; import Google = AppFromServer.google.protobuf;

export type Timestamp = {seconds:number|Long; nanos:number;}
export type Duration = {seconds:number|Long; nanos:number;}
export class ProtoUtils{
	static toNumber( value:number|Long ):number{
		return typeof(value)==='object' ? value.toNumber() : value;
	}
	static toDate( value:Timestamp ):Date|null{
		const date = value==null || value.seconds==0 ? null : new Date( ProtoUtils.toNumber(value.seconds)*1000 + value.nanos/1000000 );
		return date;
	}
	static fromDate( value:Date ):Timestamp|null{
		return value
			? { seconds: Long.fromNumber(Math.floor(value.getTime() / 1000)), nanos: (value.getTime() % 1000) * 1000000 }
			: null;
	}
}