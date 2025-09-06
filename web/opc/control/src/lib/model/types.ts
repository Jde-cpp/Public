export type OpcId = string;

export interface ILocalizedText{
	locale: string;
	text: string;
}

export interface IBrowseName{
	ns:number;
	name:String;
}

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
	BaseDataType = 24,
	FolderType = 61
}