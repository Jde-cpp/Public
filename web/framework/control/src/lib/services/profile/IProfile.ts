
export interface IProfile{
	get<T>( key:string ):Promise<T>;
	put<T>( name:string, value:T ):void;
	putJson( name:string, json:string ):void;
}