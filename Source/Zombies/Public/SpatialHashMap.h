//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
//#include "SpatialHashMap.generated.h"


//Base data for spatial hash maps
template <typename InGeometryType, typename InValueType>
struct TSpatialHashMapBase
{
	using GeometryType = InGeometryType;
	using ValueType = InValueType;
	using CellType = TArray<int32>;

	struct FElement
	{
		GeometryType Geometry{ ForceInit };
		ValueType Value;
	};

protected:

	FVector2D Origin;
	FVector2D CellSize;
	FVector2D InvCellSize;


	//GetTypeHash(FIntPoint) simply sums the elements together, which forces all diagonals into the same bucket which is bad for performance.
	struct FKeyFuncs : public TDefaultMapHashableKeyFuncs<FIntPoint, CellType, false>
	{
		static uint32 GetKeyHash(FIntPoint Key)
		{
			return HashCombine(*reinterpret_cast<uint32*>(&Key.X), *reinterpret_cast<uint32*>(&Key.Y));
		}
	};

	TMap<FIntPoint, CellType, FDefaultSetAllocator, FKeyFuncs> Cells;

	TSparseArray<FElement> Elements;

public:

	TSpatialHashMapBase()
	{
		SetTransform(FVector2D::Zero(), FVector2D{ 500.0, 500.0 });
	}

	const auto& GetCells() const { return Cells; }

	const auto& GetElements() const { return Elements; }

	void SetTransform(FVector2D InOrigin, FVector2D InCellSize)
	{
		//modify transform on non-empty map is not supported
		Empty();

		Origin = InOrigin;
		CellSize = InCellSize;
		InvCellSize = { 1.0 / CellSize.X, 1.0 / CellSize.Y };
	}

	const FVector2D& GetOrigin() const { return Origin; }
	const FVector2D& GetCellSize() const { return CellSize; }
	const FVector2D& GetInvCellSize() const { return InvCellSize; }

	FVector2D WorldToLocal(const FVector2D& Position) const
	{
		return (Position - Origin) * InvCellSize;
	}

	FBox2D WorldToLocal(const FBox2D& Box) const
	{
		return FBox2d{ WorldToLocal(Box.Min), WorldToLocal(Box.Max) };
	}

	FVector2D LocalToWorld(const FVector2D& Position) const
	{
		return Position * CellSize + Origin;
	}

	FBox2D LocalToWorld(const FBox2D& Box) const
	{
		return FBox2D{ LocalToWorld(Box.Min), LocalToWorld(Box.Max) };
	}

	FIntPoint GetCellGeometry(const FVector2D& LocalPosition) const
	{
		return FIntPoint{ FMath::FloorToInt32(LocalPosition.X), FMath::FloorToInt32(LocalPosition.Y) };
	}

	FIntRect GetCellGeometry(const FBox2D& LocalBox) const
	{
		return FIntRect{ GetCellGeometry(LocalBox.Min), GetCellGeometry(LocalBox.Max) + 1 };
	}

	FElement& GetElement(int32 ElementID)
	{
		return Elements[ElementID];
	}

	const FElement& GetElement(int32 ElementID) const
	{
		return Elements[ElementID];
	}

	ValueType& GetValue(int32 ElementID)
	{
		return Elements[ElementID].Value;
	}

	const ValueType& GetValue(int32 ElementID) const
	{
		return Elements[ElementID].Value;
	}

	const FBox2D& GetGeometry(int32 ElementID) const
	{
		return Elements[ElementID].Geometry;
	}

	void Empty()
	{
		Elements.Empty();
		Cells.Empty();
	}
};

//Maps a geometry to the cells that it should appear in
template <typename InGeometryType, typename InValueType>
struct TSpatialHashMapGeometryType : public TSpatialHashMapBase<InGeometryType, InValueType>
{
protected:

	//void AddElementID(const GeometryType& Geometry, int32 ElementID);

	//void RemoveElementID(const GeometryType& Geometry, int32 ElementID, bool bAllowShrinking);

};

template <typename InValueType>
struct TSpatialHashMapGeometryType<FVector2D, InValueType> : public TSpatialHashMapBase<FVector2D, InValueType>
{
	using Super = TSpatialHashMapBase<FVector2D, InValueType>;
	using Super::GetCellGeometry;
	using typename Super::GeometryType;

protected:

	void AddElementID(const GeometryType& Geometry, int32 ElementID)
	{
		InternalAddElementID(GetCellGeometry(Geometry), ElementID);
	}

	void MoveElementID(const GeometryType& OldGeometry, const GeometryType& NewGeometry, int32 ElementID, bool bAllowShrinking)
	{
		auto OldPoint = GetCellGeometry(OldGeometry);
		auto NewPoint = GetCellGeometry(NewGeometry);

		if (OldPoint == NewPoint)
			//Same cell, do not need to do anything
			return;
		
		InternalRemoveElementID(OldPoint, ElementID, bAllowShrinking);
		InternalAddElementID(NewPoint, ElementID);
	}

	void RemoveElementID(const GeometryType& Geometry, int32 ElementID, bool bAllowShrinking)
	{
		InternalRemoveElementID(GetCellGeometry(Geometry), ElementID, bAllowShrinking);
	}

protected:

	void InternalAddElementID(const FIntPoint& Point, int32 ElementID)
	{
		Cells.FindOrAdd(Point).Add(ElementID);
	}

	void InternalRemoveElementID(const FIntPoint& Point, int32 ElementID, bool bAllowShrinking)
	{
		if (auto Cell = Cells.Find(Point))
		{
			Cell->RemoveSingleSwap(ElementID, bAllowShrinking);

			if (Cell->Num() == 0)
				Cells.Remove(Point);
		}
	}


};

template <typename InValueType>
struct TSpatialHashMapGeometryType<FBox2D, InValueType> : public TSpatialHashMapBase<FBox2D, InValueType>
{
	using Super = TSpatialHashMapBase<FBox2D, InValueType>;
	using Super::GetCellGeometry;
	using typename Super::GeometryType;

protected:

	void AddElementID(const GeometryType& Geometry, int32 ElementID)
	{
		InternalAddElementID(GetCellGeometry(Geometry), ElementID);
	}

	void MoveElementID(const GeometryType& OldGeometry, const GeometryType& NewGeometry, int32 ElementID, bool bAllowShrinking)
	{
		auto OldBox = GetCellGeometry(OldGeometry);
		auto NewBox = GetCellGeometry(NewGeometry);

		if (OldBox == NewBox)
			//Could possibly be optimised further by doing a set intersection of covered cells instead of simple equality
			return;

		InternalRemoveElementID(OldBox, ElementID, bAllowShrinking);
		InternalAddElementID(NewBox, ElementID, bAllowShrinking);
	}

	void RemoveElementID(const GeometryType& Geometry, int32 ElementID, bool bAllowShrinking)
	{
		InternalRemoveElementID(GetCellGeometry(Geometry), ElementID, bAllowShrinking);
	}

protected:

	void InternalAddElementID(const FIntRect& Box, int32 ElementID)
	{
		for (int32 Y = Box.Min.Y; Y < Box.Max.Y; ++Y)
			for (int32 X = Box.Min.X; X < Box.Max.X; ++X)
				Cells.FindOrAdd(FIntPoint{ X,Y }).Add(ElementID);
	}

	void InternalRemoveElementID(const FIntRect& Box, int32 ElementID, bool bAllowShrinking)
	{
		for (int32 Y = Box.Min.Y; Y < Box.Max.Y; ++Y)
			for (int32 X = Box.Min.X; X < Box.Max.X; ++X)
				if (auto Cell = Cells.Find(FIntPoint{ X,Y }))
				{
					Cell->RemoveSingleSwap(ElementID, bAllowShrinking);

					if (Cell->Num() == 0)
						Cells.Remove(FIntPoint{ X,Y });
				}
	}
};


//UE style iterator
template <typename MapType>
struct TSpatialHashMapBoxQuery
{
	template <typename T>
	using ChooseConstType = typename TChooseClass<TIsConst<MapType>::Value, const T, T>::Result;

	using IteratorMapType = MapType;

	using IteratorCellType = typename ChooseConstType<typename MapType::CellType>;

	using IteratorElementType = typename ChooseConstType<typename MapType::FElement>;

	using IteratorValueType = typename ChooseConstType<typename MapType::ValueType>;

	TSpatialHashMapBoxQuery(IteratorMapType& InMap, const FBox2D& InQueryBox) : Map(InMap), QueryBox(InQueryBox), QueryCellBox(InMap.GetCellGeometry(InQueryBox))
	{
		//Start one to the left so that Advance() will advance to the first valid element
		CurrentCell.X = QueryCellBox.Min.X - 1;
		CurrentCell.Y = QueryCellBox.Min.Y;

		Advance();
	};

	operator bool() const
	{
		return CurrentCell.X < QueryCellBox.Max.X && CurrentCell.Y < QueryCellBox.Max.Y;
	}

	TSpatialHashMapBoxQuery& operator++()
	{
		Advance();
		return *this;
	}

	IteratorValueType& operator*() const
	{
		check(CurrentElement);
		return CurrentElement->Value;
	}

	IteratorValueType* operator->() const
	{
		check(CurrentElement);
		return &CurrentElement->Value;
	}

	const FBox2D& GetLocalGeometry() const
	{
		check(CurrentElement);
		return CurrentElement->Geometry;
	}

	const FBox2D& GetWorldGeometry() const
	{
		return Map.LocalToWorld(GetLocalGeometry());
	}

protected:

	IteratorMapType& Map;

	FBox2D QueryBox;

	FIntRect QueryCellBox;

	FIntPoint CurrentCell;

	IteratorElementType* CurrentElement = nullptr;

	IteratorCellType* Elements = nullptr;

	int32 ElementIndex = -1;

	//Technically these overloads should be specialized template structs to allow further extension
	bool ShouldVisitElement(const FBox2D& Box) const
	{
		//Since the map is flat, box elements can appear in multiple cells, so we only actually visit it once when we hit the elements minimum cell that is within the query.
		//We are guaranteed to hit the minimum cell of an element first.
		FIntPoint MinCell = Map.GetCellGeometry(Box.Min).ComponentMax(QueryCellBox.Min);

		return CurrentCell == MinCell && Box.Intersect(QueryBox);
	}

	bool ShouldVisitElement(const FVector2D& Point) const
	{
		//Point type can only ever appear in one cell
		return QueryBox.IsInside(Point);
	}

	void Advance()
	{
		while (true)
		{
			//First try advance current element if any
			if (Elements)
			{
				++ElementIndex;
				CurrentElement = nullptr;

				if (ElementIndex < Elements->Num())
				{
					auto ElementID = (*Elements)[ElementIndex];

					auto& Element = Map.GetElement(ElementID);

					if (ShouldVisitElement(Element.Geometry))
					{
						//Found an overlapping element, stop advancing
						CurrentElement = &Element;
						return;
					}
					else
						//Element does not overlap or is already visited, go to the next one in the array
						continue;
				}
			}

			//Moving on to new cell
			Elements = nullptr;
			ElementIndex = -1;

			CurrentCell.X++;

			if (CurrentCell.X >= QueryCellBox.Max.X)
			{
				CurrentCell.Y++;

				if (CurrentCell.Y >= QueryCellBox.Max.Y)
					//Reached end of rect
					return;

				CurrentCell.X = QueryCellBox.Min.X;
			}

			//Initialize element iteration in new current cell.
			Elements = Map.GetCells().Find(CurrentCell);
			ElementIndex = -1;
		}
	}

};



//Spatial hash map that maps a geometry to value and is spatially queryable
//Currently only supports 2D point and box geometries.
template <typename InGeometryType, typename InValueType>
struct TSpatialHashMap : TSpatialHashMapGeometryType<InGeometryType, InValueType>
{
	using Super = TSpatialHashMapGeometryType<InGeometryType, InValueType>;
	using Super::WorldToLocal;
	using Super::LocalToWorld;
	using Super::GetCellGeometry;
	using typename Super::GeometryType;
	using typename Super::ValueType;
	using Super::GetElement;
	using Super::GetValue;
	//using typename Super::FElement;

	template <typename ...ArgTypes>
	int32 AddElementWorldSpace(const GeometryType& WorldGeometry, ArgTypes&& ... Args)
	{
		return AddElementLocalSpace(WorldToLocal(WorldGeometry), Forward<ArgTypes>(Args)...);
	}

	template <typename ...ArgTypes>
	int32 AddElementLocalSpace(const GeometryType& LocalGeometry, ArgTypes&& ... Args)
	{
		int32 SearchIndex = 0;

		auto InsertResult = Elements.EmplaceAtLowestFreeIndex(SearchIndex, FElement{ LocalGeometry, ValueType{ Forward<ArgTypes>(Args)... } });

		AddElementID(LocalGeometry, InsertResult);

		return InsertResult;
	}

	bool MoveElementWorldSpace(int32 ElementID, const GeometryType& NewWorldGeometry)
	{
		return MoveElementLocalSpace(ElementID, WorldToLocal(NewWorldGeometry));
	}

	bool MoveElementLocalSpace(int32 ElementID, const GeometryType& NewLocalGeometry)
	{
		if (!Elements.IsValidIndex(ElementID))
			return false;

		auto& Element = Elements[ElementID];

		if (Element.Geometry == NewLocalGeometry)
			return true;

		MoveElementID(Element.Geometry, NewLocalGeometry, ElementID, true);

		Element.Geometry = NewLocalGeometry;

		return true;
	}

	bool RemoveElement(int32 ElementID)
	{
		if (!Elements.IsValidIndex(ElementID))
			return false;

		RemoveElementID(Elements[ElementID].Geometry, ElementID, true);

		Elements.RemoveAt(ElementID);

		return true;
	}


	using TBoxQuery = TSpatialHashMapBoxQuery<TSpatialHashMap>;

	using TConstBoxQuery = TSpatialHashMapBoxQuery<const TSpatialHashMap>;

	TConstBoxQuery LocalBoxQuery(const FBox2D& LocalBox) const
	{
		return TConstBoxQuery{ *this, LocalBox };
	}

	TBoxQuery LocalBoxQuery(const FBox2D& LocalBox)
	{
		return TBoxQuery{ *this, LocalBox };
	}

	TConstBoxQuery WorldBoxQuery(const FBox2D& WorldBox) const
	{
		return TConstBoxQuery{ *this, WorldToLocal(WorldBox) };
	}

	TBoxQuery WorldBoxQuery(const FBox2D& WorldBox)
	{
		return TBoxQuery{ *this, WorldToLocal(WorldBox) };
	}

};




////Spatial hash map that maps a geometry to value and is spatially queryable
////Currently only supports 2D point and box geometries.
//template <typename GeometryType, typename ValueType>
//struct TSpatialHashMap
//{
//protected:
//
//	FVector2D Origin = FVector2D::Zero();
//	FVector2D CellSize = FVector2D::One();
//	FVector2D InvCellSize = FVector2D::One();
//
//	using CellType = TArray<int32>;
//
//	//GetTypeHash(FIntPoint) simply sums the elements together, which forces all diagonals into the same bucket which is bad for performance.
//	struct FKeyFuncs : public TDefaultMapHashableKeyFuncs<FIntPoint, CellType, false>
//	{
//		static uint32 GetKeyHash(FIntPoint Key)
//		{
//			return HashIntPoint(Key);
//		}
//
//		static uint32 HashIntPoint(FIntPoint Key)
//		{
//			return HashCombine(*reinterpret_cast<uint32*>(&Key.X), *reinterpret_cast<uint32*>(&Key.Y));
//		}
//	};
//
//	TMap<FIntPoint, CellType, FDefaultSetAllocator, FKeyFuncs> Cells;
//
//	void AddElementID(FIntRect Rect, int32 ElementID)
//	{
//		for (int32 Y = Rect.Min.Y; Y < Rect.Max.Y; ++Y)
//			for (int32 X = Rect.Min.X; X < Rect.Max.X; ++X)
//				Cells.FindOrAdd(FIntPoint{ X,Y }).Add(ElementID);
//	}
//
//	void RemoveElementID(FIntRect Rect, int32 ElementID, bool bAllowShrinking)
//	{
//		for (int32 Y = Rect.Min.Y; Y < Rect.Max.Y; ++Y)
//			for (int32 X = Rect.Min.X; X < Rect.Max.X; ++X)
//				if (auto Cell = Cells.Find(FIntPoint{ X,Y }))
//				{
//					Cell->RemoveSingleSwap(ElementID, bAllowShrinking);
//
//					if (Cell->Num() == 0)
//						Cells.Remove(FIntPoint{ X,Y });
//				}
//	}
//
//	struct FElementValue
//	{
//		FBox2D Bounds{ForceInit};
//		ValueType Value;
//	};
//
//	TSparseArray<FElementValue> Elements;
//	
//public:
//
//	const auto& GetCells() const { return Cells; }
//
//	const auto& GetElements() const { return Elements; }
//
//	void SetTransform(FVector2D InOrigin, FVector2D InCellSize)
//	{
//		//modify transform on non-empty map is not supported
//		Empty();
//
//		Origin = InOrigin;
//		CellSize = InCellSize;
//		InvCellSize = { 1.0 / CellSize.X, 1.0 / CellSize.Y };
//	}
//
//	const FVector2D& GetOrigin() const { return Origin; }
//	const FVector2D& GetCellSize() const { return CellSize; }
//	const FVector2D& GetInvCellSize() const { return InvCellSize; }
//
//	FVector2D WorldToLocal(const FVector2D& Position) const
//	{
//		return (Position - Origin) * InvCellSize;
//	}
//
//	FBox2D WorldToLocal(const FBox2D& Box) const
//	{
//		return FBox2d{ WorldToLocal(Box.Min), WorldToLocal(Box.Max) };
//	}
//
//	FVector2D LocalToWorld(const FVector2D& Position) const
//	{
//		return Position * CellSize + Origin;
//	}
//
//	FBox2D LocalToWorld(const FBox2D& Box) const
//	{
//		return FBox2D{ LocalToWorld(Box.Min), LocalToWorld(Box.Max) };
//	}
//
//	FIntPoint GetCellPosition(const FVector2D& LocalPosition) const
//	{
//		return FIntPoint{ FMath::FloorToInt32(LocalPosition.X), FMath::FloorToInt32(LocalPosition.Y) };
//	}
//
//	//Gets the cells that overlap the local space rect
//	FIntRect GetCellBox(const FBox2D& LocalBox) const
//	{
//		return FIntRect{ GetCellPosition(LocalBox.Min), GetCellPosition(LocalBox.Max) + 1 };
//	}
//
//	template <typename ...ArgTypes>
//	int32 AddElementWorldSpace(const FBox2D& WorldBounds, ArgTypes&& ... Args)
//	{
//		return AddElementLocalSpace(WorldToLocal(WorldBounds), Forward<ArgTypes&&>(Args)...);
//	}
//
//	template <typename ...ArgTypes>
//	int32 AddElementLocalSpace(const FBox2D& LocalBounds, ArgTypes&& ... Args)
//	{
//		int32 SearchIndex = 0;
//		auto InsertResult = Elements.EmplaceAtLowestFreeIndex(SearchIndex, FElementValue{ LocalBounds, ElementType{ Forward<ArgTypes&&>(Args)... } });
//
//		AddElementID(GetCellBox(LocalBounds), InsertResult);
//		
//		return InsertResult;
//	}
//
//	bool MoveElementWorldSpace(int32 ElementID, const FBox2D& NewWorldBounds)
//	{
//		return MoveElementLocalSpace(ElementID, WorldToLocal(NewWorldBounds));
//	}
//
//	bool MoveElementLocalSpace(int32 ElementID, const FBox2D& NewLocalBounds)
//	{
//		if (!Elements.IsValidIndex(ElementID))
//			return false;
//
//		auto& Element = Elements[ElementID];
//
//		if (Element.Bounds == NewLocalBounds)
//			return true;
//
//		RemoveElementID(GetCellBox(Element.Bounds), ElementID, true);
//
//		Element.Bounds = NewLocalBounds;
//
//		AddElementID(GetCellBox(Element.Bounds), ElementID);
//
//		return true;
//	}
//
//	bool RemoveElement(int32 ElementID)
//	{
//		if (!Elements.IsValidIndex(ElementID))
//			return false;
//
//		RemoveElementID(GetCellBox(Elements[ElementID].Bounds), ElementID, true);
//
//		Elements.RemoveAt(ElementID);
//
//		return true;
//	}
//
//	ElementType& GetElement(int32 ElementID)
//	{
//		return Elements[ElementID].Value;
//	}
//
//	const ElementType& GetElement(int32 ElementID) const
//	{
//		return Elements[ElementID].Value;
//	}
//
//	const FBox2D& GetElementBounds(int32 ElementID)
//	{
//		return Elements[ElementID].Bounds;
//	}
//
//	void Empty()
//	{
//		Elements.Empty();
//		Cells.Empty();
//	}
//
//	//UE style iterator
//	template <bool bIsConst>
//	struct TBoxIteratorBase
//	{
//		template <typename T>
//		using ChooseConstType = typename TChooseClass<bIsConst, const T, T>::Result;
//
//		using IteratorMapType = typename ChooseConstType<typename TSpatialHashMap>;
//		
//		using IteratorCellType = typename ChooseConstType<typename TSpatialHashMap::CellType>;
//
//		using IteratorElementType = typename ChooseConstType<typename ElementType>;
//
//		TBoxIteratorBase(IteratorMapType& InMap, FBox2D InQueryBox) : Map(InMap), QueryBox(InQueryBox), CellBox(InMap.GetCellBox(InQueryBox))
//		{
//			//Start one to the left so that Advance() will advance to the first valid element
//			CurrentCell.X = CellBox.Min.X - 1;
//			CurrentCell.Y = CellBox.Min.Y;
//
//			Advance();
//		};
//
//		operator bool() const
//		{
//			return CurrentCell.X < CellBox.Max.X && CurrentCell.Y < CellBox.Max.Y;
//		}
//
//		TBoxIteratorBase& operator++()
//		{
//			Advance();
//			return *this;
//		}
//
//		IteratorElementType& operator*() const
//		{
//			check(CurrentElement);
//			return CurrentElement->Value;
//		}
//
//		IteratorElementType* operator->() const
//		{
//			check(CurrentElement);
//			return CurrentElement->Value;
//		}
//
//		const FBox2D& GetLocalBounds() const
//		{
//			check(CurrentElement);
//			return CurrentElement->Bounds;
//		}
//
//		const FBox2D& GetWorldBounds() const
//		{
//			return Map.LocalToWorld(GetLocalBounds());
//		}
//
//	protected:
//
//		IteratorMapType& Map;
//
//		FBox2D QueryBox;
//
//		FIntRect CellBox;
//
//		FIntPoint CurrentCell;
//
//		typename ChooseConstType<FElementValue>* CurrentElement = nullptr;
//
//		IteratorCellType* Elements = nullptr;
//
//		int32 ElementIndex = -1;
//
//		bool ShouldVisitElement(const FBox2D& Box) const
//		{
//			//Since the map is flat, box elements can appear in multiple cells, so we only actually visit it once when we hit the elements minimum cell that is within the query.
//			//We are guaranteed to hit the minimum cell of an element first.
//			FIntPoint MinCell = Map.GetCellPosition(Box.Min).ComponentMax(CellBox.Min);
//
//			return CurrentCell == MinCell && Element.Bounds.Intersect(QueryBox);
//		}
//
//		bool ShouldVisitElement(const FVector2D& Point) const
//		{
//			//Point type can only ever appear in one cell
//			return QueryBox.IsInside(Point);
//		}
//		void Advance()
//		{
//			while (true)
//			{
//				//First try advance current element if any
//				if (Elements)
//				{
//					++ElementIndex;
//					CurrentElement = nullptr;
//
//					if (ElementIndex < Elements->Num())
//					{
//						auto ElementID = (*Elements)[ElementIndex];
//
//						check(Map.Elements.IsValidIndex(ElementID));
//
//						auto& Element = Map.Elements[ElementID];
//
//						//Since the map is flat, elements can appear in multiple cells, so we only actually visit it once when we hit the elements minimum cell that is within the query.
//						//We are guaranteed to hit the minimum cell of an element first.
//						FIntPoint MinCell = Map.GetCellPosition(Element.Bounds.Min).ComponentMax(CellBox.Min);
//
//						if (CurrentCell == MinCell && Element.Bounds.Intersect(Bounds))
//						{
//							//Found an overlapping element, stop advancing
//							CurrentElement = &Element;
//							return;
//						}
//						else
//							//Element does not overlap or is already visited, go to the next one in the array
//							continue;
//					}
//				}
//
//				//Moving on to new cell
//				Elements = nullptr;
//				ElementIndex = -1;
//
//				CurrentCell.X++;
//
//				if (CurrentCell.X >= CellBox.Max.X)
//				{
//					CurrentCell.Y++;
//
//					if (CurrentCell.Y >= CellBox.Max.Y)
//						//Reached end of rect
//						return;
//
//					CurrentCell.X = CellBox.Min.X;
//				}
//
//				//Initialize element iteration in new current cell.
//				Elements = Map.Cells.Find(CurrentCell);
//				ElementIndex = -1;
//			}
//		}
//
//
//	};
//
//	using TBoxIterator = TBoxIteratorBase<false>;
//
//	using TConstBoxIterator = TBoxIteratorBase<true>;
//
//	TConstBoxIterator LocalBoxQuery(const FBox2D &LocalBox) const
//	{
//		return TConstBoxIterator{*this, LocalBox };
//	}
//
//	TBoxIterator LocalBoxQuery(const FBox2D& LocalBox)
//	{
//		return TBoxIterator{*this, LocalBox };
//	}
//
//	TConstBoxIterator WorldBoxQuery(const FBox2D& WorldBox) const
//	{
//		return TConstBoxIterator{*this, WorldToLocal(WorldBox) };
//	}
//
//	TBoxIterator WorldBoxQuery(const FBox2D& WorldBox)
//	{
//		return TBoxIterator{*this, WorldToLocal(WorldBox) };
//	}
//
//};