#pragma once
#include <string>
#include <vector>
#include <span>
#include <bitset>
#include <boost/container/flat_set.hpp>
#include <boost/property_tree/ptree.hpp>
#include <jde/Exception.h>
#include <jde/io/tinyxml2.h>

namespace Jde::Blockly
{
	using std::ostream;
	using boost::property_tree::ptree;
	using boost::container::flat_set;
	typedef std::bitset<91> TickBits;
	enum class EValueType : uint8//EFundamentals
	{
		None=0,
		Bool=1,
		Double=2,
		Amount=3,
		Price=4,
		TimePoint=5,
		Duration=6,
		Size=7
	};
	constexpr array<sv,6> ValueTypeStrings = { "void"sv, "bool"sv, "double"sv, "Amount"sv, "Price"sv, "ProcTimePoint"sv };
	inline sv ToString( EValueType t ){ return ValueTypeStrings[(uint8)t]; }

	string MemberName( const string& Id, const string& v )noexcept(false);

	struct ImplArgs
	{
		const bool HaveNowParam{ false };
		bool IsConst{ true };
		bool IsNoExcept{ true };
		const bool IsMember{ true };
		bool CallStatic{ false };
		const bool TickTrigger{ false };
	};
#define IMPL α Implementation( ImplArgs& impl )Ε->string
	enum EStaticParams{ Prototype, Local, Member,  };
#define STATIC_PARAMS α StaticParams( EStaticParams type={} )Ι->string
#define TICK_TRIGGER_NAMES α TickTriggerNames( flat_set<string>& names )Ι->void
#define TRIGGER_VARS α TriggerVariables( flat_set<string>& x )Ι->void
#define var const auto
	struct IBlock;
	struct File;
	#define ALARMS void Alarms( flat_set<string>& alarms )Ι
	#define TICKS void TickFields( TickBits& tickFields, bool includeFunctions=true )Ι
	struct Field
	{
		Field( const Xml::XMLElement& e )noexcept;
		EValueType ValueType()Ε;

		string Name;
		string Id;
		string Type;
		string Value;
		constexpr static sv ElementName = "field";
	};
	struct Mutation
	{
		Mutation( const Xml::XMLElement& e )noexcept(false);
		string Name;
		constexpr static sv ElementName = "mutation";
	};
	struct Next
	{
		Next( const Xml::XMLElement& e, const File& file )noexcept;
		const IBlock& Block()const noexcept{ return *BlockPtr; }
		constexpr static sv ElementName = "next";
		IMPL;
		sp<const IBlock> BlockPtr;
	};

	struct Value;
	struct Values
	{
		uint size()const noexcept{ return Items.size(); }
		void emplace_back( const Xml::XMLElement& e, const File& file )noexcept(false);
		const Value& front()const noexcept{ return Items.front(); }
		const Value& operator[](sv sv)Ε;
		vector<Value> Items;
	};
	struct IBlock
	{
		IBlock( const Xml::XMLElement& e )noexcept(false);

		virtual ALARMS=0;
		virtual IMPL=0;
		virtual TICKS=0;
		string TypeId;
		string Id;
		bool Deletable;

		constexpr static sv ElementName = "block";
	};

	struct Statement
	{
		Statement( const Xml::XMLElement& data, const File& file )noexcept;
		ALARMS{ BlockPtr->Alarms(alarms); }
		IMPL;
		TICKS;
		string Name;
		sp<const IBlock> BlockPtr;
		constexpr static sv ElementName = "statement";
	};

	struct NextBlock
	{
		optional<Blockly::Next> NextPtr;
	protected:
		NextBlock( const Xml::XMLElement& e, const File& file )noexcept(false);
		virtual ~NextBlock(){};
	};

	struct FieldBlock
	{
		Field Field;
		EValueType ValueType()Ε{ return Field.ValueType(); }
		protected:
	   	FieldBlock( const Xml::XMLElement& e );
	};

	struct IValue : virtual IBlock //Block that can be in a value e, needs to have a field or mutation(Function call) or Values(Ternary)?
	{
		IValue( const Xml::XMLElement& e ):IBlock{ e }{}
		virtual EValueType ValueType()Ε=0;
		virtual IMPL=0;
		virtual TICKS=0;
		virtual STATIC_PARAMS{ return {}; }
		virtual TICK_TRIGGER_NAMES=0;
		virtual TRIGGER_VARS=0;
		static sp<const IValue> Factory( const Xml::XMLElement& e, const File& file )noexcept(false);
	};

	struct IValueField : IValue, FieldBlock
	{
		IValueField( const Xml::XMLElement& e )noexcept(false):IBlock{ e }, IValue{ e }, FieldBlock{ e }{}
		IMPL override;
		TICK_TRIGGER_NAMES override{}
		EValueType ValueType()Ε override { return Field.ValueType(); }
	};

	struct Predicate : virtual IValue//returns a boolean
	{
		Predicate( const Xml::XMLElement& e )noexcept(false):IBlock{ e }, IValue{e}{}
		EValueType ValueType()const noexcept override{ return EValueType::Bool; }
	};

	struct Value final
	{
		Value( const Xml::XMLElement& e, const File& file )noexcept;
		EValueType ValueType()Ε{ return Ptr->ValueType(); }
		Ŧ TryType()const noexcept->const T*{ return dynamic_cast<const T*>(&Type()); }
		const IValue& Type()const noexcept{ return *Ptr; }
		IMPL{ return Ptr->Implementation( impl ); }
		STATIC_PARAMS{ return Ptr->StaticParams( type ); }
		TICK_TRIGGER_NAMES{ Ptr->TickTriggerNames( names ); }
		TRIGGER_VARS{ Ptr->TriggerVariables( x ); }
		string Name;
		sp<const IValue> Ptr;//IValueField + Field or mutation

		constexpr static sv ElementName = "value";
	};

	struct ValueBlock
	{
		Blockly::Value Value;
		virtual STATIC_PARAMS{ return Value.StaticParams(); }
		virtual TICK_TRIGGER_NAMES{ Value.TickTriggerNames( names ); }
		virtual TRIGGER_VARS{ return Value.TriggerVariables( x ); }
		protected:
	   	ValueBlock( const Xml::XMLElement& e, const File& file );
	};

	struct StatementBlock
	{
		StatementBlock( const Xml::XMLElement& e, const File& file )noexcept(false);
		optional<Blockly::Statement> StatementPtr;
	};
	struct ValuesBlock
	{
		Blockly::Values Values;
		protected:
	   	ValuesBlock( const Xml::XMLElement& e, const File& file )noexcept;
	};
	struct Block : virtual IBlock//Can have statements?
	{
		Block( const Xml::XMLElement& e, const File& file )noexcept(false);
		Block( const Xml::XMLElement& e )noexcept(false);
		static sp<const IBlock> Factory( const Xml::XMLElement& e, const File& file )noexcept(false);
		vector<Statement> Statements;
		optional<Field> Field;
		Blockly::Values Values;//Logic op has 2 values
		optional<Next> NextValue;
	protected:

	};

	struct ProcedureDefinition : virtual IBlock, FieldBlock, StatementBlock
	{
		ProcedureDefinition( const Xml::XMLElement& e, const File& file )noexcept:IBlock{ e }, FieldBlock{ e }, StatementBlock{ e, file }{};
		virtual ~ProcedureDefinition()=default;
		IMPL override{ THROW("not implemented"); }
		TICKS override;
		ALARMS override;
		const string& Name()const noexcept{ return Field.Value; }

		/*virtual*/ string Prototype( sv className={}, ImplArgs args={} )Ε;
		virtual STATIC_PARAMS{ return {}; }
		virtual TRIGGER_VARS{}
		virtual string Implementation( str className, ImplArgs args={}, str returnString={} )Ε;
		bool IsConst()Ε;
		bool IsNoExcept()Ε;
		virtual bool IsTickTrigger()Ε{ return false; }
		static constexpr sv TypeId = "procedures_defnoreturn"sv;
	};

	struct ProcedureDefinitionReturn final : ProcedureDefinition, ValueBlock
	{
		ProcedureDefinitionReturn( const Xml::XMLElement& e, const File& file )ι;
		IMPL override{ THROW("not implemented"); }
		ALARMS override;
		TICKS override;
		TRIGGER_VARS override;
		STATIC_PARAMS override{ return Value.StaticParams( type ); }
		bool IsTickTrigger()Ε override{ TickBits tickFields; TickFields( tickFields, true ); return tickFields.any(); }
		EValueType ValueType()Ε{ return Value.ValueType(); }
		string Implementation( str className, ImplArgs args={}, str returnString={} )Ε override;

		static constexpr sv TypeId = "procedures_defreturn"sv;
	};

	struct ProcCall : IBlock, std::enable_shared_from_this<ProcCall>
	{
		ProcCall( const Xml::XMLElement& e, const File& file )noexcept(false);
		virtual ~ProcCall()=default;
		void AssignProc( const File& file )noexcept;

		IMPL override;
		TICKS override;
		ALARMS override;
		TRIGGER_VARS;
		Blockly::Mutation Mutation;
		static constexpr sv TypeId = "procedures_callnoreturn"sv;
		sp<const ProcedureDefinition> Proc()const noexcept{ return _pProc; }
		void SetProc( sp<const ProcedureDefinition> p )noexcept{ _pProc = p; }
	protected:
		sp<const ProcedureDefinition> _pProc;
	};

	struct ProcCallReturn final : virtual ProcCall, virtual IValue
	{
		ProcCallReturn( const Xml::XMLElement& e, const File& file ): ProcCall{e, file}, ProcCall::IBlock{e}, IValue{ e }{}
		EValueType ValueType()Ε override;
		IMPL override{ auto v = ProcCall::Implementation( impl ); return v.substr( 0, v.size()-2 ); }
		STATIC_PARAMS override{ return {}; }
		TICK_TRIGGER_NAMES override{ TickBits tickFields; TickFields( tickFields ); if( tickFields.any() && _pProc ) names.emplace( _pProc->Name() ); }
		TRIGGER_VARS override{ ProcCall::TriggerVariables( x ); }
		TICKS override{ ProcCall::TickFields( tickFields, includeFunctions ); }
		ALARMS override{ ProcCall::Alarms( alarms ); }

		sp<const ProcedureDefinitionReturn> Proc()const noexcept{ return static_pointer_cast<const ProcedureDefinitionReturn>( _pProc ); }
		void SetProc( sp<const ProcedureDefinitionReturn> p )noexcept{ _pProc = p; }
		static constexpr sv TypeId = "procedures_callreturn"sv;
	};

	struct MathNumber final: IValueField
	{
		MathNumber( const Xml::XMLElement& e )noexcept : IBlock{ e }, IValueField( e ){};
		EValueType ValueType()const noexcept override{ return EValueType::Double; }
		TICKS override{}
		ALARMS override{}
		TRIGGER_VARS override{}
		static constexpr sv TypeId = "math_number"sv;
	};

	struct SetMember : IBlock, FieldBlock, ValueBlock, NextBlock//has field & value, optional<next>
	{
		SetMember( const Xml::XMLElement& e, const File& file )noexcept : IBlock{e}, FieldBlock{ e }, ValueBlock{ e, file }, NextBlock{ e, file }
		{
			DEBUG_IF( Id=="limits_priceLimit_set" );
		};
		IMPL override;
		string Implementation( ImplArgs& impl, optional<bool> includeTicks )Ε;
		TICKS override{}
		ALARMS override{}
		α IsProperty()Ε->bool{ return !Field.Value.starts_with( "limits" ); }
		static constexpr sv Suffix = "_set"sv;
	};


	struct GetMember : IValueField//has field
	{
		GetMember( const Xml::XMLElement& e )noexcept : IBlock{ e }, IValueField{ e }{};
		IMPL override;
		STATIC_PARAMS override;
		TICK_TRIGGER_NAMES override{}
		α IsProperty()Ε->bool{ return !Field.Value.starts_with( "limits" ); }
		TRIGGER_VARS override;
	};
	struct AccountMember final: GetMember
	{
		AccountMember( const Xml::XMLElement& e )noexcept: IBlock{ e }, GetMember{ e }{};
		TICKS override{}
		ALARMS override{}

		IMPL override;
		static constexpr sv Prefix = "variables_account_"sv;
	};
	struct TickMember final: GetMember
	{
		TickMember( const Xml::XMLElement& e )noexcept:IBlock{e},GetMember{ e }{};
		IMPL override;
		TICKS override;
		TRIGGER_VARS override{}
		STATIC_PARAMS override;
		ALARMS override{}//nothing in the future.
		static constexpr sv Prefix = "variables_tick_"sv;
	};

	struct OrderMemberSet final : SetMember
	{
		OrderMemberSet( const Xml::XMLElement& e, const File& file )noexcept: SetMember( e, file ){};

		IMPL override;
		TICKS override{}
		ALARMS override{}
		//static constexpr sv Prefix = "variables_order_"sv;
	};

	struct OrderMember final : GetMember
	{
		OrderMember( const Xml::XMLElement& e )noexcept:IBlock{e}, GetMember{ e }{};
		EValueType ValueType()const noexcept override{ return Field.Value=="order.lastUpdate" ? EValueType::TimePoint : EValueType::Price; }
		TICKS override{}
		ALARMS override{};
		IMPL override{ return format("{}{}", GetMember::Implementation( impl ), impl.IsMember ? "()" : ""); }
		static constexpr sv Prefix = "variables_order_"sv;
	};

	struct LimitsMember final: GetMember
	{
		LimitsMember( const Xml::XMLElement& e )noexcept:IBlock{e},GetMember{ e }{};
		EValueType ValueType()const noexcept override{ return EValueType::Price; }
		TICKS override{}
		ALARMS override{}
		static constexpr sv Prefix = "variables_limits_"sv;
	};

	struct PriceBlock final : IValueField
	{
		PriceBlock( const Xml::XMLElement& e )noexcept:IBlock{e},IValueField( e ){};
		IMPL override{ return format("PriceConst{{{}}}", Field.Value); }
		//STATIC_PARAMS override{ return {}; }
		TICK_TRIGGER_NAMES override{}
		TRIGGER_VARS override{}
		TICKS override{}
		ALARMS override{}
		EValueType ValueType()const noexcept override{ return EValueType::Price; }
		static constexpr sv TypeId = "variables_math_price"sv;
	};

	struct PlaceOrder final: IBlock
	{
		PlaceOrder( const Xml::XMLElement& e )noexcept:IBlock{e}{};
		IMPL override{ return "\t\t\tLOG( \"({})Limit={}, BidPrice={}, AskPrice={}\", Order.OrderId(), Order.Limit().ToString(), Tick.BidPrice().ToString(), Tick.AskPrice().ToString() );\n\t\t\tPlaceOrder();\n"; }
		TICKS override{}
		ALARMS override{}
		static constexpr sv TypeId = "variables_order_placeOrder"sv;
	};

	struct OptionOrder final : IBlock
	{
		OptionOrder( const Xml::XMLElement& e, const File& file )noexcept(false);

		IMPL override{ THROW("Can't implement base function 'OptionOrder'"); }
		TICKS override{}
		ALARMS override{}
		Blockly::Statement Statement;
	};

#define var const auto
	Ŧ Require( const auto& parent, sv description, SRCE )noexcept(false)->const T&
	{
		var p = dynamic_cast<const T*>( &parent );
#ifdef _MSC_VER
		if( !p ) throw Exception{ sl, "does not have a {}.", description };
#else
		THROW_IF( !p, "{} does not have a {}.", description, GetTypeName<T>() ); //MSVC does not like
#endif
		return *p;
	}

	struct When : IBlock, ValueBlock, StatementBlock, NextBlock
	{
		When( const Xml::XMLElement& e, const File& file )noexcept(false):IBlock{ e }, ValueBlock{ e, file }, StatementBlock{ e, file }, NextBlock{ e, file }, If{ Require<Predicate>(Value.Type(), "When") } {}
		IMPL override;
		ALARMS override;
		TICKS override;
		TICK_TRIGGER_NAMES override{ If.TickTriggerNames( names ); }
		const Predicate& If;
		static constexpr sv TypeId = "variables_events_when";
	};

	template<uint N, const std::array<sv,N>& TBlocklyStrings, const std::array<sv,N>& TImplStrings, typename TEnum>
	struct BinaryOperation : virtual IValue, ValuesBlock
	{
		static_assert(std::is_enum<TEnum>::value, "TEnum not an enum" );
		BinaryOperation( const Xml::XMLElement& e, const File& file )noexcept(false);
		IMPL override{ return format("({} {} {})", A.Implementation( impl ), TImplStrings[(uint)Operation], B.Implementation( impl )); }
		TICKS override
		{
			A.Type().TickFields( tickFields, includeFunctions );
			var& t = B.Type();
			t.TickFields( tickFields, includeFunctions );
		}
		TRIGGER_VARS override{ A.Type().TriggerVariables( x ); B.Type().TriggerVariables( x ); }

		STATIC_PARAMS override{ vector<string> v{ A.StaticParams(type), B.StaticParams(type) }; v.erase( std::remove_if(v.begin(), v.end(), [](var&x){return x.empty();}), v.end() ); return Str::AddSeparators(v, ", "); }
		TICK_TRIGGER_NAMES override{ A.TickTriggerNames( names ), B.TickTriggerNames( names ); }
		const TEnum Operation;
		const Value& A;//()Ε{ return Values["A"]; }
		const Value& B;//()Ε{ return Values["B"]; }
	};

	enum class ELogicOperation : uint8{ And, Or };
	static constexpr array<sv,2> LogicBlocklyStrings = { "AND"sv, "OR"sv };
	static constexpr array<sv,2> LogicOperationStrings = { "&&"sv, "||"sv };
	using LogicBase=BinaryOperation<2, LogicBlocklyStrings, LogicOperationStrings, ELogicOperation>;
#pragma warning( disable : 4250 )
	struct Logic final : Predicate, LogicBase
	{
		Logic( const Xml::XMLElement& e, const File& file )noexcept(false);
		IMPL override;
		ALARMS override;
		TICKS override;
		static constexpr sv TypeId = "logic_operation"sv;
	};
#pragma warning( default : 4250 )
	static constexpr array<sv,6> ArithmeticBlocklyStrings = { "ADD"sv, "MINUS"sv, "MULTIPLY"sv, "DIVIDE"sv, "POW"sv };
	static constexpr array<sv,6> ArithmeticOperationStrings = { "+"sv, "-"sv, "*"sv, "/"sv, "std::pow({},{})"sv };
	enum class EArithmeticOperation : uint8{ Add, Minus, Multiply, Divide, Power };
	using ArithmeticBase=BinaryOperation<6, ArithmeticBlocklyStrings, ArithmeticOperationStrings, EArithmeticOperation>;
	struct Arithmetic final: ArithmeticBase
	{
		Arithmetic( const Xml::XMLElement& e, const File& file )noexcept;
		IMPL override{ return ArithmeticBase::Implementation( impl ); }
		ALARMS override;
		EValueType ValueType()Ε override;
	};

	static constexpr array<sv,6> LogicCompareBlocklyStrings = { "EQ"sv, "NEQ"sv, "GT"sv, "GTE"sv, "LT"sv, "LTE"sv };
	static constexpr array<sv,6> LogicCompareOperationStrings = { "=="sv, "!="sv, ">"sv, ">="sv, "<"sv, "<="sv };
	enum class ELogicCompareOperation : uint8{ Equal, NotEqual, GreaterThan, GreaterThanOrEqual, LessThan, LessThanOrEqual };
	using LogicCompareBase=BinaryOperation<6, LogicCompareBlocklyStrings, LogicCompareOperationStrings, ELogicCompareOperation>;
#pragma warning( disable : 4250 )
	struct LogicCompare final : Predicate, LogicCompareBase
	{
		LogicCompare( const Xml::XMLElement& e, const File& file )noexcept:IBlock{ e }, IValue{e}, Predicate{ e }, BinaryOperation{ e, file }{};
		IMPL override{ return LogicCompareBase::Implementation( impl ); }
		ALARMS override;
		TICKS override;
		static constexpr sv TypeId = "variables_logic_compare"sv;
	};
#pragma warning( default : 4250 )
	struct LogicNegate final : Predicate, ValueBlock
	{
		LogicNegate( const Xml::XMLElement& e, const File& file )noexcept:IBlock{ e }, IValue{ e }, Predicate{ e }, ValueBlock{ e, file }{};
		IMPL override{ return format( "!{}", Value.Implementation(impl) ); }
		ALARMS override;
		TICKS override;
		STATIC_PARAMS override{ return ValueBlock::StaticParams( type ); }
		TICK_TRIGGER_NAMES override{ return ValueBlock::TickTriggerNames( names ); }
		TRIGGER_VARS override{ return ValueBlock::TriggerVariables( x ); }
		static constexpr sv TypeId = "logic_negate"sv;
	};

	struct Ternary final: IValue, ValuesBlock
	{
		Ternary( const Xml::XMLElement& e, const File& file )noexcept:IBlock{e}, IValue{ e }, ValuesBlock{ e, file }{};
		EValueType ValueType()Ε override;
		IMPL override{ return format("{} ? {} : {}", If().Implementation( impl ), Then().Implementation( impl ), Else().Implementation( impl )); }
		STATIC_PARAMS override{ vector<string> v{ If().StaticParams(type), Then().StaticParams(type), Else().StaticParams(type) }; v.erase( std::remove_if(v.begin(), v.end(), [](var&x){return x.empty();}), v.end() ); return Str::AddSeparators(v, ", "); }
		TICK_TRIGGER_NAMES override{ If().TickTriggerNames( names ); Then().TickTriggerNames( names ); Else().TickTriggerNames( names ); }
		TRIGGER_VARS override{ If().TickTriggerNames( x ); Then().TickTriggerNames( x ); Else().TickTriggerNames( x ); }
		TICKS override{ return If().Type().TickFields( tickFields, includeFunctions ); Then().Type().TickFields( tickFields, includeFunctions ); Else().Type().TickFields( tickFields, includeFunctions ); }
		ALARMS override{ return If().Type().Alarms( alarms ); Then().Type().Alarms( alarms ); Else().Type().Alarms( alarms ); }
		const Value& If()Ε{ return Values["If"sv]; };
		const Value& Then()Ε{ return Values["Then"sv]; };
		const Value& Else()Ε{ return Values["Else"sv]; };

		constexpr static sv TypeName = "variables_logic_ternary";
	};

	struct TimeNow final: IValueField
	{
		TimeNow( const Xml::XMLElement& e )noexcept : IBlock{e}, IValueField{e}{};
		IMPL override{ return impl.HaveNowParam ? "now" : "ProcTimePoint::now()"; }
		TICKS override{}
		ALARMS override{}
		TICK_TRIGGER_NAMES override{}//TODO move to base class
		TRIGGER_VARS override{}
		EValueType ValueType()const noexcept override{ return EValueType::TimePoint; }
		static constexpr sv TypeId = "variables_time_now"sv;
	};

	struct DurationMinutes final: IValueField
	{
		DurationMinutes( const Xml::XMLElement& e )noexcept : IBlock{e}, IValueField{e}{};
		IMPL override{ return format( "ProcDuration{{{}min}}", Field.Value ); }
		TICKS override{}
		ALARMS override{}
		TRIGGER_VARS override{}
		EValueType ValueType()const noexcept override{ return EValueType::Duration; }
		static constexpr sv TypeId = "variables_time_minutes"sv;
	};

	struct TimeClose final: IValueField
	{
		TimeClose( const Xml::XMLElement& e )noexcept : IBlock{e}, IValueField{e}{};
		EValueType ValueType()const noexcept override{ return EValueType::TimePoint; }
		TICKS override{}
		TRIGGER_VARS override{}
		ALARMS override{}//built-in alarm.
		TICK_TRIGGER_NAMES override{}//TODO move to base class
		IMPL override{ impl.IsNoExcept = false; return "ProcTimePoint::ClosingTime(Contract.TradingHoursPtr)"; }

		static constexpr sv TypeId = "variables_time_close"sv;
	};

	struct Variable
	{
		Variable( const Xml::XMLElement& e )ι;
		string Id;
		string Type;
		string Value;
		constexpr static sv ElementName = "variable";
		constexpr static sv VariableContainerName = "variables";
	};
	#pragma warning( disable : 4297 )
	Ŧ OptFactory( const Xml::XMLElement& e )noexcept->	optional<T>
	{
		optional<T> v;
		for( var* c = e.FirstChildElement(T::ElementName); c; c = c->NextSiblingElement(T::ElementName) )
		{
			THROW_IF( v, "({})element occurs 2x+.", T::ElementName );
			v = T( *c );
		}
		return v;
	}
	Ŧ Factory( const Xml::XMLElement& e )noexcept(false)->T
	{
		auto v = OptFactory<T>( e );
		THROW_IF( !v, "Could not find element '{}'.", T::ElementName );
		return v.value();
	}

	Ŧ OptFactory( const Xml::XMLElement& e, const File& file )noexcept(false)->optional<T>
	{
		optional<T> y;
		for( var* c = e.FirstChildElement(T::ElementName); c; c = c->NextSiblingElement(T::ElementName) )
		{
			THROW_IF( y, "({})element occurs 2x+.", T::ElementName );
			y = T( *c, file );
		}
		return y;
	}

	Ŧ Factory( const Xml::XMLElement& e, const File& file )noexcept(false)->T
	{
		auto v = OptFactory<T>( e, file );
		THROW_IF( !v, "Could not find element '{}'.", T::ElementName );
		return v.value();
	}

	Ξ GetOperation( const Xml::XMLElement& e, sv Id, std::span<const sv> blocklyStrings )->uint8
	{
		var field = Blockly::Factory<Blockly::Field>( e );
		THROW_IF( field.Name!="OP", "({})Expecting single field named OP.", Id );
		var operation = std::distance( blocklyStrings.begin(), std::find(blocklyStrings.begin(), blocklyStrings.end(), field.Value) );
		THROW_IF( (uint8)operation>=(uint8)blocklyStrings.size(), "({})OP value '{}' not implemented.", Id, field.Value );
		return (uint8)operation;
	}

	template<uint N, const std::array<sv,N>& TBlocklyStrings, const std::array<sv,N>& TImplStrings, typename TEnum>
	BinaryOperation<N,TBlocklyStrings, TImplStrings, TEnum>::BinaryOperation( const Xml::XMLElement& e, const File& file )noexcept(false):
		IBlock{ e },
		ValuesBlock{ e, file },
		Operation{ GetOperation(e, Id, TBlocklyStrings) },
		A{ Values["A"] },
		B{ Values["B"] }
	{

	}
#undef var
#undef IMPL
#undef STATIC_PARAMS
#undef TICK_TRIGGER_NAMES
#undef TRIGGER_VARS
}