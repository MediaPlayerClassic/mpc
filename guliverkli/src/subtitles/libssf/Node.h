#pragma once

namespace ssf
{
	template <class T = CString, class S = CString> 
	class CAtlStringMap : public CAtlMap<S, T, CStringElementTraits<S> >
	{
	public:
		CAtlStringMap() {}
		CAtlStringMap(const CAtlStringMap& s2t) {*this = s2t;}
		CAtlStringMap& operator = (const CAtlStringMap& s2t)
		{
			RemoveAll();
			POSITION pos = s2t.GetStartPosition();
			while(pos) {const CAtlStringMap::CPair* p = s2t.GetNext(pos); SetAt(p->m_key, p->m_value);}
			return *this;
		}
	};

	class Definition;
	class NodeFactory;

	enum NodePriority {PLow, PNormal, PHigh};

	class Node
	{
	protected:
		const NodeFactory* m_pnf;

	public:
		CAtlList<Node*> m_nodes;
		CString m_type, m_name;
		NodePriority m_priority;

		Node(const NodeFactory* pnf, CString name);
		virtual ~Node() {}

		bool IsNameUnknown();
		bool IsTypeUnknown();
		bool IsType(CString type);

		virtual void AddTail(Node* pNode);
		virtual void GetChildDefs(CAtlList<Definition*>& l, LPCTSTR type = NULL, bool fFirst = true);
		virtual void Dump(NodePriority minpriority = PNormal, int level = 0, bool fLast = false) = 0;
	};

	class Reference : public Node
	{
	public:
		Reference(const NodeFactory* pnf, CString name);
		virtual ~Reference();

		void GetChildDefs(CAtlList<Definition*>& l, LPCTSTR type = NULL, bool fFirst = true);
		void Dump(NodePriority minpriority = PNormal, int level = 0, bool fLast = false);
	};

	class Definition : public Node
	{
	public:
		template<typename T> struct Number {T value; int sign; CString unit;};
		struct Time {Number<double> start, stop;};

		enum status_t {node, string, number, boolean, block};

	private:
		status_t m_status;
		bool m_autotype;
		CString m_value, m_unit;

		CAtlStringMap<Definition*> m_type2def;
		void RemoveFromCache(LPCTSTR type = NULL);

		template<typename T> 
		void GetAsNumber(Number<T>& n, CAtlStringMap<T>* n2n = NULL);

	public:
		Definition(const NodeFactory* pnf, CString name);
		virtual ~Definition();

		void AddTail(Node* pNode);
		void Dump(NodePriority minpriority = PNormal, int level = 0, bool fLast = false);

		Definition& operator[] (LPCTSTR type);

		bool IsValue(status_t s = (status_t)0);

		void SetAsValue(status_t s, CString v, CString u = _T(""));
		void SetAsNumber(CString v, CString u = _T(""));

		void GetAsString(CString& str);
		void GetAsNumber(Number<int>& n, CAtlStringMap<int>* n2n = NULL);
		void GetAsNumber(Number<DWORD>& n, CAtlStringMap<DWORD>* n2n = NULL);
		void GetAsNumber(Number<double>& n, CAtlStringMap<double>* n2n = NULL);
		template<typename T> 
		void GetAsNumber(T& t, CAtlStringMap<T>* n2n = NULL) {Number<T> n; GetAsNumber(n, n2n); t = n.value; if(n.sign) t *= n.sign;}
		void GetAsBoolean(bool& b);
		bool GetAsTime(Time& t, CAtlStringMap<double>& offset, CAtlStringMap<double>* n2n = NULL, int default_id = 0);

		operator LPCTSTR();
		operator double();
		operator bool();
	};
}
