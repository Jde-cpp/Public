import Long from "long";
import * as AppFromServer from '../proto/App.FromServer'; import Google = AppFromServer.google.protobuf;

export class ProtoUtilities{
	static toNumber( value:number|Long ):number{
		return typeof(value)==='object' ? value.toNumber() : value;
	}
	static toDate( value:Google.ITimestamp ):Date|null{
		const date = value==null || value.seconds==0 ? null : new Date( ProtoUtilities.toNumber(value.seconds)*1000 + value.nanos/1000000 );
		return date;
	}
	static fromDate( value:Date ):Google.ITimestamp|null{
		if(!value) return null;
		return value
			? { seconds: Long.fromNumber(Math.floor(value.getTime() / 1000)), nanos: (value.getTime() % 1000) * 1000000 }
			: null;
	}
}