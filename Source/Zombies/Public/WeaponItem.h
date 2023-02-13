//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "EquippableItem.h"
#include "WeaponItem.generated.h"

/**
 * Base class for weapons. Weapons are equippable items which have a set of utility tasks that implement their combat behavior.
 */
UCLASS()
class ZOMBIES_API AWeaponItem : public AEquippableItem
{
	GENERATED_BODY()
	
public:

	//Begin AEquippableItem

	virtual void SetPawn(class APawn* NewPawn) override;

	virtual void Equipped_Implementation(const FInventoryLocationHandle& OldLocation) override;

	virtual void Unequipped_Implementation(const FInventoryLocationHandle& OldLocation) override;

	//End AEquippableItem

	//Finds an owning object that has a weapon hosting interface
	UFUNCTION(BlueprintCallable, Category = Item)
	UObject* FindWeaponHost() const;

	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	void SetWeaponHost(UObject* NewWeaponHost);

	FORCEINLINE auto GetWeaponHost() const { return WeaponHost; }

	//Set the utility task component used by this weapons tasks
	UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
	void SetUtilityComponent(class UUtilityComponent* NewUtilityComponent);

	FORCEINLINE auto GetUtilityComponent() const { return UtilityComponent; };

	//Call after changing relevant actors to let the utility functions know
	UFUNCTION(BlueprintCallable, Category = Item)
	void NotifyRelevantActorsUpdated();

	//Actors that are relevant to the weapons logic, such as enemies or friendlies.
	//Typically this is the set of actors visible to the owning unit.
	UPROPERTY(BlueprintReadWrite, Category = Item)
	TSet<AActor*> RelevantActors;

protected:

	UPROPERTY(Transient, BlueprintReadWrite, BlueprintSetter = SetWeaponHost, Category = Item, Meta = (AllowPrivateAccess = "True"))
	UObject* WeaponHost;

	//Utility tasks to perform while this weapon is equipped.
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True"))
	TArray<class UUtilityTask*> UtilityTasks;

	//The utility component that is running the weapon behavior
	UPROPERTY(Transient, BlueprintReadWrite, BlueprintSetter = SetUtilityComponent, Category = Item, Meta = (AllowPrivateAccess = "True"))
	class UUtilityComponent* UtilityComponent;

	void UpdateUtilityTaskComponent();



};
