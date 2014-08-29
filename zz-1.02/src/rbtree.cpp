#include "rbtree.hpp"

const ZString *RBTree::NodeB::ins_data = NULL;
int RBTree::NodeB::cmp = 0;

bool RBTree::insert(const ZString &d)
{
	if(!root)
	{
		root = new NodeB(new ZString(d), NULL, NULL);
		return true;
	}
	NodeB::ins_data = new ZString(d);
	root = root->insert();
	if(!NodeB::cmp)
	{// fail
		delete NodeB::ins_data;
		return false;
	}
	//win
	return true;
}

RBTree::NodeB *RBTree::NodeB::insert()
{
	cmp = data->compare(*ins_data);
	if(!cmp)	return this;
	if(cmp<0)
	{
		if(!left)
		{
			left = new NodeR(ins_data, NULL, NULL);
			cmp = 1;// OK
			return this;
		}
		left = left->insert();
		if(cmp >= 0)	return this;
		NodeB *p = new NodeR(data, left->right, right);
		left->right = p;
		p = left;
		delete this;
		cmp = 1;// OK
		return p;
	}
	return NULL;
}

RBTree::NodeB *RBTree::NodeR::insert()
{
	cmp = data->compare(*ins_data);
	if(!cmp)	return this;
	if(cmp<0)
	{
		if(!left)
		{
			NodeB *t = new NodeB(data, new NodeR(ins_data, NULL, NULL), right);
			delete this;
			cmp = -1;
			return t;
		}
	}
	return NULL;
}

void RBTree::NodeB::clear()
{
	if(left)
		left->clear();
	if(right)
		right->clear();
	delete data;
}

RBTree::~RBTree()
{
	if(root)
		root->clear();
}
