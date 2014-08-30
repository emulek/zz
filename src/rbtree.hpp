#ifndef RBTREE_HPP
#define RBTREE_HPP

#include "zstring.hpp"

class RBTree
{
	private:
		class NodeB
		{
			friend class RBTree;
			private:
				NodeB(const NodeB &) {}
				const NodeB &operator=(const NodeB &) { return *this; }
			protected:
				const ZString *data;
				NodeB *left;
				NodeB *right;
				void clear();
				static const ZString *ins_data;
				static int cmp;
				virtual NodeB *insert();
			public:
				NodeB(const ZString *d, NodeB *l, NodeB *r) : data(d), left(l), right(r)
				{}
				virtual ~NodeB()
				{}
		};
		class NodeR : public NodeB
		{
			private:
				NodeR(const NodeR &) : NodeB(NULL,NULL,NULL) {}
				const NodeR &operator=(const NodeR &) { return *this; }
			protected:
				virtual NodeB *insert();
			public:
				NodeR(const ZString *d, NodeB *l, NodeB *r) : NodeB(d, l, r)
				{}
				virtual ~NodeR()
				{}
		};
		RBTree(const RBTree &) {}
		const RBTree &operator=(const RBTree &) { return *this; }
		NodeB *root;
	public:
		RBTree() : root(NULL)
		{}
		bool insert(const ZString &d);
		virtual ~RBTree();
};

#endif
