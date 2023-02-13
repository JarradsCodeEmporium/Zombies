//Copyright Jarrad Alexander 2022


#include "WeaponItem.h"
#include "UtilityComponent.h"
#include "UtilityTask.h"
#include "AIController.h"
#include "WeaponHost.h"

void AWeaponItem::SetPawn(APawn* NewPawn)
{
	Super::SetPawn(NewPawn);

	for (auto Task : UtilityTasks)
		Task->SetPawn(NewPawn);
}

void AWeaponItem::Equipped_Implementation(const FInventoryLocationHandle& OldLocation)
{	
	SetWeaponHost(FindWeaponHost());

	UpdateUtilityTaskComponent();

	Super::Equipped_Implementation(OldLocation);
}

void AWeaponItem::Unequipped_Implementation(const FInventoryLocationHandle& OldLocation)
{
	SetWeaponHost(nullptr);

	UpdateUtilityTaskComponent();

	Super::Unequipped_Implementation(OldLocation);
}

UObject* AWeaponItem::FindWeaponHost() const
{
	for (auto Actor = GetOwner(); Actor; Actor = Actor->GetOwner())
		if (Actor->Implements<UWeaponHost>())
			return Actor;

	return nullptr;
}

void AWeaponItem::SetWeaponHost(UObject* NewWeaponHost)
{
	if (IsValid(WeaponHost))
		IWeaponHost::Execute_UnregisterWeapon(WeaponHost, this);

	WeaponHost = nullptr;

	if (!IsValid(NewWeaponHost) || !NewWeaponHost->Implements<UWeaponHost>())
		return;

	WeaponHost = NewWeaponHost;

	IWeaponHost::Execute_RegisterWeapon(WeaponHost, this);
}

void AWeaponItem::SetUtilityComponent(UUtilityComponent* NewUtilityComponent)
{
	UtilityComponent = NewUtilityComponent;

	UpdateUtilityTaskComponent();
}

void AWeaponItem::NotifyRelevantActorsUpdated()
{
	auto Tasks = UtilityTasks;

	for (auto Task : Tasks)
		if (IsValid(Task))
			Task->UpdateScore();
}

void AWeaponItem::UpdateUtilityTaskComponent()
{
	//Only bind to a utility task component when equipped
	auto NewUtilityComponent = IsEquipped() ? UtilityComponent : nullptr;
	
	//Begin/End task events could theoretically trigger something to modify this array
	auto Tasks = UtilityTasks;

	for (auto Task : Tasks)
		if (IsValid(Task))
			Task->SetUtilityComponent(NewUtilityComponent);
}