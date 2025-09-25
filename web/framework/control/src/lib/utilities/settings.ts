import { IProfile } from '../services/profile/IProfile';

export interface IAssignable<TUnderlying>
{
	assign( other:TUnderlying );
}
interface ILoad
{
	//load():Promise<any>;
	loadedPromise:Promise<boolean>;
}
export class Settings<TUnderlying extends IAssignable<TUnderlying>> //implements ILoad
{
	constructor( private type: new()=>TUnderlying, private key:string, private profile:IProfile  )
	{
		this.value = new this.type;
		this.loadedPromise = new Promise<boolean>( (resolve) => {this.resolve = resolve;} );
		this.load();
	}
	loadThen( fnctn:()=>any )//todo take out
	{
		this.load().then( ()=>{fnctn();} );
	}
	private load():Promise<void>
	{
		return new Promise( (resolve)=>
		{
			this.profile.get<TUnderlying>( this.key ).then( (value)=>
			{
				if( value )
				{
					try
					{
					    this.value.assign( value );
    					if( !this.original )
	    					this.original = new this.type;
		    			this.original.assign( value );
					}
					catch//changed settings
					{
						this.value = new this.type;
						this.original = null;
					}
				}
				else
					this.original = null;
				this.resolve(true);
				resolve();
			});
		} );
	}
	reset( suffix:string )
	{
		this.value = new this.type;
		const index = this.key.lastIndexOf( '.' );
		if( index!=-1 )
			this.key = `${this.key.substr(0,index)}.${suffix}`;
	}
	save()
	{
		if( !this.isLoaded ){ debugger;console.log(`tried to save unloaded settings ${this.key}.`); return; }
		const settings = JSON.stringify( this.value );
		const originalSettings = this.original ? JSON.stringify( this.original ) : this.defaultJson;
		const isDefault = settings==this.defaultJson;
		if( originalSettings!=settings )
		{
			console.log( `Saving settings '${this.key}'.` );
			this.profile.putJson( this.key, isDefault ? null : settings );
			if( !this.original )
				this.original = new this.type;
			else if( isDefault )
				this.original = null;
			if( this.original )
				this.original.assign( this.value );
		}
		else if( isDefault )
			this.original = null;
	}
	loadedPromise: Promise<boolean>;
	private resolve: Function|null = null;
	get isLoaded(){return this.original!==undefined;}
	get value(){return this._value;} set value(value){this._value=value;} private _value:TUnderlying;
	get defaultJson():string{ return this._defaultJson || (this._defaultJson=JSON.stringify(new this.type));} _defaultJson:string;
	private get original(){ return this._original;} private set original(value){this._original=value;} _original:TUnderlying;
}
export function JoinSettings( ...args: ILoad[] ):Promise<void>
{
	return new Promise<void>( async (resolve)=>
	{
		for( var a of args )
			await a.loadedPromise;
		resolve();
	} );
}