

export class std
{
	static accumulate( input: any[], initial:number, fnctn: (sum:number,element:any) => number ):number
	{
		let acc:number = initial;
		for( const element of input )
			acc = fnctn( acc, element );

		return acc
	}
}