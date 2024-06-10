#pragma once

#include "std.h"

template<typename TELEM>
class TListNode
{
public:
	TListNode():
		Next(nullptr),
		Prev(nullptr),
		Element{}
	{

	}

	TListNode(const TELEM& Elem):
		Element(Elem),
		Next(nullptr),
		Prev(nullptr)
	{
	}

	TListNode(TELEM&& Elem) :
		Element(move(Elem)),
		Next(nullptr),
		Prev(nullptr)
	{
	}

	TListNode& operator=(const TListNode& Src)
	{
		if (&Src != this)
		{
			Element = Src.Element;
		}

		return *this;
	}

	TListNode& operator=(TListNode&& Src)
	{
		if (&Src != this)
		{
			Element = move(Src.Element);
		}

		return *this;
	}

	void LinkAfter(TListNode* Node)
	{
		if (Node)
		{
			Next = Node->Next;
			Prev = Node;

			if (Node->Next)
				Node->Next->Prev = this;

			Node->Next = this;
		}
	}

	void LinkBefore(TListNode* Node)
	{
		if (Node)
		{
			Next = Node;
			Prev = Node->Prev;

			if (Node->Prev)
				Node->Prev->Next = this;

			Node->Prev = this;
		}
	}

	void Unlink()
	{
		if (Prev)
			Prev->Next = Next;

		if (Next)
			Next->Prev = Prev;

		Next = nullptr;
		Prev = nullptr;
	}

	void LinkReplace(TListNode* Node)
	{
		if (Node)
		{
			Next = Node->Next;
			Prev = Node->Prev;

			Next = nullptr;
			Prev = nullptr;
		}
	}

	void Reverse()
	{
		swap(Next, Prev);
	}

	bool IsFirst() const
	{
		return (Next && !Prev);
	}

	bool IsLast() const
	{
		return (!Next && Prev);
	}

	bool IsSingle() const
	{
		return (!Next && !Prev);
	}

	bool IsAdjecent(const TListNode* Node) const
	{
		return (Node &&
			(Next == Node && Node->Prev == this ||
				Prev == Node && Node->Next == this));
	}

	TListNode* GetNext() const
	{
		return Next;
	}

	TListNode* GetPrev() const
	{
		return Prev;
	}

	void SetNext(TListNode* Node)
	{
		Next = Node;
	}

	void SetPrev(TListNode* Node)
	{
		Prev = Node;
	}

	auto* GetElement()
	{
		return &Element;
	}

	const auto* GetElement() const
	{
		return &Element;
	}

	auto* operator->()
	{
		return &Element;
	}

	const auto* operator->() const
	{
		return &Element;
	}

	auto& operator*()
	{
		return Element;
	}

	const auto& operator*() const
	{
		return Element;
	}

	~TListNode()
	{

	}

private:
	TELEM Element;
	TListNode* Next;
	TListNode* Prev;
};

template<typename TNODE>
class TListBase
{
public:
	using TNode = TNODE; // FList::TListNode<TELEM>;

	class TIterator 
	{
	public:

		TIterator() :
			Node(nullptr),
			First(nullptr),
			Last(nullptr)
		{
		}

		TIterator(TNode* First, TNode* Last, TNode* Node) :
			Node(Node),
			First(First),
			Last(Last)
		{
		}

		TIterator(const TIterator& Src) :
			Node(Src.Node),
			First(Src.First),
			Last(Src.Last)
		{
		}

		TIterator(TIterator&& Src) :
			Node(Src.Node),
			First(Src.First),
			Last(Src.Last)
		{
			Src = nullptr;
		}

		TIterator(nullptr_t) :
			Node(nullptr),
			First(nullptr),
			Last(nullptr)
		{

		}

		TIterator& operator=(const TIterator& Src)
		{
			if (&Src != this)
			{
				Node = Src.Node;
				First = Src.First;
				Last = Src.Last;
			}
			return *this;
		}

		TIterator& operator=(TIterator&& Src)
		{
			if (&Src != this)
			{
				Node = Src.Node;
				First = Src.First;
				Last = Src.Last;
				Src = nullptr;
			}
			return *this;
		}

		TIterator& operator=(nullptr_t Src)
		{
			Node = nullptr;
			First = nullptr;
			Last = nullptr;
			return *this;
		}

		TIterator operator++()
		{
			Node = Node->GetNext();
			return *this;
		}

		TIterator operator++(int)
		{
			TIterator Copy = *this;
			++*this;
			return Copy;
		}

		TIterator operator+(uint32_t Distance)
		{
			auto Tmp = *this;
			while (*this && Distance)
			{
				++*this;
				--Distance;
			}
			return Tmp;
		}

		TIterator operator-(uint32_t Distance)
		{
			auto Tmp = *this;
			while (*this && Distance)
			{
				--*this;
				--Distance;
			}
			return Tmp;
		}

		TIterator operator--()
		{
			Node = Node->GetPrev();
			return *this;
		}

		TIterator operator--(int)
		{
			TIterator Copy = *this;
			--*this;
			return Copy;
		}

		void Reset()
		{
			Node = First;
		}

		TNode* GetNode()
		{
			return Node;
		}

		auto& operator*()
		{
			return **Node;
		}

		const auto& operator*() const
		{
			return **Node;
		}

		auto* operator->()
		{
			return &(**Node);
		}

		const auto* operator->() const
		{
			return &(**Node);
		}

		bool operator==(const TIterator& IterB)
		{
			return (Node == IterB.Node);
		}

		bool operator!=(const TIterator& IterB)
		{
			return !(*this == IterB);
		}

		explicit operator bool() const
		{
			return Node != nullptr;
		}

	private:
		TNode* Node;
		TNode* First;
		TNode* Last;
	};

	using TIter = TIterator;

	TListBase() :
		FirstNode(nullptr),
		LastNode(nullptr)
	{

	}
	
	TListBase(TNode* First, TNode* Last) :
		FirstNode(First),
		LastNode(Last)
	{

	}

	void PushBack(TNode* Node)
	{
		InsertAfter(LastNode, Node);
	}

	TNode* PopBack()
	{
		TNode* Last = LastNode;
		Delete(Last);

		return Last;
	}

	void PushFront(TNode* Node)
	{
		InsertBefore(FirstNode, Node);
	}

	TNode* PopFront()
	{
		TNode* Front = FirstNode;
		Delete(Front);

		return Front;
	}

	void SwapLists(TListBase& Src)
	{
		if (&Src != this)
		{
			swap(FirstNode, Src.FirstNode);
			swap(LastNode, Src.LastNode);
		}
	}

	void Reverse()
	{
		TNode* Next = nullptr;
		TNode* Pos = FirstNode;

		while(Pos)
		{
			Next = Pos->GetNext();
			Pos->Reverse();
			Pos = Next;
		}
	
		swap(FirstNode, LastNode);
	}

	bool IsEmpty() const
	{
		return !FirstNode;
	}

	void InsertListAfter(TNode* Pos, TNode* First, TNode* Last)
	{
		if (!FirstNode)
		{
			FirstNode = First;
			LastNode = Last;
		}
		else
		{
			if (First && Last)
			{
				TNode* PosNext = Pos->GetNext();

				First->SetPrev(Pos);
				Last->SetNext(PosNext);

				if(PosNext)
					Pos->GetNext()->SetPrev(Last);

				Pos->SetNext(First);
			}
		}
	}

	void InsertListBefore(TNode* Pos, TNode* First, TNode* Last)
	{
		if (!FirstNode)
		{
			FirstNode = First;
			LastNode = Last;
		}
		else
		{
			if (First && Last)
			{
				TNode* PosPrev = Pos->GetPrev();

				Last->SetNext(Pos);
				First->SetPrev(PosPrev);

				if (PosPrev)
					Pos->GetPrev()->SetNext(First);

				Pos->SetPrev(Last);
			}
		}
	}

	void InsertAfter(TNode* Pos, TNode* Node)
	{
		if (!FirstNode)
		{
			FirstNode = Node;
			LastNode = Node;
		}
		else
		{
			if (Node)
				Node->LinkAfter(Pos);

			if (Pos == LastNode)
				LastNode = Node;
		}
	}

	void InsertBefore(TNode* Pos, TNode* Node)
	{
		if (!FirstNode)
		{
			FirstNode = Node;
			LastNode = Node;
		}
		else
		{
			if (Node)
				Node->LinkBefore(Pos);

			if (Pos == FirstNode)
				FirstNode = Node;
		}
	}

	// relocate Elem to after Pos;
	void RelocateAfter(TIterator Pos, TIterator Elem)
	{
		TNode* PosNode = Pos.GetNode();
		TNode* ElemNode = Elem.GetNode();
		Delete(ElemNode);
		InsertAfter(PosNode, ElemNode);
	}

	void RelocateBefore(TIterator Pos, TIterator Elem)
	{
		TNode* PosNode = Pos.GetNode();
		TNode* ElemNode = Elem.GetNode();
		Delete(ElemNode);
		InsertBefore(PosNode, ElemNode);
	}

	void Delete(TNode* Node)
	{
		if (Node)
		{
			TNode* Next = Node->GetNext();
			TNode* Prev = Node->GetPrev();

			if (FirstNode == Node)
				FirstNode = Next;

			if (LastNode == Node)
				LastNode = Prev;

			Node->Unlink();
		}
	}

	void Delete()
	{
		FirstNode = nullptr;
		LastNode = nullptr;
	}

	TNode* GetLastNode(TNode* First, uint32_t Count)
	{
		if (First)
		{
			while (Count)
			{
				First = First->GetNext();
				--Count;
			}
		}

		return First;
	}

	TNode* GetFirst() const
	{
		return FirstNode;
	}

	TNode* GetLast() const
	{
		return LastNode;
	}

	TNode* GetFirst() 
	{
		return FirstNode;
	}

	TNode* GetLast() 
	{
		return LastNode;
	}

	TIter GetFirstIt() 
	{
		return TIter(FirstNode, LastNode, FirstNode);
	}

	TIter GetLastIt() 
	{
		return TIter(FirstNode, LastNode, LastNode);
	}


	TIter GetBegin()
	{
		return TIter(FirstNode, LastNode, FirstNode);
	}

	TIter GetEnd()
	{
		if(LastNode)
			return TIter(FirstNode, LastNode, LastNode->GetNext());
		return nullptr;
	}

	void SetFirst(TNode* First)
	{
		FirstNode = First;
	}

	void SetLast(TNode* Last)
	{
		LastNode = Last;
	}

private:
	TNode* FirstNode;
	TNode* LastNode;
};