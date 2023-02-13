//Copyright Jarrad Alexander 2022


#include "Item.h"
#include "InventoryComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ItemIconSubsystem.h"
#include "ItemIconRenderer.h"

PRAGMA_DISABLE_OPTIMIZATION

// Sets default values
AItem::AItem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	static ConstructorHelpers::FClassFinder<class AItemIconRenderer> DefaultIconRendererClass(TEXT("Script/Engine.Blueprint'/Game/UI/BP_DefaultItemIconRenderer.BP_DefaultItemIconRenderer_C'"));

	ItemIconRendererClass = DefaultIconRendererClass.Class;

}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}



FIntPoint AItem::GetItemSize() const
{
	return ItemSize;
}

bool AItem::CanSetItemSize(FIntPoint NewSize)
{
	if (!UInventoryComponent::IsValidLocation(InventoryLocation))
		return true;

	FInventoryLocationHandle NewLocation = InventoryLocation;

	NewLocation.Rect.SetUnrotatedSize(NewSize);

	if (!UInventoryComponent::CanPlaceItemAt(this, NewLocation))
		return false;

	return true;
}

bool AItem::SetItemSize(FIntPoint NewSize)
{
	if (!CanSetItemSize(NewSize))
		return false;

	if (ItemSize != NewSize)
		//Invalidate existing instanced item icon
		InstancedItemIcon = nullptr;

	ItemSize = NewSize;

	FInventoryLocationHandle NewLocation = InventoryLocation;

	NewLocation.Rect.SetUnrotatedSize(NewSize);

	verify(SetInventoryLocation(NewLocation));

	return true;
}

bool AItem::CanBePlacedInContainer_Implementation(const FInventoryContainerHandle& Container) const
{
	if (!UInventoryComponent::ItemMatchesContainerFilter(this, Container))
		return false;

	return true;
}

bool AItem::CanSetInventoryLocation(const FInventoryLocationHandle& NewLocation) const
{
	if (!UInventoryComponent::IsValidLocation(NewLocation))
		//World slot can always accept the item
		return true;

	if (GetItemSize() != NewLocation.Rect.GetUnrotatedSize())
		return false;
	
	if (!UInventoryComponent::CanPlaceItemAt(this, NewLocation))
		return false;

	return true;
}

bool AItem::SetInventoryLocation(const FInventoryLocationHandle& NewLocation)
{
	if (!CanSetInventoryLocation(NewLocation))
		return false;

	auto OldLocation = InventoryLocation;

	auto OldContainer = UInventoryComponent::GetContainer(InventoryLocation);

	if (OldContainer)
		OldContainer->Items.RemoveSingleSwap(this);
	
	InventoryLocation = NewLocation;

	SetOwner(InventoryLocation.Inventory ? InventoryLocation.Inventory->GetOwner() : nullptr);

	auto NewContainer = UInventoryComponent::GetContainer(InventoryLocation);

	if (NewContainer)
		NewContainer->Items.Add(this);

	InventoryLocationChanged(OldLocation);

	{
		auto Delegate = OnInventoryLocationChanged;
		Delegate.Broadcast(this, OldLocation);
	}

	if (OldContainer && OldContainer == NewContainer)
	{
		auto Delegates = OldContainer->OnChanged;

		for (auto& Delegate : Delegates)
			Delegate.ExecuteIfBound(this, OldLocation, EInventoryContainerChangeType::Moved);
	}
	else
	{
		if (OldContainer)
		{
			auto Delegates = OldContainer->OnChanged;

			for (auto& Delegate : Delegates)
				Delegate.ExecuteIfBound(this, OldLocation, EInventoryContainerChangeType::Removed);
		}

		if (NewContainer)
		{
			auto Delegates = NewContainer->OnChanged;

			for (auto& Delegate : Delegates)
				Delegate.ExecuteIfBound(this, NewLocation, EInventoryContainerChangeType::Added);
		}
	}

	return true;
}

UTexture* AItem::GetIconTexture()
{
	auto Subsystem = GetWorld()->GetSubsystem<UItemIconSubsystem>();

	switch (IconType)
	{
	case EItemIconType::StaticImage:
		return StaticImageIcon;
	case EItemIconType::ClassRendered:
	{
		if (!Subsystem)
			return nullptr;

		return Subsystem->GetClassIcon(GetClass());
	}
	case EItemIconType::InstanceRendered:
	{
		if (InstancedItemIcon)
			return InstancedItemIcon;

		InstancedItemIcon = Subsystem->CreateIconRenderTarget(this, this);

		Subsystem->RenderIcon(this, InstancedItemIcon);

		return InstancedItemIcon;
	}
	default:
		return nullptr;
	}
}

TArray<AActor*> AItem::GetIconVisibleActors_Implementation()
{
	return TArray<AActor*>{this};
}

void AItem::InventoryLocationChanged_Implementation(const FInventoryLocationHandle& OldLocation)
{
}


PRAGMA_ENABLE_OPTIMIZATION