#pragma once
#include <span>

#define let const auto
namespace Jde::IO
{
	template<class T=iv>
	struct Parser
	{
		//using bsv = std::basic_string_view<char,Traits>;
		Parser( T text, uint i_=0, uint iLine=1 )ι:Text{text}, i{i_},_line{iLine}{}
		α Next( char end )ι->T;
		α Next( sv x )ι->optional<uint>;
		α Next( const vector<sv>& phrases )ι->optional<uint>;
		α NextWords( const vector<T>& phrases, bool stem )ι->optional<Str::FindPhraseResult>;
		β Next( bool testQuote=true )ι->T;
		α Peek()ι->T{ return _peekValue.empty() ? _peekValue = Next() : _peekValue; }

		α Index()Ι{ return i;} α SetIndex( uint i_ )ι->void{i=i_;}
		α Line()Ι->uint{ return _line;}
		operator bool()Ι{ return i!=string::npos; }
		T Text;
	protected:
		α ResetPeek()ι->void{ if( _peekValue.size() ){ i = i-_peekValue.size(); _peekValue = {}; } }
		α LineIncrement( char ch )ι->void{ if( ch=='\n' ) ++_line; }

		uint i;
		uint _line;
		T _peekValue;
	};

	template<class T=iv>
	struct TokenParser : Parser<T>
	{
		using base=Parser<T>;
		using base::Text; using base::_peekValue; using base::i; using base::_line;
		TokenParser( T text, TokenParser* p, uint i_=0, uint iLine=1 )ι:base{ text, i_, iLine==1 && p ? p->Line() : iLine }, Tokens{p ? p->Tokens : vector<T>{}}{}
		TokenParser( T text, vector<T> tokens )ι:base{text}, Tokens{move(tokens)}{}
		α Next( bool testQuote=true )ι->T override;
		α NextToken( const vector<T>& tokens )ι->T;
		//α Peek()ι->T{ return base::_peekValue.empty() ? base::_peekValue = Next() : base::_peekValue; }
		α SetText( string x, uint index, uint line )ι{ _text = mu<string>( move(x) ); base::Text=*_text; base::i=index; base::_line=line; }
		const vector<T> Tokens;
	private:
		up<string> _text;
	};

#define $ template<class T> α Parser<T>
	$::Next( bool /*ignoreQuotes*/ )ι->T
	{
		ResetPeek();
		let v = Str::NextWordLocation( Text.substr(i) );
		if( v )
			i += get<1>( *v );
		else
			i = T::npos;
		return v ? get<0>( *v ) : T{};
	}

	$::Next( sv x )ι->optional<uint>
	{
		ResetPeek();
		let subLocation = Text.find( {x.data(), x.size()}, i );
		return subLocation==sv::npos ? nullopt : optional<uint>{ i=subLocation+x.size() };
	}

	$::NextWords( const vector<T>& phrases, bool stem )ι->optional<Str::FindPhraseResult>
	{
		ResetPeek();
		optional<Str::FindPhraseResult> y; //let start = i;
		bool complete;
		for( let& phrase : phrases )
		{
			let words = Str::Words<T>( phrase );
			auto end = [&](){ return (y && complete ? y->Start : Text.size())-i; };
			if( let pIndexes = Str::FindPhrase<T>({Text.data()+i, end()}, words, stem); pIndexes )//TODO cache the words FindPhrase parses.
			{
				if( let next = i+pIndexes->StartNextWord; !y || (next<y->StartNextWord && (!complete || pIndexes->NextEntry==words.size())) )
				{
					y = *pIndexes+i;
					complete = pIndexes->NextEntry==words.size();
				}
			}
		}
		if( y )
			i=y->StartNextWord;
		return y;
	}
	$::Next( const vector<sv>& phrases )ι->optional<uint>
	{
		ResetPeek();
		optional<uint> y;
		T text{Text};
		for( let& phrase : phrases )
		{
			if( let subLocation = text.find({phrase.data(), phrase.size()}, i); subLocation!=sv::npos )
			{
				text = { Text.data(), subLocation-i };
				y = i = subLocation+phrase.size();
			}
		}
		return y;
	}

	$::Next( char end )ι->T
	{
		T result;
		ResetPeek();
		for( auto ch = Text[i]; i<Text.size() && ch!=end && isspace(ch); ch = Text[++i] )
			LineIncrement( ch );

		if( i<Text.size() )
		{
			uint start = i;
			for( auto ch = Text[i]; i<Text.size() && ch!=end; ch = Text[++i] )
				LineIncrement( ch );

			++i;
			result = Text.substr( start, i-start );
		}
		return result;
	}
#undef $
#define $ template<class T> α TokenParser<T>
	$::Next( bool testQuote )ι->T
	{
		//using base::Text; using base::_peekValue; using base::i; using base::_line;
		T result = base::_peekValue;
		//uint i = base::Text.size();
		if( result.empty() && i<base::Text.size() )
		{
			for( auto ch = Text[i]; i<Text.size() && isspace(ch); ch = i<Text.size()-1 ? Text[++i] : Text[i++] )
				base::LineIncrement( ch );

			if( i<Text.size() )
			{
				uint start=i;
				if( testQuote && Text[i]=='"' )
				{
					++i;
					auto n = base::Next( '"' );
					result = Text.substr( start, n.size()+1 );
				}
				else
				{
					for( bool found = false; !found && i<Text.size(); ++i )
					{
						char ch = Text[i];
						base::LineIncrement( ch );
						for( auto p = Tokens.begin(); !found && p!=Tokens.end(); ++p  )
						{
							found = p->size()==1
								? ch==(*p)[0]
								: p->size()<=i-start+1 && Text.substr(i-p->size()+1, p->size())==*p;
						}
					}
					result = i==start ? Text.substr( i++, 1 ) : Text.substr( start, i-start );
				}
			}
		}
		else
			_peekValue = {};
		return result;
	}
	$::NextToken( const vector<T>& tokens )ι->T
	{
		T y;
		while( (y = Next()).size() )
		{
			if( std::find_if(tokens.begin(), tokens.end(), [&y](let& t){return y.ends_with(t);})!=tokens.end() )
				break;
		}
		return y;
	}
#undef let
#undef $
}