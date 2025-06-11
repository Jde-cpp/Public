#pragma once

namespace Jde::Opc::Server {
	using NodePK = uint;
	struct Node final{
		Node( NodePK pk, NodeId&& r )ι:
			NodeId{ pk },
			ExNode{ move(r) }
		{}

		Node( DB::Row&& r )ι:
			NodeId{ r.GetUInt(0) },
			ExNode{ r, 1, 2, 3, 4, 5, 6, 7 },
			IsGlobal{ r.GetBitOpt(8).value_or(false) },
			ParentNodeId{ r.GetUInt32Opt(9) },
			ReferenceTypeId{ r.GetUInt32Opt(10) },
			TypeDefId{ r.GetUInt32Opt(11) },
			OAttributeId{ r.GetUInt32Opt(12).value_or(0) },
			TypeAttribId{ r.GetUInt32Opt(13).value_or(0) },
			VAttributeId{ r.GetUInt32Opt(14).value_or(0) },
			Name{ r.GetString(15) }
		{}

		NodePK NodeId;
		Opc::NodeId ExNode;
		bool IsGlobal;
		optional<NodePK> ParentNodeId;
		optional<NodePK> ReferenceTypeId;
		optional<NodePK> TypeDefId;
		uint32 OAttributeId;
		uint32 TypeAttribId;
		uint32 VAttributeId;
		string Name;
	};
}