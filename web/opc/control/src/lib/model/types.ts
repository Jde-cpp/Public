export type OpcId = string;
export type Ns = number;
export type StatusCode = number;

export interface ILocalizedText{
	locale: string;
	text: string;
}
export enum EAccess{
	None = 0,
	Read = 1,
	Write = 2,
	HistoryRead = 4,
	HistoryWrite = 8,
	SemanticChange = 0x10,
	StatusWrite = 0x20,
	TimestampWrite = 0x40,
	All = 0x7F
}
export enum EWriteAccess{
	None = 0,
	Access = 1,
	ArrayDimensions = 2,
	BrowseName = 4,
	ContainsNoLoops = 8,
	DataType = 0x10,
	Description = 0x20,
	DisplayName = 0x40,
	EventNotifier = 0x80,
	Executable = 0x100,
	Historizing = 0x200,
	InverseName = 0x400,
	MinimumSamplingInterval = 0x800,
	NodeClass = 0x1000,
	Symmetric = 0x2000,
	UserAccessLevel = 0x4000,
	UserExecutable = 0x8000,
	UserWriteMask = 0x10000,
	ValueRank = 0x20000,
	WriteMask = 0x40000,
	ValueForVariableType = 0x80000,
	DataTypeDefinition = 0x100000,
	RolePermissions = 0x200000,
	AccessRestrictions = 0x400000,
	AccessLevelEx = 0x800000,
	All = 0xFFFFFF
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