
namespace Jde::Opc{
	struct UADateTime{
		UADateTime( const UA_DateTime& dt )ι;
		UADateTime( const jvalue& json, SRCE )ε;
		UADateTime( const google::protobuf::Timestamp& timestamp )ι;
		UADateTime( const google::protobuf::Duration& duration )ι;
		α ToJson()Ι->jobject;
		α ToProto()Ι->google::protobuf::Timestamp;
		α ToDuration()Ι->google::protobuf::Duration;
		α UA()Ι->UA_DateTime;
	private:
		α ToParts()Ι->tuple<_int,int>;
		TimePoint _time;
	};
}