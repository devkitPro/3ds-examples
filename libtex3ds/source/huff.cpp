/*------------------------------------------------------------------------------
 * Copyright (c) 2017-2022
 *     Michael Theall (mtheall)
 *
 * This file is part of tex3ds.
 *
 * tex3ds is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tex3ds is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tex3ds.  If not, see <http://www.gnu.org/licenses/>.
 *----------------------------------------------------------------------------*/
/** @file huff.c
 *  @brief Huffman compression routines
 */

#include "compress.h"
#include "future.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>

namespace Tex3DS
{

namespace
{
/** @brief Huffman node */
class Node
{
public:
	/** @brief Parameterized constructor
	 *  @param val   Node value
	 *  @count count Node count
	 */
	Node (uint8_t val, size_t count) : count (count), val (val)
	{
	}

	/** @brief Parameterized constructor
	 *  @param left  Left child
	 *  @count right Right child
	 */
	Node (std::unique_ptr<Node> left, std::unique_ptr<Node> right)
	    : child{std::move (left), std::move (right)}, count (child[0]->count + child[1]->count)
	{
		// set children's parent to self
		child[0]->parent = this;
		child[1]->parent = this;
	}

	Node () = delete;

	Node (const Node &other) = delete;

	Node (Node &&other) = delete;

	Node &operator= (const Node &other) = delete;

	Node &operator= (Node &&other) = delete;

	/** @brief Comparison operator
	 *  @param other Object to compare
	 */
	bool operator< (const Node &other) const
	{
		// major key is count
		if (count != other.count)
			return count < other.count;

		// minor key is value
		return val < other.val;
	}

	/** @brief Whether this node is a parent */
	bool isParent () const
	{
		return static_cast<bool> (child[0]);
	}

	/** @brief Build Huffman codes
	 *  @param[in] node    Huffman node
	 *  @param[in] code    Huffman code
	 *  @param[in] codeLen Huffman code length (bits)
	 */
	static void buildCodes (std::unique_ptr<Node> &node, uint32_t code, size_t codeLen);

	/** @brief Build lookup table
	 *  @param[in] nodes Table to fill
	 *  @param[in] n     Huffman node
	 */
	static void buildLookup (std::vector<Node *> &nodes, const std::unique_ptr<Node> &node);

	/** @brief Serialize Huffman tree
	 *  @param[out] tree Serialized tree
	 *  @param[in]  node Root of subtree
	 *  @param[in]  next Next available slot in tree
	 */
	static void serializeTree (std::vector<Node *> &tree, Node *node, unsigned next);

	/** @brief Fixup serialized Huffman tree
	 *  @param[inout] tree Serialized tree
	 */
	static void fixupTree (std::vector<Node *> &tree);

	/** @brief Encode Huffman tree
	 *  @param[out] tree Huffman tree
	 *  @param[in]  node Huffman node
	 */
	static void encodeTree (std::vector<uint8_t> &tree, Node *node);

	/** @brief Count number of nodes in subtree
	 *  @returns Number of nodes in subtree
	 */
	size_t numNodes () const
	{
		// sum of children plus self
		if (isParent ())
			return child[0]->numNodes () + child[1]->numNodes () + 1;

		// this is a data node, just count self
		return 1;
	}

	/** @brief Count number of leaves in subtree
	 *  @returns Number of leaves in subtree
	 */
	size_t numLeaves ()
	{
		if (leaves == 0)
		{
			if (isParent ())
			{
				// sum of children
				leaves = child[0]->numLeaves () + child[1]->numLeaves ();
			}
			else
			{
				// this is a data node; it is a leaf
				leaves = 1;
			}
		}

		return leaves;
	}

	/** @brief Get code */
	uint32_t getCode () const
	{
		assert (!isParent ());
		return code;
	}

	/** @brief Get code length */
	uint8_t getCodeLen () const
	{
		assert (!isParent ());
		return codeLen;
	}

private:
	Node *parent;                   ///< Parent node
	std::unique_ptr<Node> child[2]; ///< Children nodes
	size_t count    = 0;            ///< Node weight
	uint32_t code   = 0;            ///< Huffman encoding
	unsigned leaves = 0;            ///< Number of leaves
	uint8_t val     = 0;            ///< Huffman tree value
	uint8_t codeLen = 0;            ///< Huffman code length (bits)
#ifndef NDEBUG
	uint16_t pos = 0; ///< Huffman tree position
#endif
};

void Node::buildCodes (std::unique_ptr<Node> &node, uint32_t code, size_t codeLen)
{
	// don't exceed 32-bit codes
	assert (codeLen < 32);

	if (node->isParent ())
	{
		// build codes for each subtree
		assert (node->child[0] && node->child[1]);
		buildCodes (node->child[0], (code << 1) | 0, codeLen + 1);
		buildCodes (node->child[1], (code << 1) | 1, codeLen + 1);
	}
	else
	{
		// set code for data node
		assert (!node->child[0] && !node->child[1]);
		node->code    = code;
		node->codeLen = codeLen;
	}
}

void Node::buildLookup (std::vector<Node *> &nodes, const std::unique_ptr<Node> &node)
{
	if (!node->isParent ())
	{
		// set lookup entry
		nodes[node->val] = node.get ();
		return;
	}

	// build subtree lookups
	buildLookup (nodes, node->child[0]);
	buildLookup (nodes, node->child[1]);
}

void Node::serializeTree (std::vector<Node *> &tree, Node *node, unsigned next)
{
	assert (node->isParent ());

	if (node->numLeaves () > 0x40)
	{
		// this subtree will overflow the offset field if inserted naively
		tree[next + 0] = node->child[0].get ();
		tree[next + 1] = node->child[1].get ();

		unsigned a = 0;
		unsigned b = 1;

		if (node->child[1]->numLeaves () < node->child[0]->numLeaves ())
			std::swap (a, b);

		if (node->child[a]->isParent ())
		{
			node->child[a]->val = 0;
			serializeTree (tree, node->child[a].get (), next + 2);
		}

		if (node->child[b]->isParent ())
		{
			node->child[b]->val = node->child[a]->numLeaves () - 1;
			serializeTree (tree, node->child[b].get (), next + 2 * node->child[a]->numLeaves ());
		}

		return;
	}

	std::deque<Node *> queue;

	queue.emplace_back (node->child[0].get ());
	queue.emplace_back (node->child[1].get ());

	while (!queue.empty ())
	{
		node = queue.front ();
		queue.pop_front ();

		tree[next++] = node;

		if (!node->isParent ())
			continue;

		node->val = queue.size () / 2;

		queue.emplace_back (node->child[0].get ());
		queue.emplace_back (node->child[1].get ());
	}
}

void Node::fixupTree (std::vector<Node *> &tree)
{
	for (unsigned i = 1; i < tree.size (); ++i)
	{
		if (!tree[i]->isParent () || tree[i]->val <= 0x3F)
			continue;

		unsigned shift = tree[i]->val - 0x3F;

		if ((i & 1) && tree[i - 1]->val == 0x3F)
		{
			// right child, and left sibling would overflow if we shifted;
			// shift the left child by 1 instead
			--i;
			shift = 1;
		}

		unsigned nodeEnd   = i / 2 + 1 + tree[i]->val;
		unsigned nodeBegin = nodeEnd - shift;

		unsigned shiftBegin = 2 * nodeBegin;
		unsigned shiftEnd   = 2 * nodeEnd;

		// move last child pair to front
		auto tmp = std::make_pair (tree[shiftEnd], tree[shiftEnd + 1]);
		std::memmove (
		    &tree[shiftBegin + 2], &tree[shiftBegin], sizeof (Node *) * (shiftEnd - shiftBegin));
		std::tie (tree[shiftBegin], tree[shiftBegin + 1]) = tmp;

		// adjust offsets
		tree[i]->val -= shift;
		for (unsigned index = i + 1; index < shiftBegin; ++index)
		{
			if (!tree[index]->isParent ())
				continue;

			unsigned node = index / 2 + 1 + tree[index]->val;
			if (node >= nodeBegin && node < nodeEnd)
				++tree[index]->val;
		}

		if (tree[shiftBegin + 0]->isParent ())
			tree[shiftBegin + 0]->val += shift;
		if (tree[shiftBegin + 1]->isParent ())
			tree[shiftBegin + 1]->val += shift;

		for (unsigned index = shiftBegin + 2; index < shiftEnd + 2; ++index)
		{
			if (!tree[index]->isParent ())
				continue;

			unsigned node = index / 2 + 1 + tree[index]->val;
			if (node > nodeEnd)
				--tree[index]->val;
		}
	}
}

void Node::encodeTree (std::vector<uint8_t> &tree, Node *node)
{
	std::vector<Node *> nodeTree (tree.size ());
	nodeTree[1] = node;
	serializeTree (nodeTree, node, 2);
	fixupTree (nodeTree);

#ifndef NDEBUG
	for (unsigned i = 1; i < nodeTree.size (); ++i)
	{
		assert (nodeTree[i]);
		nodeTree[i]->pos = i;
	}

	for (unsigned i = 1; i < nodeTree.size (); ++i)
	{
		node = nodeTree[i];
		if (!node->isParent ())
			continue;

		assert (!(node->val & 0x80));
		assert (!(node->val & 0x40));
		assert (node->child[0]->pos == (node->pos & ~1) + 2 * node->val + 2);
	}
#endif

	for (unsigned i = 1; i < nodeTree.size (); ++i)
	{
		node = nodeTree[i];

		tree[i] = node->val;

		if (!node->isParent ())
			continue;

		if (!node->child[0]->isParent ())
			tree[i] |= 0x80;
		if (!node->child[1]->isParent ())
			tree[i] |= 0x40;
	}
}

/** @brief Build Huffman tree
 *  @param[in] src Source data
 *  @param[in] len Source data length
 *  @returns Root node
 */
std::unique_ptr<Node> buildTree (const uint8_t *src, size_t len)
{
	// fill in histogram
	std::vector<size_t> histogram (256);
	for (size_t i = 0; i < len; ++i)
		++histogram[src[i]];

	std::vector<std::unique_ptr<Node>> nodes;
	{
		uint8_t val = 0;
		for (const auto &count : histogram)
		{
			if (count > 0)
				nodes.push_back (future::make_unique<Node> (val, count));

			++val;
		}
	}

	// done with histogram
	histogram.clear ();

	// combine nodes
	while (nodes.size () > 1)
	{
		// sort nodes by count; we will combine the two smallest nodes
		std::sort (std::begin (nodes),
		    std::end (nodes),
		    [] (const std::unique_ptr<Node> &lhs, const std::unique_ptr<Node> &rhs) -> bool {
			    return *lhs < *rhs;
		    });

		// allocate a parent node
		std::unique_ptr<Node> node =
		    future::make_unique<Node> (std::move (nodes[0]), std::move (nodes[1]));

		// replace first node with self
		nodes[0] = std::move (node);

		// replace second node with last node
		nodes[1] = std::move (nodes.back ());
		nodes.pop_back ();
	}

	// root is the last node left
	std::unique_ptr<Node> root = std::move (nodes[0]);

	// root must have children
	if (!root->isParent ())
		root = future::make_unique<Node> (std::move (root), future::make_unique<Node> (0x00, 0));

	// build Huffman codes
	Node::buildCodes (root, 0, 0);

	// return root node
	return root;
}

/** @brief Bitstream */
class Bitstream
{
public:
	Bitstream (std::vector<uint8_t> &buffer) : buffer (buffer)
	{
	}

	/** @brief Flush bitstream block, padded to 32 bits */
	void flush ()
	{
		if (pos >= 32)
			return;

		// append bitstream block to output buffer
		buffer.reserve (buffer.size () + 4);
		buffer.emplace_back (code >> 0);
		buffer.emplace_back (code >> 8);
		buffer.emplace_back (code >> 16);
		buffer.emplace_back (code >> 24);

		// reset bitstream block
		pos  = 32;
		code = 0;
	}

	/** @brief Push Huffman code onto bitstream
	 *  @param[in] code Huffman code
	 *  @param[in] len  Huffman code length (bits)
	 */
	void push (uint32_t code, size_t len)
	{
		for (size_t i = 1; i <= len; ++i)
		{
			// get next bit position
			--pos;

			// set/reset bit
			if (code & (1U << (len - i)))
				this->code |= (1U << pos);
			else
				this->code &= ~(1U << pos);

			// flush bitstream block
			if (pos == 0)
				flush ();
		}
	}

private:
	std::vector<uint8_t> &buffer; ///< Output buffer
	size_t pos    = 32;           ///< Bit position
	uint32_t code = 0;            ///< Bitstream block
};
}

std::vector<uint8_t> huffEncode (const void *source, size_t len)
{
	const uint8_t *src = (const uint8_t *)source;
	size_t count;

	// build Huffman tree
	std::unique_ptr<Node> root = buildTree (src, len);

	// build lookup table
	std::vector<Node *> lookup (256);
	Node::buildLookup (lookup, root);

	// get number of nodes
	count = root->numNodes ();

	// allocate Huffman encoded tree
	std::vector<uint8_t> tree ((count + 2) & ~1);

	// first slot encodes tree size
	tree[0] = count / 2;

	// encode Huffman tree
	Node::encodeTree (tree, root.get ());

	// create output buffer
	std::vector<uint8_t> result;
	result.reserve (len); // hopefully our output will be smaller

	// append compression header
	compressionHeader (result, 0x28, len);

	// append Huffman encoded tree
	result.insert (std::end (result), std::begin (tree), std::end (tree));

	// we're done with the Huffman encoded tree
	tree.clear ();

	// create bitstream
	Bitstream bitstream (result);

	// encode each input byte
	for (size_t i = 0; i < len; ++i)
	{
		// lookup the byte value's node
		Node *node = lookup[src[i]];

		// add Huffman code to bitstream
		bitstream.push (node->getCode (), node->getCodeLen ());
	}

	// we're done with the Huffman tree and lookup table
	root.reset ();
	lookup.clear ();

	// flush the bitstream
	bitstream.flush ();

	// pad the output buffer to 4 bytes
	if (result.size () & 0x3)
		result.resize ((result.size () + 3) & ~0x3);

	// return the output data
	return result;
}

void huffDecode (const void *src, void *dst, size_t size)
{
	const size_t bits   = 8;
	const uint8_t *in   = (const uint8_t *)src;
	uint8_t *out        = (uint8_t *)dst;
	uint32_t treeSize   = ((*in) + 1) * 2; // size of the huffman header
	uint32_t word       = 0;               // 32-bits of input bitstream
	uint32_t mask       = 0;               // which bit we are reading
	uint8_t dataMask    = (1 << bits) - 1; // mask to apply to data
	const uint8_t *tree = in;              // huffman tree
	size_t node;                           // node in the huffman tree
	size_t child;                          // child of a node
	uint32_t offset;                       // offset from node to child

	// point to the root of the huffman tree
	node = 1;

	// move input pointer to beginning of bitstream
	in += treeSize;

	while (size > 0)
	{
		if (mask == 0) // we exhausted 32 bits
		{
			// reset the mask
			mask = 0x80000000;

			// read the next 32 bits
			word = (in[0] << 0) | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
			in += 4;
		}

		// read the current node's offset value
		offset = tree[node] & 0x3F;

		child = (node & ~1) + offset * 2 + 2;

		if (word & mask) // we read a 1
		{
			// point to the "right" child
			++child;

			if (tree[node] & 0x40) // "right" child is a data node
			{
				// copy the child node into the output buffer and apply mask
				*out++ = tree[child] & dataMask;
				size--;

				// start over at the root node
				node = 1;
			}
			else // traverse to the "right" child
				node = child;
		}
		else // we read a 0
		{
			// pointed to the "left" child

			if (tree[node] & 0x80) // "left" child is a data node
			{
				// copy the child node into the output buffer and apply mask
				*out++ = tree[child] & dataMask;
				size--;

				// start over at the root node
				node = 1;
			}
			else // traverse to the "left" child
				node = child;
		}

		// shift to read next bit (read bit 31 to bit 0)
		mask >>= 1;
	}
}

} // namespace Tex3DS
