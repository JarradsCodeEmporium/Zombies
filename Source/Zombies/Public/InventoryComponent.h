//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemCommon.h"
#include "InventoryComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ZOMBIES_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

//	virtual void InitializeComponent() override;
//	
//	// Called every frame
//	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Whether the slot refers to a valid slot on an existing component
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static bool IsValidContainer(const FInventoryContainerHandle& Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static bool IsValidLocation(const FInventoryLocationHandle& Location);

	//Whether this inventory is contained within the given item.
	UFUNCTION(BlueprintCallable, Category = Item)
	bool IsContainedWithinItem(const AItem* Item) const;

	//Whether this inventory is contained within the other inventory.
	UFUNCTION(BlueprintCallable, Category = Item)
	bool IsContainedWithinInventory(const UInventoryComponent* OtherInventory) const;

	//Gets all the items that overlap a given inventory location
	UFUNCTION(BlueprintCallable, Category = Item)
	static TArray<AItem*> GetOverlappingItems(const FInventoryLocationHandle& Location);

	FORCEINLINE auto& GetContainers() const { return Containers; }

	FORCEINLINE static FInventoryContainer* GetContainer(const FInventoryContainerHandle& Container)
	{
		return Container.Inventory ? Container.Inventory->Containers.Find(Container.Container) : nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = Item)
	static void AddContainerObserver(const FInventoryContainerHandle& Container, FInventoryContainerChangedDelegate Delegate);

	UFUNCTION(BlueprintCallable, Category = Item)
	static void RemoveContainerObserver(const FInventoryContainerHandle& Container, FInventoryContainerChangedDelegate Delegate);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static FText GetContainerName(const FInventoryContainerHandle& Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static FIntPoint GetContainerSize(const FInventoryContainerHandle& Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static FGameplayTagQuery GetContainerFilter(const FInventoryContainerHandle& Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static int32 GetContainerMaxItems(const FInventoryContainerHandle& Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static TArray<AItem*> GetContainerItems(const FInventoryContainerHandle& Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static bool ItemMatchesContainerFilter(const AItem* Item, const FInventoryContainerHandle& Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static bool CanPlaceItemInContainer(const AItem* Item, const FInventoryContainerHandle& Container);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static bool CanPlaceItemAt(const AItem* Item, const FInventoryLocationHandle& Location);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static FInventoryLocationHandle FindLocationForItem(const AItem* Item, const FInventoryContainerHandle& Container);

	//Find the closest location for an item.
	//@Param Item: The item to find a position for.
	//@Param Container: The container handle to search in.
	//@Param QueryLocation: The location closest to the center point of the item to search.
	//@Param bPreferredRotation: Find a spot for the item in this orientation first.
	//@Param bAllowRotation: Whether the non-preferred rotation can also be accepted.
	//@Param bAllowInvalidLocations: If a valid location is not found at all, can we still return the closest obstructed location? Generally only used for UI item drag preview.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Item)
	static FInventoryLocationHandle FindClosestLocationForItem(const AItem* Item, const FInventoryContainerHandle& Container, FIntPoint QueryLocation, bool bPreferredRotation, bool bAllowRotation, bool bAllowInvalidLocations = false);

protected:

	//The structure and contents of this inventory.
	//Slot types are identified by this tag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True"))
	TMap<FGameplayTag, FInventoryContainer> Containers;

	static bool ItemRectFitsContainer(const FItemRect& Rect, const FInventoryContainer& Container);

	static bool ItemRectOverlapsOthers(const AItem* Item, const FItemRect& Rect, const FInventoryContainer& Container);

	static bool ContainerHasRoomForItem(const AItem* Item, const FInventoryContainer& Container);
};
