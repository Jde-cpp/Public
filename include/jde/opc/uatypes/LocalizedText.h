
namespace Jde::Opc{
	struct LocalizedText final : UA_LocalizedText{
		LocalizedText()ι{ UA_LocalizedText_init(this); }
		LocalizedText( sv text, sv locale={} )ι:UA_LocalizedText{ AllocUAString(locale), AllocUAString(text) }{}
		LocalizedText( LocalizedText&& x )ι:UA_LocalizedText{ x }{ UA_LocalizedText_init( &x ); }
		~LocalizedText(){ UA_LocalizedText_clear(this); }

		Ω ToJson( const UA_LocalizedText& ua )ι->jobject{
			jobject o;
			if( ua.text.length ){
				if( ua.locale.length )
					o["locale"] = Opc::ToString( ua.locale );
				o["text"] = Opc::ToString( ua.text );
			}
			return o;
		}
		α ToJson()Ι->jobject{ return ToJson( *this ); }
	};
}