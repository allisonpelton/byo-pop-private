#ifndef __WORDHARMONY_DICTIONARY_H__
#define __WORDHARMONY_DICTIONARY_H__

// The game's two word lists (0x41f8d0-0x421190). config\hintList.txt (words of 4 letters or more) goes into the
// anagram tree (the controller's +0x10), which finds the words a set of letters and chained pieces can make;
// config\matchList.txt (3 letters or more) goes into the word tree (+0x14), which checks a word and finds the words a
// pattern with '?' matches. Both trees are trees of letters: a node's sibling holds another letter at its place, its
// child the next letter, and a '\0' node ends a word. The original takes the nodes from pools of 1024 (0x41fed0,
// 0x420d70) and never frees them; so does the port (NodePool, below).

#include "Object.h"
#include <cstddef>
#include <new>
#include <string>
#include <vector>

namespace engine
{
class InputStream;
}

// The nodes' memory, the original's pools of 1024 nodes that are never freed (0x41fed0, 0x420d70; port, for speed: the
// two lists are some 440,000 nodes, which as one allocation each cost a header and padding apiece, 7 MB, and the time of the
// calls). A node's class takes its memory from the pool of its own size with its operator new; delete frees nothing.
template <class Node>
struct NodePool
{
	static void* allocate(size_t theSize)
	{
		if (sLeft == 0)
		{
			sNext = static_cast<char*>(::operator new(theSize * 1024));
			sLeft = 1024;
		}
		void* aNode = sNext;
		sNext += theSize;
		sLeft--;
		return aNode;
	}
	static char* sNext;
	static size_t sLeft;
};
template <class Node> char* NodePool<Node>::sNext = nullptr;
template <class Node> size_t NodePool<Node>::sLeft = 0;

// The letters to make words of (0x41fcd0, made on the stack): the count of each letter, the wildcards, the total and
// the chained pieces, each of which must be used whole
struct LetterSet
{
	LetterSet(const std::string& theLetters);  // 0x41fcd0

	// 0x41f8f0: the piece starting with theLetter, its index into theIndex
	bool findPiece(char theLetter, unsigned int& theIndex);
	// 0x41fd70: piece theIndex taken out
	void removePiece(unsigned int theIndex);
	// 0x41ff40: a piece put (back) in
	void addPiece(std::string thePiece);

	signed char mCounts[26];         // +0x00
	signed char mWildcards;          // +0x1a: the '?'s
	signed char mTotal;              // +0x1b: the letters left, the pieces' included
	std::vector<std::string> mPieces;  // +0x1c
};

// A node of the anagram tree (0x24 bytes, vtable 0x482ba8)
class AnagramNode
{
public:
	AnagramNode(char theLetter);
	virtual ~AnagramNode();  // 0x41fdd0: its sibling and child deleted
	static void* operator new(size_t theSize) { return NodePool<AnagramNode>::allocate(theSize); }
	static void operator delete(void*) {}

	// 0x41ffb0: theWord added under the letters of theKey; returns the node's mDepth
	int add(const char* theKey, const char* theWord);
	// 0x420100: words that use up all of theLetters (each found word starts again at theRoot while more than 3
	// letters are left) into theWords; *theDone when they do. theChain: the rest of a piece being followed
	void findAllLetters(LetterSet& theLetters, bool* theDone, std::vector<std::string>* theWords, AnagramNode* theRoot,
	                    const char* theChain);
	// 0x420390: every word of 4 to 6 letters theLetters can make, into theWords
	void findWords(LetterSet& theLetters, std::vector<std::string>* theWords, const char* theChain);

	char mLetter;                      // +0x04
	std::vector<std::string> mWords;   // +0x08: on a '\0' node, the words that end here
	AnagramNode* mSibling;             // +0x18
	AnagramNode* mChild;               // +0x1c
	signed char mDepth;                // +0x20: one more than the sibling's
};

// The anagram tree (the controller's +0x10)
class Dictionary
{
public:
	// 0x41f8d0
	Dictionary() : mRoot(nullptr) {}

	// 0x4208d0
	void add(const char* theWord);
	// 0x4205b0: whether theLetters and the pieces theChains can be used up entirely by words (into theWords; the hints)
	bool findWords(const std::string& theLetters, const std::vector<std::string>& theChains,
	               std::vector<std::string>* theWords);
	// 0x420720: the words of 4 to 6 letters they can make, added to theWords; false when there are none
	bool findAnyWords(const std::string& theLetters, const std::vector<std::string>& theChains,
	                  std::vector<std::string>& theWords);

	AnagramNode* mRoot;  // +0x00
};

// A node of the word tree (0x10 bytes, vtable 0x482bec)
class WordNode
{
public:
	WordNode(char theLetter) : mLetter(theLetter), mSibling(nullptr), mChild(nullptr) {}
	virtual ~WordNode();  // 0x420d40
	static void* operator new(size_t theSize) { return NodePool<WordNode>::allocate(theSize); }
	static void operator delete(void*) {}

	// 0x420dd0
	void add(const char* theWord);
	// 0x420b70: *theStatus 0 when theWord is in the tree as it is; 1 when only its first letter's case differs, 2
	// when its case differs elsewhere, 3 when it is not there and 4 when it runs past a word or stops inside one.
	// theExact rules out other cases; *theLast gets the last node looked at
	void find(const char* theWord, bool theExact, int* theStatus, bool theFirst, WordNode** theLast);
	// 0x420ec0: the words thePattern ('?' for any letter, case ignored) matches, each thePrefix and the tree's letters
	void match(const char* thePattern, std::vector<std::string>* theWords, std::string thePrefix);

	char mLetter;         // +0x04
	WordNode* mSibling;   // +0x08
	WordNode* mChild;     // +0x0c
};

// The word tree (the controller's +0x14, vtable 0x482be8)
class WordTree
{
public:
	// 0x420b40
	WordTree() : mRoot(nullptr) {}
	virtual ~WordTree();  // 0x420b50

	// 0x420e60
	void add(const char* theWord);
	// 0x420d10: the status of WordNode::find, 3 with no words loaded
	int checkWord(const char* theWord, bool theExact);
	// 0x420fe0: the words thePattern matches, into theWords (emptied first); how many
	int findMatches(const char* thePattern, std::vector<std::string>* theWords);

	WordNode* mRoot;  // +0x04
};

// Reads a stream's lines (0x418 bytes, constructor 0x4209c0)
class LineReader
{
public:
	// 0x4209c0
	LineReader(engine::InputStream* theStream) : mPos(0), mCount(0), mStream(theStream) {}

	// 0x420a80: the next line that is not empty into theLine; its length, 0 at the end
	int readLine(std::string& theLine);

private:
	char mBuffer[0x400];           // +0x0c
	unsigned int mPos;             // +0x40c
	unsigned int mCount;           // +0x410
	engine::InputStream* mStream;  // +0x414
};

#endif
