export type OpcId = string;
export type Ns = number;
export type StatusCode = number;

export interface ILocalizedText{
	locale: string;
	text: string;
}

export const toLocalizedText = ( value:any ):ILocalizedText=>{
	return typeof(value)=="string" ? { locale: undefined, text: value } : value;
}

export type Browse = { ns?:number; name:String; }
export function toBrowse( path:string, defaultNs:Ns ):Browse{
	const segments = path.split("/");
	if( segments.length>1 )
		return toBrowse( segments[segments.length-1], defaultNs );
	const text = segments[0];
	const index = text.indexOf( "~" );
	if( index>0 ){
		const nsStr = text.substring( 0, index );
		if( /^-?\d+$/.test(nsStr) )
			return { ns: +nsStr, name: text.substring( index + 1 ) };
	}
	return { ns: defaultNs, name: text };
}
export function browseEq( a:Browse, b:Browse ):boolean{ return a.ns===b.ns && a.name===b.name; }

export enum EReferenceType{
  Organizes = 35,
	HasModelingRule = 37,
	HasSubType = 45,
	HasComponent = 47
}
export enum ETypes{
	None = 0,
	Boolean = 1,
	SByte = 2,
	Byte = 3,
	Int16 = 4,
	UInt16 = 5,
	Int32 = 6,
	UInt32 = 7,
	Int64 = 8,
	UInt64 = 9,
	Float = 10,
	Double = 11,
	String = 12,
	DateTime = 13,
	LocalizedText = 21,
	BaseData = 24,
	Folder = 61,
	UtcDateTime = 294
}