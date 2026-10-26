// JeffLib::DataDrivenClass (vtable 0x7c5388, constructor 0x6b2d40, destructor 0x6b2da0, loader 0x6b3820; code
// 0x6b2a90-0x6b4fd0): the base of the classes that take their data from an XML file (the credits; the XLayout
// loader of the screens' layouts is the same code). The derived class says which tags it knows, as a tree: the
// root's tag, the groups inside it (a tag that has tags of its own) and the elements (a tag that has none)
// inside each group, with a function for each that the loader calls as it reads the file. A group has one
// function for its opening and one for its closing; an element gets its attributes.
//
//   <Main>                  the root (its name is the derived class's choice)
//     <Credit>              a group of the root: AddGroup(nullptr, ...)
//       <Settings .../>     an element of the group: AddElement(aCredit, ...)
//       <Column>            a group of that group
//         <Entry .../>
//
// The loader acts on the tags in the order they come, as the original's did: a tag that is the name of a
// group opens that group (the group is looked for under the one open now), any other tag is an element of the
// open group, and a tag that is not known there is passed over. Names are compared as written.
//
// Two slots of the original's vtable are for reloading the file while the game runs (0x6b3e50 reloads every
// object made, from a function nothing calls): the first before the data is read again (slot 0, 0x6b2b00,
// empty here) and the second after it (slot 1, empty at 0x4c8420).
//
// port: the original also kept every object and the file it was loaded from in a global table (0x6b36f0), for
// that reload, and could bind an element's attributes to variables of the derived class by name (ints, floats,
// bools and strings: 0x6b3600-0x6b36c0, set by 0x6b2b10) when it had no function; the credits have a function
// for every tag, so the bound variables are not ported. The loader reads the file with the framework's
// XMLParser, whose attributes come sorted by name, the order the original handed them over in.
#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace JeffLib {

class DataDrivenClass {
public:
	// an element's attributes, in the order the file's parser gives them (sorted by name); the keys as written
	typedef std::vector<std::pair<std::string, std::string> > AttributeList;
	// port: a bound member function in the original (two words, the object and the function)
	typedef std::function<void(const AttributeList&)> ElementHandler;
	typedef std::function<void()> GroupHandler;

	struct Element {
		std::string mName;
		ElementHandler mHandler;
	};
	// a group's node (0x4c bytes in the original)
	struct Node {
		std::string mName;
		GroupHandler mStart;               // +0x44, +0x48: when its tag opens
		GroupHandler mEnd;                 // +0x3c, +0x40: when its tag closes
		std::vector<Node*> mGroups;        // +0x30: the groups in it
		std::vector<Element> mElements;    // +0x20: the elements in it
		Node() {}
		~Node();
	private:
		Node(const Node&);
		Node& operator=(const Node&);
	};

	DataDrivenClass();                     // 0x6b2d40
	virtual ~DataDrivenClass();            // 0x6b2da0

	// slot 0, 0x6b2b00: the file is about to be read again
	virtual void BeforeReload() {}
	// slot 1, 0x4c8420: it was read again
	virtual void AfterReload() {}

	// 0x6b3820: reads theFileName, calling the functions of the tags as they come. False when it cannot be opened.
	bool Load(const std::string& theFileName);
	// 0x6b3e50 for this object: BeforeReload, Load of the file it was loaded from, AfterReload
	bool Reload();

protected:
	Node mRoot;  // +0x04: the root's tag is its name (set by the derived class)

	// 0x6b3280: a group inside theParent (the root when null), with the functions for its tag opening and closing
	Node* AddGroup(Node* theParent, const std::string& theName, const GroupHandler& theStart,
	               const GroupHandler& theEnd);
	// 0x6b3420: an element inside theGroup (the root when null)
	void AddElement(Node* theGroup, const std::string& theName, const ElementHandler& theHandler);

private:
	std::string mFileName;
	std::vector<std::string> mGroupNames;  // +0x50: the name of every group registered, for IsGroup

	bool IsGroup(const std::string& theName) const;   // 0x6b2f60
	Node* FindGroup(const std::string& theName, Node* theNode);   // 0x6b3040: theNode or a group below it
	const Element* FindElement(const std::string& theName, Node* theGroup);   // 0x6b31b0

	DataDrivenClass(const DataDrivenClass&);
	DataDrivenClass& operator=(const DataDrivenClass&);
};

}  // namespace JeffLib
