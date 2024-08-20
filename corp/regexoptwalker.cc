#include "regexopt.hh"

enum NodeType { STR_TYPE, OR_TYPE, AND_TYPE, ONE_TYPE, TWO_TYPE, SEPARATOR_TYPE };

class Node {
private:
	Node() {
		;
	}
	Node(NodeType ntype, Node *child = NULL) : type(ntype), first(child), last(child), next(NULL) {
		;
	}
public:
	~Node() {
		Node *curr = first;
		Node *next;
		while(curr != NULL) {
			next = curr->next;
			delete curr;
			curr = next;
		}
	}
	NodeType type;
	Node *first, *last, *next;
	string str;
	bool isRegex;

	void addChild(Node *child) {
		last->next = child;
		last = child;
	}

	static Node *createStr(string str, bool isRegex) {
		Node *n = new Node(STR_TYPE);
		n->str = str;
		n->isRegex = isRegex;
		return n;
	}
	static Node *createOr(Node *firstChild) {
		return new Node(OR_TYPE, firstChild);
	}
	static Node *createAnd(Node *firstChild) {
		return new Node(AND_TYPE, firstChild);
	}
	static Node *createOneAndMore(Node *child) {
		return new Node(ONE_TYPE, child);
	}
	static Node *createTwoAndMore(Node *child) {
		return new Node(TWO_TYPE, child);
	}
	static Node *createSeparator() {
		return new Node(SEPARATOR_TYPE);
	}

	friend std::ostream &operator<<(std::ostream &os, Node const &m);
};

std::ostream &operator<<(std::ostream &os, Node const &m) {
	switch(m.type) {
	case STR_TYPE:
		os << "STR";
		if(m.isRegex) {
			os << "re<" << m.str << ">";
		} else {
			os << "<" << m.str << ">";
		}
		break;
	case OR_TYPE:
		os << "OR";
		break;
	case AND_TYPE:
		os << "AND";
		break;
	case ONE_TYPE:
		os << "ONE";
		break;
	case TWO_TYPE:
		os << "TWO";
		break;
	case SEPARATOR_TYPE:
		os << "SEP";
		break;
	default:
		os << "UNK";
		break;
	}

	if(m.first) {
		os << "(";
		Node *curr = m.first;
		Node *next;
		while(curr != NULL) {
			next = curr->next;
			os << *curr;
			curr = next;
		}
		os << ")";
	}
	return os;
}

class TreeWalker {
private:
	Node *firstNode, *lastNode;
	vector<Node*> nodes;
	WordList *a;

	TreeWalker(WordList *a) : a(a) {
		nodes.reserve (32);
		firstNode = Node::createStr("^", false);
		lastNode = Node::createStr("$", false);
	}

	~TreeWalker() {
		nodes.clear();
		delete firstNode;
		delete lastNode;
	}

	void add(Node *n) {
		nodes.push_back(n);
	}

	bool hasNodes() {
		return !nodes.empty();
	}

	FastStream *nodes2fs () {
	    size_t len = nodes.size();
	    FastStream *f = NULL;
	    if (len <= 3) {
	        f = str2fs (0, len);
	    } else {
	    	f = str2fs (0, 3);
		    for (size_t i = 1; i <= len - 3; i++)
		        f = new QAndNode (f, str2fs (i, i + 3));
	    }
	    nodes.clear();
	    return f;
	}

	FastStream *str2fs (int from, int to) {
		bool isRegex = false;
	    vector<Node*>::const_iterator end = nodes.begin() + to;
	    for (vector<Node*>::const_iterator it = nodes.begin() + from; it != end; it++) {
	        if ((*it)->isRegex) {
	            isRegex = true;
	            break;
	        }
	    }
	    ostringstream oss;
	    for (vector<Node*>::const_iterator it = nodes.begin() + from; it != end; it++) {
	        if (!isRegex && (*it)->str[0] == '\\') {
	            oss << (*it)->str.substr(1);
	        } else {
	        	if (isRegex && ((*it)->str[0] == '^' || (*it)->str[0] == '$'))
	            	oss << '\\';
	            oss << (*it)->str;
	        }
	    }
	    string str = oss.str();
	// DEBUG: fprintf (stderr, "str2fs: %s, is_regex = %d\n", str.c_str(), is_regex);
	    if (!isRegex) {
	        return a->dynid2srcids (a->str2id (str.c_str()));
	    }
	    FastStream *fs = new Gen2Fast<int> (a->regexp2ids (str.c_str(), false));
	    Position id;
	    vector<FastStream*> *fsv = new vector<FastStream*>();
	    fsv->reserve (32);
	    while ((id = fs->next()) < fs->final())
	        fsv->push_back (a->dynid2srcids (id));
	    delete fs;
	    return QOrVNode::create (fsv);
	}

	FastStream *walk(Node *tree, bool isFirst, bool isLast) {
		FastStream *f = NULL;
		switch (tree->type) {
		case STR_TYPE:
			if (isFirst)
				add(firstNode);
			add(tree);
			if (isLast)
				add(lastNode);
			break;
		case OR_TYPE:
		{
			if (hasNodes()) {
				f = nodes2fs();
			}
			FastStream* p = walkOrNode(tree, isFirst, isLast);
			if (p != NULL && f != NULL) {
				f = new QAndNode(f, p);
			} else if (p != NULL) {
				f = p;
			}
			break;
		}
		case AND_TYPE:
			if (tree->first == tree->last) {
				f = walk(tree->first, isFirst, isLast);
			} else {
				for(Node *curr = tree->first; curr != NULL; curr = curr->next) {
					FastStream *p = walk(curr, isFirst && curr == tree->first, isLast && curr == tree->last);
					if (p != NULL && f != NULL)
						f = new QAndNode(f, p);
					else if (p != NULL)
						f = p;
				} 
			}
			if (hasNodes()) {
				FastStream *p = nodes2fs();
				if (f != NULL)
					f = new QAndNode(f, p);
				else
					f = p;
			}
			break;
		case TWO_TYPE:
		case ONE_TYPE:
			if (isFirst)
				add(firstNode);
			f = walk(tree->first, false, false);
			if (hasNodes()) {
				if (f != NULL) {
					f = new QAndNode(f, nodes2fs());
				} else {
					f = nodes2fs();
				}
			}
			// if (isLast)
			//	add(lastNode);
			break;
		case SEPARATOR_TYPE:
			if (hasNodes())
				return nodes2fs();
			break;
		default:
			throw new RegexOptException("unrecognized type in regexopt TreeWalker");
		}
		return f;
	}
	FastStream *walkOrNode(Node *orNode, bool isFirst, bool isLast) {
		FastStream *f = NULL;
		for(Node *curr = orNode->first; curr != NULL; curr = curr->next) {
			FastStream *p = walk(curr, isFirst, isLast);
			if (p != NULL && f != NULL) {
				f = new QOrNode(f, p);
			} else if (p != NULL) {
				f = p;
			}
		}
		return f;
	}
public:
	static FastStream *walk(WordList *a, Node *tree) {
		TreeWalker t(a);
		// std::cerr << *tree << std::endl;
		FastStream *f = t.walk(tree, true, true);
		return f;
	}
};
