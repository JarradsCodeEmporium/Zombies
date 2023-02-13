//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponHost.generated.h"

UINTERFACE(MinimalAPI)
class UWeaponHost : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ZOMBIES_API IWeaponHost
{
	GENERATED_BODY()

public:

	//Registers the weapon with the host, subscribing it to the events and data which weapons need in order to perform their function.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = WeaponHost)
	void RegisterWeapon(class AWeaponItem* WeaponItem);

	//Unregisters weapon
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = WeaponHost)
	void UnregisterWeapon(class AWeaponItem* WeaponItem);

};
