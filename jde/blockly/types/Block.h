#pragma once
#include <string>
#include <vector>
#include <span>
#include <bitset>
#include <boost/container/flat_set.hpp>
#include <boost/property_tree/ptree.hpp>
#include <jde/Exception.h>

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
		ImplArgs()=default;
		ImplArgs( bool haveNowParam ):HaveNowParam{haveNowParam}{}
		const bool HaveNowParam{ false };
		bool IsConst{ true };
		bool IsNoExcept{ true };
	};
	#define IMPL string Implementation( ImplArgs& impl )const noexcept(false)

	struct IBlock;
	struct File;
	#define ALARMS void Alarms( flat_set<string>& alarms )const noexcept
	#define TICKS void TickFields( TickBits& tickFields )const noexcept
	struct Field
	{
		Field( const ptree& element )noexcept;
		EValueType ValueType()const noexcept(false);

		string Name;
		string Id;
		string Type;
		string Value;
		constexpr static sv ElementName = "field";
	};
	struct Mutation
	{
		Mutation( const ptree& element )noexcept(false);
		string Name;
		constexpr static sv ElementName = "mutation";
	};
	struct Next
	{
		Next( const ptree& element, const File& file )noexcept;
		const IBlock& Block()const noexcept{ return *BlockPtr; }
		constexpr static sv ElementName = "next";
		IMPL;
		sp<const IBlock> BlockPtr;
	};

	struct Value;
	struct Values
	{
		uint size()const noexcept{ return Items.size(); }
		void emplace_back( const ptree& element, const File& file )noexcept(false);
		const Value& front()const noexcept{ return Items.front(); }
		const Value& operator[](sv sv)const noexcept(false);
		vector<Value> Items;
	};
	struct IBlock
	{
		IBlock( const ptree& element )noexcept(false);

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
		Statement( const ptree& data, const File& file )noexcept;
		ALARMS{ BlockPtr->Alarms(alarms); }
		IMPL;
		TICKS;
		string Name;
		sp<const IBlock> BlockPtr;
		constexpr static sv ElementName = "statement";
	};

/*	struct FieldOptionalBlock
	{
		optional<Field> FieldPtr;
		protected:
	   	FieldOptionalBlock( const ptree& element )noexcept(false);
	};*/

	struct NextBlock
	{
		optional<Blockly::Next> NextPtr;
	protected:
		NextBlock( const ptree& element, const File& file )noexcept(false);
		virtual ~NextBlock(){};
	};

	struct FieldBlock
	{
		Field Field;
		EValueType ValueType()const noexcept(false){ return Field.ValueType(); }
		protected:
	   	FieldBlock( const ptree& element );
	};

	struct IValueType2 : virtual IBlock //Block that can be in a value element, needs to have a field or mutation(Function call) or Values(Ternary)?
	{
		IValueType2( const ptree& element ):IBlock{ element }{}
		virtual EValueType ValueType()const noexcept(false)=0;
		virtual IMPL=0;
		virtual TICKS=0;
		static sp<const IValueType2> Factory( const ptree& element, const File& file )noexcept(false);
	};

	struct IValueType : IValueType2, FieldBlock
	{
		IValueType( const ptree& element )noexcept(false):IBlock{ element }, IValueType2{ element }, FieldBlock{ element }{}
		IMPL override;
		EValueType ValueType()const noexcept(false) override { return Field.ValueType(); }
	};


	struct Predicate : virtual IValueType2//returns a boolean
	{
		Predicate( const ptree& element )noexcept(false):IBlock{ element }, IValueType2{element}{}
		EValueType ValueType()const noexcept override{ return EValueType::Bool; }
		//virtual ALARMS=0;
		//virtual TICKS=0;
	};

	struct Value
	{
		Value( const ptree& element, const File& file )noexcept;
		EValueType ValueType()const noexcept(false){ return Ptr->ValueType(); }
		template<typename T>
		const T* TryType()const noexcept{ return dynamic_cast<const T*>(&Type()); }
		const IValueType2& Type()const noexcept{ return *Ptr; }
		IMPL{ return Ptr->Implementation( impl ); }
		string Name;
		sp<const IValueType2> Ptr;//IValueType + Field or mutation

		constexpr static sv ElementName = "value";
	};

	struct ValueBlock
	{
		Blockly::Value Value;
		protected:
	   	ValueBlock( const ptree& element, const File& file );
		private:
		//has a unused name
	};
	struct StatementBlock
	{
		StatementBlock( const ptree& element, const File& file )noexcept(false);
		optional<Blockly::Statement> StatementPtr;
	};
	struct ValuesBlock
	{
		Blockly::Values Values;
		protected:
	   	ValuesBlock( const ptree& element, const File& file )noexcept;
	};
	struct Block : virtual IBlock//Can have statements?
	{
		Block( const ptree& element, const File& file )noexcept(false);
		Block( const ptree& element )noexcept(false);
		static sp<const IBlock> Factory( const ptree& element, const File& file )noexcept(false);
		vector<Statement> Statements;
		optional<Field> Field;
		Blockly::Values Values;//Logic op has 2 values
		optional<Next> NextValue;
	protected:

	};

	struct ProcedureDefinition : virtual IBlock, FieldBlock, StatementBlock
	{
		ProcedureDefinition( const ptree& element, const File& file )noexcept:IBlock{ element }, FieldBlock{ element }, StatementBlock{ element, file }{};
		virtual ~ProcedureDefinition()=default;
		IMPL override{ THROW(Exception("not implemented")); }
		TICKS override;
		ALARMS override;
		const string& Name()const noexcept{ return Field.Value; }

		virtual std::ostringstream& Prototype( std::ostringstream& os, sv prefix={} )const noexcept(false);
		virtual std::ostringstream& Implementation( sv className, std::ostringstream& os, sv returnString={} )const noexcept(false);
		bool IsConst()const noexcept(false);
		bool IsNoExcept()const noexcept(false);

		static constexpr sv TypeId = "procedures_defnoreturn"sv;
	};

	struct ProcedureDefinitionReturn final : ProcedureDefinition, ValueBlock
	{
		ProcedureDefinitionReturn( const ptree& element, const File& file )noexcept:IBlock{ element }, ProcedureDefinition{ element, file }, ValueBlock{ element, file }{ /*DBG("ProcedureDefinitionReturn::{}"sv, Name());*/ }
		IMPL override{ THROW(Exception("not implemented")); }
		ALARMS override;
		TICKS override;
		EValueType ValueType()const noexcept(false){ return Value.ValueType(); }
		std::ostringstream& Implementation( sv className, std::ostringstream& os, sv returnString={} )const noexcept(false) override;
		static constexpr sv TypeId = "procedures_defreturn"sv;
	};

	struct ProcCall : IBlock, std::enable_shared_from_this<ProcCall>
	{
		ProcCall( const ptree& element, const File& file )noexcept(false);
		virtual ~ProcCall()=default;
		void AssignProc( const File& file )noexcept;

		IMPL override;
		TICKS override;
		ALARMS override;
		Blockly::Mutation Mutation;
		static constexpr sv TypeId = "procedures_callnoreturn"sv;
		sp<const ProcedureDefinition> Proc()const noexcept{ return _pProc; }
		void SetProc( sp<const ProcedureDefinition> p )noexcept{ _pProc = p; }
	protected:
		sp<const ProcedureDefinition> _pProc;
	};

	struct ProcCallReturn final : virtual ProcCall, virtual IValueType2
	{
		ProcCallReturn( const ptree& element, const File& file ): ProcCall{element, file}, ProcCall::IBlock{element}, IValueType2{ element }{}
		EValueType ValueType()const noexcept(false) override;
		IMPL override{ auto v = ProcCall::Implementation( impl ); return v.substr( 0, v.size()-2 ); }
		TICKS override{ ProcCall::TickFields( tickFields ); }
		ALARMS override{ ProcCall::Alarms( alarms ); }

		sp<const ProcedureDefinitionReturn> Proc()const noexcept{ return static_pointer_cast<const ProcedureDefinitionReturn>( _pProc ); }
		void SetProc( sp<const ProcedureDefinitionReturn> p )noexcept{ _pProc = p; }
		static constexpr sv TypeId = "procedures_callreturn"sv;
	};

	struct MathNumber final: IValueType
	{
		MathNumber( const ptree& element )noexcept : IBlock{ element }, IValueType( element ){};
		EValueType ValueType()const noexcept override{ return EValueType::Double; }
		TICKS override{}
		ALARMS override{}
		static constexpr sv TypeId = "math_number"sv;
	};

	struct SetMember : IBlock, FieldBlock, ValueBlock, NextBlock//has field & value, optional<next>
	{
		SetMember( const ptree& element, const File& file )noexcept : IBlock{element}, FieldBlock{ element }, ValueBlock{ element, file }, NextBlock{ element, file }{};
		IMPL override;
		string Implementation( ImplArgs& impl, optional<bool> includeTicks )const noexcept(false);
		TICKS override{}
		ALARMS override{}
		static constexpr sv Suffix = "_set"sv;
	};


	struct GetMember : IValueType//has field
	{
		GetMember( const ptree& element )noexcept : IBlock{ element }, IValueType{ element }{};
		IMPL override;
	};
	struct AccountMember final: GetMember
	{
		AccountMember( const ptree& element )noexcept: IBlock{element}, GetMember( element ){};
		TICKS override{}
		ALARMS override{};
		IMPL override;
		//EValueType ValueType()const noexcept override{ return EValueType::Amount; }
		static constexpr sv Prefix = "variables_account_"sv;
	};
	struct TickMember final: GetMember
	{
		TickMember( const ptree& element )noexcept:IBlock{element},GetMember( element ){};
		IMPL override;
		TICKS override;
		ALARMS override{}//nothing in the future.
		static constexpr sv Prefix = "variables_tick_"sv;
	};

	struct OrderMemberSet final : SetMember
	{
		OrderMemberSet( const ptree& element, const File& file )noexcept: SetMember( element, file ){};

		IMPL override;
		TICKS override{}
		ALARMS override{}
		//static constexpr sv Prefix = "variables_order_"sv;
	};

	struct OrderMember final : GetMember
	{
		OrderMember( const ptree& element )noexcept:IBlock{element}, GetMember( element ){};
		EValueType ValueType()const noexcept override{ return Field.Value=="order.lastUpdate" ? EValueType::TimePoint : EValueType::Price; }
		TICKS override{}
		ALARMS override{};
		IMPL override{ /*DBG("om={}"sv, Field.Value);*/ return format("{}{}", GetMember::Implementation( impl ), /*Field.Value=="order.lastUpdate"*/ true ? "()" : ""); }
		static constexpr sv Prefix = "variables_order_"sv;
	};

	struct LimitsMember final: GetMember
	{
		LimitsMember( const ptree& element )noexcept:IBlock{element},GetMember( element ){};
		EValueType ValueType()const noexcept override{ return EValueType::Price; }
		TICKS override{}
		ALARMS override{}
		static constexpr sv Prefix = "variables_limits_"sv;
	};

	struct PriceBlock final : IValueType
	{
		PriceBlock( const ptree& element )noexcept:IBlock{element},IValueType( element ){};
		IMPL override{ return format("PriceConst{{{}}}", Field.Value); }
		TICKS override{}
		ALARMS override{}
		EValueType ValueType()const noexcept override{ return EValueType::Price; }
		static constexpr sv TypeId = "variables_math_price"sv;
	};

	struct PlaceOrder final: IBlock
	{
		PlaceOrder( const ptree& element )noexcept:IBlock{element}{};
		IMPL override{ return "PlaceOrder();\n"; }
		TICKS override{}
		ALARMS override{}
		static constexpr sv TypeId = "variables_order_placeOrder"sv;
	};

	struct OptionOrder final : IBlock
	{
		OptionOrder( const ptree& element, const File& file )noexcept(false);

		IMPL override{ THROW(Exception("Can't implement base function 'OptionOrder'")); }
		TICKS override{}
		ALARMS override{}
		Blockly::Statement Statement;
	};

#define var const auto
	template<typename T>
	const T& Require( const auto& parent, sv description )noexcept(false)
	{
		var p = dynamic_cast<const T*>( &parent );
		THROW_IF( !p, Exception("{} does not have a {}.", description, GetTypeName<T>()) );
		return *p;
	}

	struct When : IBlock, ValueBlock, StatementBlock, NextBlock
	{
		When( const ptree& element, const File& file )noexcept(false):IBlock{ element }, ValueBlock{ element, file }, StatementBlock{ element, file }, NextBlock{ element, file }, If{ Require<Predicate>(Value.Type(), "When") } {}
		IMPL override;
		ALARMS override;
		TICKS override;
		const Predicate& If;
		static constexpr sv TypeId = "variables_events_when";
	};

	template<uint N, const std::array<sv,N>& TBlocklyStrings, const std::array<sv,N>& TImplStrings, typename TEnum>
	struct BinaryOperation : virtual IValueType2, ValuesBlock
	{
		static_assert(std::is_enum<TEnum>::value, "TEnum not an enum" );
		BinaryOperation( const ptree& element, const File& file )noexcept(false);
		IMPL override{ return format("({} {} {})", A.Implementation( impl ), TImplStrings[(uint)Operation], B.Implementation( impl )); }
		TICKS override{ A.Type().TickFields( tickFields ); B.Type().TickFields( tickFields ); }
		const TEnum Operation;
		const Value& A;//()const noexcept(false){ return Values["A"]; }
		const Value& B;//()const noexcept(false){ return Values["B"]; }
	};

	enum class ELogicOperation : uint8{ And, Or };
	static constexpr array<sv,2> LogicBlocklyStrings = { "AND"sv, "OR"sv };
	static constexpr array<sv,2> LogicOperationStrings = { "&&"sv, "||"sv };
	typedef BinaryOperation<2, LogicBlocklyStrings, LogicOperationStrings, ELogicOperation> LogicBase;
#pragma warning( disable : 4250 )
	struct Logic final : Predicate, LogicBase
	{
		Logic( const ptree& element, const File& file )noexcept(false);
//		virtual ~Logic()=default;
		IMPL override;
		ALARMS override;
		TICKS override;
		static constexpr sv TypeId = "logic_operation"sv;
	};
#pragma warning( default : 4250 )
	static constexpr array<sv,6> ArithmeticBlocklyStrings = { "ADD"sv, "MINUS"sv, "MULTIPLY"sv, "DIVIDE"sv, "POW"sv };
	static constexpr array<sv,6> ArithmeticOperationStrings = { "+"sv, "-"sv, "*"sv, "/"sv, "std::pow({},{})"sv };
	enum class EArithmeticOperation : uint8{ Add, Minus, Multiply, Divide, Power };
	typedef BinaryOperation<6, ArithmeticBlocklyStrings, ArithmeticOperationStrings, EArithmeticOperation> ArithmeticBase;
	struct Arithmetic final: ArithmeticBase
	{
		Arithmetic( const ptree& element, const File& file )noexcept;
		IMPL override{ return ArithmeticBase::Implementation( impl ); }
		ALARMS override;
		EValueType ValueType()const noexcept(false) override;
	};

	static constexpr array<sv,6> LogicCompareBlocklyStrings = { "EQ"sv, "NEQ"sv, "GT"sv, "GTE"sv, "LT"sv, "LTE"sv };
	static constexpr array<sv,6> LogicCompareOperationStrings = { "=="sv, "!="sv, ">"sv, ">="sv, "<"sv, "<="sv };
	enum class ELogicCompareOperation : uint8{ Equal, NotEqual, GreaterThan, GreaterThanOrEqual, LessThan, LessThanOrEqual };
	typedef BinaryOperation<6, LogicCompareBlocklyStrings, LogicCompareOperationStrings, ELogicCompareOperation> LogicCompareBase;
#pragma warning( disable : 4250 )
	struct LogicCompare final : Predicate, LogicCompareBase
	{
		LogicCompare( const ptree& element, const File& file )noexcept:IBlock{ element }, IValueType2{element}, Predicate{ element }, BinaryOperation{ element, file }{};
		IMPL override{ return LogicCompareBase::Implementation( impl ); }
		ALARMS override;
		TICKS override;
		static constexpr sv TypeId = "variables_logic_compare"sv;
	};
#pragma warning( default : 4250 )
	struct LogicNegate final : Predicate, ValueBlock
	{
		LogicNegate( const ptree& element, const File& file )noexcept:IBlock{ element }, IValueType2{ element }, Predicate{ element }, ValueBlock{ element, file }{};
		IMPL override{ return format( "!{}", Value.Implementation(impl) ); }
		ALARMS override;
		TICKS override;
		static constexpr sv TypeId = "logic_negate"sv;
	};

	struct Ternary final: IValueType2, ValuesBlock
	{
		Ternary( const ptree& element, const File& file )noexcept:IBlock{element}, IValueType2{ element }, ValuesBlock{ element, file }{};
		EValueType ValueType()const noexcept(false) override;
		IMPL override{ return format("{} ? {} : {}", If().Implementation( impl ), Then().Implementation( impl ), Else().Implementation( impl )); }
		TICKS override{ return If().Type().TickFields( tickFields ); Then().Type().TickFields( tickFields ); Else().Type().TickFields( tickFields ); }
		ALARMS override{ return If().Type().Alarms( alarms ); Then().Type().Alarms( alarms ); Else().Type().Alarms( alarms ); }
		const Value& If()const noexcept(false){ return Values["If"sv]; };
		const Value& Then()const noexcept(false){ return Values["Then"sv]; };
		const Value& Else()const noexcept(false){ return Values["Else"sv]; };

		constexpr static sv TypeName = "variables_logic_ternary";
	};

	struct TimeNow final: IValueType
	{
		TimeNow( const ptree& element )noexcept : IBlock{element}, IValueType{element}{};
		IMPL override{ return impl.HaveNowParam ? "now" : "ProcTimePoint::now()"; }
		TICKS override{}
		ALARMS override{}
		EValueType ValueType()const noexcept override{ return EValueType::TimePoint; }
		static constexpr sv TypeId = "variables_time_now"sv;
	};

	struct DurationMinutes final: IValueType
	{
		DurationMinutes( const ptree& element )noexcept : IBlock{element}, IValueType{element}{};
		IMPL override{ return format( "ProcDuration{{{}min}}", Field.Value ); }
		TICKS override{}
		ALARMS override{}
		EValueType ValueType()const noexcept override{ return EValueType::Duration; }
		static constexpr sv TypeId = "variables_time_minutes"sv;
	};

	struct TimeClose final: IValueType
	{
		TimeClose( const ptree& element )noexcept : IBlock{element}, IValueType{element}{};
		EValueType ValueType()const noexcept override{ return EValueType::TimePoint; }
		TICKS override{}
		ALARMS override{}//built-in alarm.
		IMPL override{ impl.IsNoExcept = false; return "ProcTimePoint::ClosingTime(Contract.TradingHoursPtr)"; }

		static constexpr sv TypeId = "variables_time_close"sv;
	};

	struct Variable
	{
		Variable( const ptree& element )noexcept;
		string Id;
		string Type;
		string Value;
		constexpr static sv ElementName = "variable";
		constexpr static sv VariableContainerName = "variables";
	};
	template<typename T>
	optional<T> OptFactory( const ptree& element )noexcept
	{
		optional<T> v;
		for( const auto& [elementName,subElement] : element.get_child("") )
		{
			if( elementName==T::ElementName )
			{
				THROW_IF( v, Exception("({})element occurs 2x+.", elementName) );
				v = T( subElement );
			}
		}
		return v;
	}
	template<typename T>
	T Factory( const ptree& element )noexcept(false)
	{
		auto v = OptFactory<T>( element );
		THROW_IF( !v, Exception("Could not find element '{}'.", T::ElementName) );
		return v.value();
	}

	template<typename T>
	optional<T> OptFactory( const ptree& element, const File& file )noexcept(false)
	{
		optional<T> v;
		for( const auto& [elementName,subElement] : element.get_child("") )
		{
			if( elementName==T::ElementName )
			{
				THROW_IF( v, Exception("({})element occurs 2x+.", elementName) );
				v = T( subElement, file );
			}
		}
		return v;
	}

	template<typename T>
	T Factory( const ptree& element, const File& file )noexcept(false)
	{
		auto v = OptFactory<T>( element, file );
		THROW_IF( !v, Exception("Could not find element '{}'.", T::ElementName) );
		return v.value();
	}

	inline uint8 GetOperation( const ptree& element, sv Id, std::span<const sv> blocklyStrings )
	{
		var field = Blockly::Factory<Blockly::Field>( element );
		THROW_IF( field.Name!="OP", Exception("({})Expecting single field named OP.", Id) );
		var operation = std::distance( blocklyStrings.begin(), std::find(blocklyStrings.begin(), blocklyStrings.end(), field.Value) );
		THROW_IF( (uint8)operation>=(uint8)blocklyStrings.size(), Exception("({})OP value '{}' not implemented.", Id, field.Value) );
		return (uint8)operation;
	}

	template<uint N, const std::array<sv,N>& TBlocklyStrings, const std::array<sv,N>& TImplStrings, typename TEnum>
	BinaryOperation<N,TBlocklyStrings, TImplStrings, TEnum>::BinaryOperation( const ptree& element, const File& file )noexcept(false):
		IBlock{ element },
		ValuesBlock{ element, file },
		Operation{ GetOperation(element, Id, TBlocklyStrings) },
		A{ Values["A"] },
		B{ Values["B"] }
	{

	}
#undef var
#undef IMPL
}