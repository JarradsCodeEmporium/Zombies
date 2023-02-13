//Copyright Jarrad Alexander 2022


#include "PlayerUnit.h"
#include "FogOfWarVisionComponent.h"
#include "FogOfWarDisplayComponent.h"
#include "FocusComponent.h"
#include "InventoryComponent.h"
#include "WeaponItem.h"
#include "AIController.h"
#include "UtilityComponent.h"

APlayerUnit::APlayerUnit()
{
	FogOfWarVisionComponent = CreateDefaultSubobject<UFogOfWarVisionComponent>("FogOfWarVisionComponent");

	FogOfWarVisionComponent->SetupAttachment(FocusComponent);

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");

}

void APlayerUnit::BeginPlay()
{
	FogOfWarVisionComponent->OnVisibleActorsUpdated.AddDynamic(this, &APlayerUnit::OnVisibleActorsUpdated);

	Super::BeginPlay();
}

void APlayerUnit::SetOwner(AActor* NewOwner)
{
	if (NewOwner == GetOwner())
		return;

	if (auto OldOwner = GetOwner(); IsValid(OldOwner))
		if (auto Display = OldOwner->FindComponentByClass<UFogOfWarDisplayComponent>())
			Display->RemoveVisionComponent(FogOfWarVisionComponent);

	Super::SetOwner(NewOwner);

	if (auto CurrentOwner = GetOwner(); IsValid(CurrentOwner))
		if (auto Display = CurrentOwner->FindComponentByClass<UFogOfWarDisplayComponent>())
			Display->AddVisionComponent(FogOfWarVisionComponent);
}

void APlayerUnit::RegisterWeapon_Implementation(AWeaponItem* WeaponItem)
{
	if (!IsValid(WeaponItem))
		return;

	RegisteredWeapons.Add(WeaponItem);

	if (auto AIController = GetController<AAIController>())
		if (auto UtilityComponent = Cast<UUtilityComponent>(AIController->GetBrainComponent()))
			WeaponItem->SetUtilityComponent(UtilityComponent);
	
	WeaponItem->RelevantActors = FogOfWarVisionComponent->GetVisibleActors();

	WeaponItem->NotifyRelevantActorsUpdated();
}

void APlayerUnit::UnregisterWeapon_Implementation(AWeaponItem* WeaponItem)
{
	if (RegisteredWeapons.Remove(WeaponItem) != 1)
		return;

	WeaponItem->SetUtilityComponent(nullptr);

	WeaponItem->RelevantActors.Empty();

	WeaponItem->NotifyRelevantActorsUpdated();

}

void APlayerUnit::OnVisibleActorsUpdated(UFogOfWarVisionComponent* VisionComponent, const TSet<AActor*>& OldVisible, const TSet<AActor*>& NewVisible)
{
	//Push visible actors to any equipped items so they can run their attack logic
	for (auto Weapon : RegisteredWeapons)
		if (IsValid(Weapon))
		{
			Weapon->RelevantActors = NewVisible;
			Weapon->NotifyRelevantActorsUpdated();
		}
}
