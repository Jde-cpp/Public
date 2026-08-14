
export class StringUtils{
	static capitalize( value:string ):string{
		return value.charAt(0).toUpperCase()+value.slice(1);
	}
	static toJson( value:string ):string{
		return value[0].toLowerCase() + value.substring(1);
	}
	static plural( value:string ):string{
		return value.endsWith('s') ? value : value+'s';
	}
	static singular( value:string ):string{
		return value.endsWith('s') ? value.slice(0,-1) : value;
	}
	//GraphQL string literals share JSON's escape grammar (\" \\ \b \f \n \r \t \uXXXX), so JSON.stringify IS the correct escaper here - a hand-built `"${value}"` corrupts the query on any quote, backslash, or newline in user text.
	static qlString( value:string ):string{ return JSON.stringify( value ); }
	static idToDisplay( id:string ){ //TODO change function name
		let y=''
		for( let i=0; i<id.length; ++i )
		{
			let ch = id[i];
			if( i>0 )
			{
				if( ch==ch.toUpperCase() )
					y+=" ";
				y+=ch;
			}
			else
				y+=ch.toUpperCase();
		}
		return y;
	}
}