//Copyright Jarrad Alexander 2022


#include "Target.h"


bool FTarget::IsValid() const
{
	switch (Type)
	{
	case ETargetType::None:
		return false;
	case ETargetType::WorldLocation:
		return true;
	case ETargetType::RelativeLocation:
		return ::IsValid(Component);
	default:
		return false;
	}
}


FVector FTarget::GetLocation() const
{
	switch (Type)
	{
	case ETargetType::None:
		return FVector::ZeroVector;
	case ETargetType::WorldLocation:
		return Location;
	case ETargetType::RelativeLocation:
		return ::IsValid(Component) ? Component->GetComponentTransform().TransformPosition(Location) : FVector::ZeroVector;
	default:
		return FVector::ZeroVector;
	}
}

AActor* FTarget::GetActor() const 
{
	return ::IsValid(Component) ? Component->GetOwner() : nullptr;
}

void FTarget::SetWorldLocation(const FVector& InWorldLocation)
{
	Component = nullptr;
	Location = InWorldLocation;
	Type = ETargetType::WorldLocation;
}

bool FTarget::SetRelativeLocation(USceneComponent* InComponent, const FVector& InRelativeLocation)
{
	if (!::IsValid(InComponent))
	{
		Clear();
		return false;
	}

	Component = InComponent;
	Location = InRelativeLocation;
	Type = ETargetType::RelativeLocation;

	return true;
}

bool FTarget::SetRelativeLocation(const AActor* InActor, const FVector& InRelativeLocation)
{
	if (!::IsValid(InActor))
	{
		Clear();
		return false;
	}

	return SetRelativeLocation(InActor->GetRootComponent(), InRelativeLocation);
}
