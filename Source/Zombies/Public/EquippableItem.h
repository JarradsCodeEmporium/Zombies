//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "EquippableItem.generated.h"

/**
 * 
 */
UCLASS()
class ZOMBIES_API AEquippableItem : public AItem
{
	GENERATED_BODY()
public:

	//virtual void InventoryEntered_Implementation() override;

	//virtual void InventoryMoved_Implementation(const FInventorySlot& OldSlot) override;

	//virtual void InventoryExited_Implementation(const FInventorySlot& OldSlot) override;

	virtual void InventoryLocationChanged_Implementation(const FInventoryLocationHandle& OldLocation) override;


	//Called just after entering an equipment slot.
	//Old slot may be invalid if equipping directly from world
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	void Equipped(const FInventoryLocationHandle& OldLocation);

	//Called just after leaving an equipment slot.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item")
	void Unequipped(const FInventoryLocationHandle& OldLocation);

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool IsEquipped() const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool IsEquipmentContainer(FGameplayTag Container) const;

	UFUNCTION(BlueprintCallable, Category = Item)
	class APawn* FindPawn() const;

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	virtual void SetPawn(class APawn* NewPawn);
	
	FORCEINLINE class APawn* GetPawn() const { return Pawn; };

protected:

	//Any slot type tag that matches this tag will be considered an equipment slot for this item, and will trigger equipped/unequipped events when entering/leaving
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True"))
	FGameplayTag EquipmentContainerTag;

	//The pawn that this Item is equipped on (if any)
	UPROPERTY(BlueprintReadWrite, BlueprintSetter = SetPawn, Category = Item, Meta = (AllowPrivateAccess = "True"))
	APawn* Pawn;

};
