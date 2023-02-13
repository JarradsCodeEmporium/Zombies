//Copyright Jarrad Alexander 2022


#include "InventoryComponent.h"
#include "Item.h"

PRAGMA_DISABLE_OPTIMIZATION

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	bWantsInitializeComponent = false;
	// ...
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

//
//void UInventoryComponent::InitializeComponent()
//{
//	Super::InitializeComponent();
//
//	InitSlots();
//}
//
//// Called every frame
//void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
//{
//	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
//
//	// ...
//}

bool UInventoryComponent::IsValidContainer(const FInventoryContainerHandle& Container)
{
	if (!Container.IsValid())
		return false;
	
	return Container.Inventory->Containers.Contains(Container.Container);
}

bool UInventoryComponent::IsValidLocation(const FInventoryLocationHandle& Location)
{
	if (!Location.IsValid())
		return false;

	if (!Location.Inventory->Containers.Contains(Location.Container))
		return false;

	//Don't consider rect bounds as part of validity, because that makes resizing containers more difficult

	return true;
}

bool UInventoryComponent::IsContainedWithinItem(const AItem* Item) const
{
	if (!IsValid(Item))
		return false;

	for (auto OwnerItem = GetOwner<AItem>(); OwnerItem; OwnerItem = OwnerItem->GetInventoryLocation().Inventory ? OwnerItem->GetInventoryLocation().Inventory->GetOwner<AItem>() : nullptr)
		if (OwnerItem == Item)
			return true;

	return false;
}

bool UInventoryComponent::IsContainedWithinInventory(const UInventoryComponent* OtherInventory) const
{
	if (!OtherInventory)
		return false;

	return IsContainedWithinItem(OtherInventory->GetOwner<AItem>());
}

TArray<AItem*> UInventoryComponent::GetOverlappingItems(const FInventoryLocationHandle& Location)
{
	TArray<AItem*> Result;

	auto Container = GetContainer(Location);

	if (!Container)
		return Result;
	
	for (auto Item : Container->Items)
		if (IsValid(Item) && Item->GetInventoryLocation().Rect.Intersects(Location.Rect))
			Result.Add(Item);

	return Result;
}

void UInventoryComponent::AddContainerObserver(const FInventoryContainerHandle& Container, FInventoryContainerChangedDelegate Delegate)
{
	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return;

	ContainerPtr->OnChanged.Add(Delegate);
}

void UInventoryComponent::RemoveContainerObserver(const FInventoryContainerHandle& Container, FInventoryContainerChangedDelegate Delegate)
{
	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return;

	ContainerPtr->OnChanged.RemoveSingleSwap(Delegate);
}

FText UInventoryComponent::GetContainerName(const FInventoryContainerHandle& Container)
{
	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return FText::FromString("Null");

	return ContainerPtr->Name;
}

FIntPoint UInventoryComponent::GetContainerSize(const FInventoryContainerHandle& Container)
{
	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return FIntPoint{};

	return ContainerPtr->Size;
}

FGameplayTagQuery UInventoryComponent::GetContainerFilter(const FInventoryContainerHandle& Container)
{
	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return {};

	return ContainerPtr->ItemFilter;
}

int32 UInventoryComponent::GetContainerMaxItems(const FInventoryContainerHandle& Container)
{
	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return -1;

	return ContainerPtr->MaxItems;
}

TArray<AItem*> UInventoryComponent::GetContainerItems(const FInventoryContainerHandle& Container)
{
	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return {};

	return ContainerPtr->Items;
}

bool UInventoryComponent::ItemMatchesContainerFilter(const AItem* Item, const FInventoryContainerHandle& Container)
{
	if (!IsValid(Item))
		return false;

	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return true;

	if (!ContainerPtr->ItemFilter.IsEmpty() && !ContainerPtr->ItemFilter.Matches(Item->ItemTags))
		return false;

	return true;
}

bool UInventoryComponent::CanPlaceItemInContainer(const AItem* Item, const FInventoryContainerHandle& Container)
{
	if (!IsValid(Item))
		return false;

	auto ContainerPtr = UInventoryComponent::GetContainer(Container);

	if (!ContainerPtr)
		//World container always accepts items
		return true;

	if (!Item->CanBePlacedInContainer(Container))
		return false;

	if (!ContainerHasRoomForItem(Item, *ContainerPtr))
		return false;

	if (Container.Inventory->IsContainedWithinItem(Item))
		//Item can't be placed inside itself (would be a cycle in the heirarchy)
		return false;

	return true;
}

bool UInventoryComponent::CanPlaceItemAt(const AItem* Item, const FInventoryLocationHandle& Location)
{
	if (!IsValid(Item))
		return false;

	if (!IsValidLocation(Location))
		return true;

	if (!CanPlaceItemInContainer(Item, Location))
		return false;

	auto Container = UInventoryComponent::GetContainer(Location);

	check(Container);

	if (!ItemRectFitsContainer(Location.Rect, *Container))
		return false;

	if (ItemRectOverlapsOthers(Item, Location.Rect, *Container))
		return false;

	return true;
}

//Visit all positions that the item could be placed with the given rect
//Returns whether all positions were iterated, or if it was exited early
template <typename FuncType>
bool ForEachValidItemRectInContainer(const AItem* Item, const FInventoryContainer& Container, FItemRect Rect, bool bCheckOverlaps, FuncType&& Func)
{
	if (!IsValid(Item))
		return true;

	Rect.Max -= Rect.Min;
	Rect.Min = 0;

	auto Size = Rect.GetRotatedSize();

	for (int32 Y = 0; Y <= Container.Size.Y - Size.Y; ++Y)
		for (int32 X = 0; X <= Container.Size.X - Size.X;)
		{
			FIntPoint Offset{ X,Y };

			auto TestRect = Rect;

			TestRect.Min += Offset;
			TestRect.Max += Offset;

			if (bCheckOverlaps)
			{
				bool bOverlapped = false;

				for (auto OtherItem : Container.Items)
				{
					if (!IsValid(OtherItem) || OtherItem == Item)
						continue;

					auto OtherRect = OtherItem->GetInventoryLocation().Rect;

					if (OtherRect.Intersects(TestRect))
					{
						bOverlapped = true;

						//We know we cannot be valid until at least the end of the overlapping item
						X = OtherRect.Max.X;

						break;
					}
				}

				if (bOverlapped)
					continue;
			}

			if (!Func(TestRect))
				return false;

			++X;
		}

	return true;
}


FInventoryLocationHandle UInventoryComponent::FindLocationForItem(const AItem* Item, const FInventoryContainerHandle& Container)
{
	if (!IsValid(Item))
		return {};

	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return {};

	if (!CanPlaceItemInContainer(Item, Container))
		return {};

	TOptional<FInventoryLocationHandle> Result;

	auto PickFirstValidLocation = [&](const FItemRect& Rect) -> bool
	{
		Result = FInventoryLocationHandle{ Container.Inventory, Container.Container, Rect };
		return false;
	};

	FItemRect Rect{ FIntPoint{0,0}, Item->GetItemSize(), false };

	ForEachValidItemRectInContainer(Item, *ContainerPtr, Rect, true, PickFirstValidLocation);

	if (Result)
		return *Result;

	Rect.Rotate();

	ForEachValidItemRectInContainer(Item, *ContainerPtr, Rect, true, PickFirstValidLocation);

	if (Result)
		return *Result;

	return {};
}

FInventoryLocationHandle UInventoryComponent::FindClosestLocationForItem(const AItem* Item, const FInventoryContainerHandle& Container, FIntPoint QueryLocation, bool bPreferredRotation, bool bAllowRotation, bool bAllowInvalidLocations)
{
	if (!IsValid(Item))
		return {};

	auto ContainerPtr = GetContainer(Container);

	if (!ContainerPtr)
		return {};

	if (!bAllowInvalidLocations && !CanPlaceItemInContainer(Item, Container))
		return {};

	TOptional<double> MinDistSqr;
	TOptional<FItemRect> MinRect;

	auto CompareMinDistSqr = [&](const FItemRect& Rect)
	{
		FVector2D CenterPoint = FVector2D{ Rect.Min } + FVector2D{ Rect.GetRotatedSize() } * 0.5f;

		auto DistSqr = FVector2D::DistSquared(CenterPoint, QueryLocation);

		if (MinDistSqr && *MinDistSqr <= DistSqr)
			return true;

		MinDistSqr = DistSqr;

		MinRect = Rect;

		return true;
	};

	auto FindLocation = [&](bool bCheckOverlaps) -> TOptional<FInventoryLocationHandle>
	{
		auto Rect = FItemRect{ FIntPoint{0}, Item->GetItemSize(), false };

		Rect.SetRotated(bPreferredRotation);

		ForEachValidItemRectInContainer(Item, *ContainerPtr, Rect, bCheckOverlaps, CompareMinDistSqr);

		if (bAllowRotation)
		{
			Rect.Rotate();

			ForEachValidItemRectInContainer(Item, *ContainerPtr, Rect, bCheckOverlaps, CompareMinDistSqr);
		}

		if (MinRect)
			return FInventoryLocationHandle{ Container.Inventory, Container.Container, *MinRect };

		return {};
	};

	if (auto Result = FindLocation(true))
		return *Result;

	if (bAllowInvalidLocations)
		if (auto Result = FindLocation(false))
			return *Result;

	return {};
}

bool UInventoryComponent::ItemRectFitsContainer(const FItemRect& Rect, const FInventoryContainer& Container)
{
	return Rect.Min.X >= 0 && Rect.Min.Y >= 0 && Rect.Max.X <= Container.Size.X && Rect.Max.Y <= Container.Size.Y;
}

bool UInventoryComponent::ItemRectOverlapsOthers(const AItem* Item, const FItemRect& Rect, const FInventoryContainer& Container)
{
	for (auto ContainerItem : Container.Items)
		if (IsValid(ContainerItem) && ContainerItem != Item && ContainerItem->GetInventoryLocation().Rect.Intersects(Rect))
			return true;

	return false;
}

bool UInventoryComponent::ContainerHasRoomForItem(const AItem* Item, const FInventoryContainer& Container)
{
	if (!IsValid(Item))
		return false;

	if (Container.MaxItems > 0 && Container.Items.Num() >= Container.MaxItems && !Container.Items.Contains(Item))
		return false;

	return true;
}


//
////bool UInventoryComponent::FindSlotBP(const AItem* Item, const FInventorySlot& Slot, int32 MoveScope, bool bAllowSelfSlot, FInventorySlot& OutSlot)
////{
////	auto Result = FindSlot(Item, Slot, MoveScope, bAllowSelfSlot);
////
////	if (Result)
////		OutSlot = *Result;
////
////	return Result.IsSet();
////}
//
//TOptional<FInventorySlot> UInventoryComponent::FindSlot(const AItem* Item, const FInventorySlot& Slot, int32 MoveScope, bool bAllowSelfSlot)
//{
//	if (!IsValid(Item))
//		return {};
//
//	if (!Slot.Inventory)
//	{
//		//Slot does not specify an inventory, so can only ever be placed in world (if allowed)
//		if (MoveScope & IMS_World)
//			return FInventorySlot{};
//		else
//			return {};
//	}
//
//	if (Slot.Inventory->IsContainedWithinItem(Item))
//		//Item owns the slots in this inventory, so cannot move inside itself.
//		return {};
//
//	//Fall through to each search type (if allowed)
//
//	if (MoveScope & IMS_Exact)
//	{
//		if (auto SlotPtr = GetSlotPtr(Slot))
//		{
//			auto ExistingItem = SlotPtr->Item;
//
//			if (bAllowSelfSlot && ExistingItem == Item)
//				//Match self
//				return Slot;
//
//			if (!ExistingItem && Item->MatchesSlotType(Slot.Inventory, Slot.SlotTypeTag))
//				//Found an exact match
//				return Slot;
//		}
//		
//	}
//	
//	auto FindSlotInner = [&](FGameplayTag InSlotTypeTag, const FInventorySlotType& InSlotType) -> TOptional<FInventorySlot>
//	{
//		if (!Item->MatchesSlotType(Slot.Inventory, InSlotTypeTag))
//			return {};
//
//		for (int32 i = 0; i < InSlotType.Items.Num(); ++i)
//		{
//			if (InSlotType.Items[i].Item != nullptr && (!bAllowSelfSlot || InSlotType.Items[i].Item != Item))
//				continue;
//
//			FInventorySlot Result;
//
//			Result.Inventory = const_cast<UInventoryComponent*>(Result.Inventory);
//
//			Result.SlotTypeTag = InSlotTypeTag;
//
//			Result.SlotIndex = i;
//
//			return Result;
//		}
//
//		return {};
//	};
//
//	if (MoveScope & IMS_SameSlotType)
//	{
//		if (auto SlotType = Slot.Inventory->Slots.Find(Slot.SlotTypeTag))
//		{
//			auto SameSlotType = FindSlotInner(Slot.SlotTypeTag, *SlotType);
//
//			if (SameSlotType)
//				return SameSlotType;
//			
//		}
//	}
//
//	if (MoveScope & IMS_SameInventory)
//	{
//		for (auto& [SlotTypeTag, SlotType] : Slot.Inventory->Slots)
//			if (auto SameInventory = FindSlotInner(SlotTypeTag, SlotType))
//				return SameInventory;
//	}
//
//	if (MoveScope & IMS_World)
//		//Set optional with invalid slot implies that the world is a valid place to put this item.
//		return FInventorySlot{};
//
//	//No slot found
//	return {};
//}
//
//AItem* UInventoryComponent::GetItemInSlot(const FInventorySlot& Slot)
//{
//	auto SlotPtr = GetSlotPtr(Slot);
//
//	return SlotPtr ? SlotPtr->Item : nullptr;
//}
//
//
//bool UInventoryComponent::CanMoveItem(const AItem* Item, const FInventorySlot& Slot, bool bIgnoreExistingItem)
//{
//	if (!IsValid(Item))
//		return false;
//
//	if (!Slot.Inventory)
//		//Can always exit to world
//		return true;
//
//	if (!Item->MatchesSlotType(Slot.Inventory, Slot.SlotTypeTag))
//		return false;
//
//	if (!bIgnoreExistingItem)
//	{
//		auto SlotPtr = GetSlotPtr(Slot);
//
//		if (!SlotPtr)
//			return false;
//
//		if (SlotPtr->Item && SlotPtr->Item != Item)
//			//Already occupied (use transactions to move out any existing items first)
//			return false;
//	}
//
//	if (Slot.Inventory->IsContainedWithinItem(Item))
//		//Cannot contain self.
//		return false;
//
//	return true;
//}
//
//bool UInventoryComponent::TransactionAddMove(const FInventoryTransaction& Transaction, AItem* Item, const FInventorySlot& Slot, FInventoryTransaction& OutTransaction)
//{
//	OutTransaction = Transaction;
//	return OutTransaction.AddMove(Item, Slot);
//}
//
//TOptional<FInventoryTransaction> UInventoryComponent::MoveItem(AItem* Item, const FInventorySlot& NewSlot, int32 ItemMoveScope, int32 ExistingItemMoveScope)
//{
//	if (!IsValid(Item))
//		return {};
//
//	FInventoryTransaction Result;
//
//	if (Item->GetSlot() == NewSlot)
//		//Already there, so it's a valid but empty transaction.
//		return Result;
//
//	auto OldSlot = Item->GetSlot();
//
//	//First remove item from its slot so any potential swap has a place to move to.
//	Result.AddMove(Item, {});
//
//	auto ExistingItem = GetItemInSlot(NewSlot);
//
//	if (ExistingItem && (ExistingItemMoveScope & IMS_Swap))
//	{
//		//Try to swap existing item into the items old slot
//
//		auto ExistingItemFoundSlot = FindSlot(ExistingItem, OldSlot, ExistingItemMoveScope, false);
//
//		if (ExistingItemFoundSlot)
//			Result.AddMove(ExistingItem, *ExistingItemFoundSlot);
//	}
//
//	//At this point, any existing item either has moved out of the way, or can not be moved out of the way.
//
//	auto ItemFoundSlot = FindSlot(Item, NewSlot, ItemMoveScope, false);
//
//	if (!ItemFoundSlot)
//	{
//		//Cannot move item into desired slot
//		Result.Cancel();
//		return {};
//	}
//
//	//FindSlot() was set, so the result of CanMoveItem() to that slot is guaranteed to succeed
//	verify(Result.AddMove(Item, *ItemFoundSlot));
//
//	return Result;
//}
//
////bool UInventoryComponent::MoveItemBP(AItem* Item, const FInventorySlot& NewSlot, int32 ItemMoveScope, int32 ExistingItemMoveScope, FInventoryTransaction& OutTransaction)
////{
////	auto Result = MoveItem(Item, NewSlot, ItemMoveScope, ExistingItemMoveScope);
////
////	if (Result)
////		OutTransaction = *Result;
////
////	return Result.IsSet();
////}
//
//bool UInventoryComponent::MoveItemBP(AItem* Item, const FInventorySlot& NewSlot, FInventoryTransaction& OutTransaction)
//{
//		auto Result = MoveItem(Item, NewSlot, IMS_Exact, IMS_Swap);
//	
//		if (Result)
//			OutTransaction = *Result;
//	
//		return Result.IsSet();
//}
//
//TArray<FInventorySlot> UInventoryComponent::GetAllSlots() const
//{
//	TArray<FInventorySlot> Result;
//
//	FInventorySlot Slot;
//
//	Slot.Inventory = const_cast<UInventoryComponent*>(this);
//
//	for (auto& [SlotTypeTag, SlotType] : Slots)
//	{
//		Slot.SlotTypeTag = SlotTypeTag;
//
//		for (int32 i = 0; i < SlotType.NumSlots; ++i)
//		{
//			Slot.SlotIndex = i;
//			Result.Add(Slot);
//		}
//	}
//	return Result;
//}
//
//FInventorySlotData* UInventoryComponent::GetSlotPtr(const FInventorySlot& Slot)
//{
//	if (!IsValid(Slot.Inventory))
//		return nullptr;
//
//	auto SlotType = Slot.Inventory->Slots.Find(Slot.SlotTypeTag);
//
//	if (!SlotType)
//		return nullptr;
//
//	if (!SlotType->Items.IsValidIndex(Slot.SlotIndex))
//		return nullptr;
//
//	return &SlotType->Items[Slot.SlotIndex];
//}
//
//void UInventoryComponent::AddSlotObserver(const FInventorySlot& Slot, FInventorySlotChangedDelegate Delegate)
//{
//	auto SlotPtr = GetSlotPtr(Slot);
//
//	if (!SlotPtr)
//		return;
//
//	SlotPtr->OnSlotChanged.Add(Delegate);
//}
//
//void UInventoryComponent::RemoveSlotObserver(const FInventorySlot& Slot, FInventorySlotChangedDelegate Delegate)
//{
//	auto SlotPtr = GetSlotPtr(Slot);
//
//	if (!SlotPtr)
//		return;
//
//	SlotPtr->OnSlotChanged.RemoveSwap(Delegate);
//}
//
//void UInventoryComponent::InitSlots()
//{
//	for (auto& [Tag, SlotType] : Slots)
//	{
//		check(SlotType.Items.Num() == 0);
//
//		SlotType.Items.SetNum(SlotType.NumSlots);
//	}
//}

PRAGMA_ENABLE_OPTIMIZATION