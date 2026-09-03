

export class std
{
	static accumulate<T>( input: readonly T[], initial:number, fnctn: (sum:number,element:T) => number ):number
	{
		let acc:number = initial;
		for( const element of input )
			acc = fnctn( acc, element );

		return acc
	}
}