//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "BaseUnit.h"
#include "WeaponHost.h"
#include "PlayerUnit.generated.h"

/**
 * Base class for player owned and controlled characters that persist between levels and sessions.
 */
UCLASS()
class ZOMBIES_API APlayerUnit : public ABaseUnit, public IWeaponHost
{
	GENERATED_BODY()
public:

	APlayerUnit();

protected:

	virtual void BeginPlay() override;

public:

	virtual void SetOwner(AActor* NewOwner) override;

	//Begin IWeaponHost

	virtual void RegisterWeapon_Implementation(class AWeaponItem* WeaponItem) override;

	virtual void UnregisterWeapon_Implementation(class AWeaponItem* WeaponItem) override;

	//End IWeaponHost

protected:

	//Fog of war vision. Currently only player units have vision. This may change later, if so, just put this on the base unit class.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	class UFogOfWarVisionComponent* FogOfWarVisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	class UInventoryComponent* InventoryComponent;

	//Weapons which have subscribed to the combat relevant events and data that this unit generates.
	UPROPERTY(BlueprintReadOnly, Category = Unit, Meta = (AllowPrivateAccess = "True"))
	TSet<class AWeaponItem*> RegisteredWeapons;

	UFUNCTION()
	void OnVisibleActorsUpdated(class UFogOfWarVisionComponent* VisionComponent, const TSet<AActor*>& OldVisible, const TSet<AActor*>& NewVisible);

};
