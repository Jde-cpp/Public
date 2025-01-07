namespace Jde{
	struct HiLow{
		HiLow( uint32 hi, uint32 low )ι:_combined{ (uint64_t) hi<<32 | low }{}
		HiLow( uint x )ι:_combined{ x }{}
		operator uint()Ι{return _combined;}
		α Hi()Ι->uint32{ return _combined>>32; }
		α Low()Ι->uint32{ return _combined & 0x00000000FFFFFFFF; }
		α operator<=>( const HiLow& )Ι=default;
	private:
		uint _combined;
	};
}