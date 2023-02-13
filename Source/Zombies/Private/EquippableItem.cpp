//Copyright Jarrad Alexander 2022


#include "EquippableItem.h"
//
//void AEquippableItem::InventoryEntered_Implementation()
//{
//	SetPawn(FindPawn());
//
//	if (IsEquipped())
//		Equipped({});
//	
//	Super::InventoryEntered_Implementation();
//}
//
//void AEquippableItem::InventoryMoved_Implementation(const FInventorySlot& OldSlot)
//{
//	SetPawn(FindPawn());
//
//	bool bOldEquipped = IsEquipmentSlotType(OldSlot.SlotTypeTag);
//	bool bNewEquipped = IsEquipmentSlotType(GetSlot().SlotTypeTag);
//
//	if (bOldEquipped && !bNewEquipped)
//		Unequipped(OldSlot);
//	else if (!bOldEquipped && bNewEquipped)
//		Equipped(OldSlot);
//
//	Super::InventoryMoved_Implementation(OldSlot);
//}
//
//void AEquippableItem::InventoryExited_Implementation(const FInventorySlot& OldSlot)
//{
//	SetPawn(nullptr);
//
//	if (IsEquipmentSlotType(OldSlot.SlotTypeTag))
//		Unequipped(OldSlot);
//
//	Super::InventoryExited_Implementation(OldSlot);
//}

void AEquippableItem::InventoryLocationChanged_Implementation(const FInventoryLocationHandle& OldLocation)
{
	SetPawn(FindPawn());

	bool bOldEquipped = IsEquipmentContainer(OldLocation.Container);
	bool bNewEquipped = IsEquipmentContainer(GetInventoryLocation().Container);

	if (bOldEquipped && !bNewEquipped)
		Unequipped(OldLocation);
	else if (!bOldEquipped && bNewEquipped)
		Equipped(OldLocation);

	Super::InventoryLocationChanged_Implementation(OldLocation);
}

void AEquippableItem::Equipped_Implementation(const FInventoryLocationHandle& OldLocation)
{
}

void AEquippableItem::Unequipped_Implementation(const FInventoryLocationHandle& OldLocation)
{
}

bool AEquippableItem::IsEquipped() const
{
	return IsEquipmentContainer(GetInventoryLocation().Container);
}

bool AEquippableItem::IsEquipmentContainer(FGameplayTag Container) const
{
	return Container.MatchesTag(EquipmentContainerTag);
}

APawn* AEquippableItem::FindPawn() const
{
	for (AActor* Actor = GetOwner(); Actor; Actor = Actor->GetOwner())
		if (auto Result = Cast<APawn>(Actor))
			return Result;

	return nullptr;
}

void AEquippableItem::SetPawn(APawn* NewPawn)
{
	Pawn = NewPawn;
}
