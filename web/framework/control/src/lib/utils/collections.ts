import { IAssignable } from './settings';

export function toMap<T>( array:T[], getKey:(x:T)=>any ):Map<any,T>
{
	let results = new Map<any,T>();
	array.forEach( x=>results.set( getKey(x),x ) );
	return results;
}

export class CircularBuffer<T> implements IAssignable<CircularBuffer<T>>
{
	constructor( private _maxLength:number=3, private values:T[]=[] )
	{}
	add( x:T, begining:boolean )
	{
		this.remove( x );
		if( begining )
			this.values.unshift(x);
		else
			this.values.push(x);
		if( this.values.length>this.maxLength )
			this.values.length=this.maxLength;
	}
	assign( x:CircularBuffer<T> )
	{
		//this._maxLength=x.maxLength;
		for( let i=x.values.length-1; i>=0; --i )
			this.unshift( x.values[i] );
	}
	indexOf( x:T ){ return this.values.indexOf(x); }
	forEach( fnctn:(value:T, index:number, array:T[])=>void ){ this.values.forEach( (value,index,array)=>fnctn(value, index, array) ); }
	push( x:T ){ this.add( x, true ); }
	remove( x:T )
	{
		const index = this.values.indexOf( x );
		if( index!=-1 )
			this.values.splice( index, 1 );
	}
	unshift( x:T ){ this.add( x, true ); }

	get length(){ return this.values.length; }
	get maxLength(){ return this._maxLength; }
}