#pragma once

#include "Node.h"

namespace ssf
{
	class NodeFactory
	{
		Reference* m_root;
		CAtlStringMap<Node*> m_nodes;
		CAtlList<CString> m_newnodes;
		NodePriority m_priority;

		unsigned __int64 m_counter;
		CString GenName();

	public:
		NodeFactory();
		virtual ~NodeFactory();

		virtual void RemoveAll();

		void SetDefaultPriority(NodePriority priority) {m_priority = priority;}

		void Commit();
		void Rollback();

		Reference* CreateRootRef();
		Reference* GetRootRef() const;
		Reference* CreateRef(Definition* pParentDef);
		Definition* CreateDef(Reference* pParentRef = NULL, CString type = _T(""), CString name = _T(""), NodePriority priority = PNormal);
		Definition* GetDefByName(CString name) const;

		void Dump(NodePriority priority) const;
	};
}
