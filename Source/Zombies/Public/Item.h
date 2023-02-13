//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemCommon.h"
#include "GameplayTagAssetInterface.h"
#include "Item.generated.h"

//Base class for items.
UCLASS()
class ZOMBIES_API AItem : public AActor, public IGameplayTagAssetInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//Begin IGameplayTagAssetInterface

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer.AppendTags(ItemTags); }

	//End IGameplayTagAssetInterface

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Item)
	FGameplayTagContainer ItemTags;

	////Called after entering inventory from game world
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Item)
	//void InventoryEntered();

	////Called after being moved inside inventories. Current inventory and old inventory may be different.
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Item)
	//void InventoryMoved(const FInventorySlot& OldSlot);

	////Called after leaving inventory and entering the game world.
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Item)
	//void InventoryExited(const FInventorySlot& OldSlot);

	////Called after inventory slot changes. E.G. moved in inventory, moved from world to any inventory, moved from inventory to world
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Item)
	//void InventorySlotChanged(const FInventorySlot& OldSlot);

	UFUNCTION(BlueprintCallable, Category = Item)
	FIntPoint GetItemSize() const;

	UFUNCTION(BlueprintCallable, Category = Item)
	bool CanSetItemSize(FIntPoint NewSize);

	UFUNCTION(BlueprintCallable, Category = Item)
	bool SetItemSize(FIntPoint NewSize);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Item)
	bool CanBePlacedInContainer(const FInventoryContainerHandle& Container) const;

	UFUNCTION(BlueprintCallable, Category = Item)
	bool CanSetInventoryLocation(const FInventoryLocationHandle& NewLocation) const;

	UFUNCTION(BlueprintCallable, Category = Item)
	bool SetInventoryLocation(const FInventoryLocationHandle& NewLocation);

	FORCEINLINE const auto& GetInventoryLocation() const { return InventoryLocation; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Item)
	void InventoryLocationChanged(const FInventoryLocationHandle& OldLocation);

	UPROPERTY(BlueprintAssignable, Category = Item)
	FInventoryLocationChangedDelegate OnInventoryLocationChanged;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	class UTexture* GetIconTexture();

	FORCEINLINE EItemIconType GetIconType() const { return IconType; }

	FORCEINLINE	TSubclassOf<class AItemIconRenderer> GetItemIconRendererClass() const { return ItemIconRendererClass; }

	FORCEINLINE int32 GetItemIconResolution() const { return ItemIconResolution; }

	FORCEINLINE const FTransform& GetItemIconTransform() const { return ItemIconTransform; }

	//Gets the actors that should be visible in this items icon (typically just the item itself, but can also include attachments)
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = Item)
	TArray<AActor*> GetIconVisibleActors();



protected:

	UPROPERTY(BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True"))
	FInventoryLocationHandle InventoryLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True"))
	FIntPoint ItemSize = FIntPoint{ 1, 1 };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True"))
	EItemIconType IconType = EItemIconType::ClassRendered;

	//The item icon for static icon
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True", EditCondition = "IconType == EItemIconType::StaticImage", EditConditionHides))
	class UTexture2D* StaticImageIcon;

	//The class of icon renderer to use for this item. Essentially specifies a basic lighting setup to use.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True", EditCondition = "IconType != EItemIconType::StaticImage", EditConditionHides))
	TSubclassOf<class AItemIconRenderer> ItemIconRendererClass;

	//Pixels per grid unit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True", EditCondition = "IconType != EItemIconType::StaticImage", EditConditionHides))
	int32 ItemIconResolution = 64;

	//The transform offset to use when rendering item icon
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True", EditCondition = "IconType != EItemIconType::StaticImage", EditConditionHides))
	FTransform ItemIconTransform;

	//Item icon for instance rendered icons.
	UPROPERTY(Transient)
	class UTextureRenderTarget2D* InstancedItemIcon;


};
