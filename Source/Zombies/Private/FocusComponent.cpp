//Copyright Jarrad Alexander 2022


#include "FocusComponent.h"
//#include "Kismet/KismetMathLibrary.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "DrawDebugHelpers.h"

//PRAGMA_DISABLE_OPTIMIZATION

// Sets default values for this component's properties
UFocusComponent::UFocusComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UFocusComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UFocusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	InterpolateToFocus(DeltaTime);
}

bool UFocusComponent::IsFacingFocus(double AngleErrorDegrees) const
{
	AngleErrorDegrees = FMath::Clamp(AngleErrorDegrees, 0.0, 180.0);

	auto Current = GetForwardVector();

	auto Goal = (GetFocusLocation() - GetComponentLocation()).GetSafeNormal();

	return (Current | Goal) >= FMath::Cos(FMath::DegreesToRadians(AngleErrorDegrees));
}

bool UFocusComponent::IsFacingFocus2D(double AngleErrorDegrees) const
{
	AngleErrorDegrees = FMath::Clamp(AngleErrorDegrees, 0.0, 180.0);

	FVector2D Current{ GetForwardVector() };

	FVector2D Goal = (FVector2D{ GetFocusLocation() } - FVector2D{ GetComponentLocation() }).GetSafeNormal();

	return (Current | Goal) >= FMath::Cos(FMath::DegreesToRadians(AngleErrorDegrees));
}

FTarget UFocusComponent::GetFocusAtLevel(FGameplayTag Tag) const
{
	if (auto Level = GetFocusLevel(Tag))
		return Level->Target;

	return {};
}

FTarget UFocusComponent::GetFocus() const
{
	auto Focus = GetActiveFocusLevel();

	if (!Focus)
		return FTarget{};

	return Focus->Target;
}

bool UFocusComponent::SetFocus(FGameplayTag Tag, const FTarget& InFocusTarget)
{
	if (auto Level = GetFocusLevel(Tag))
	{
		Level->Target = InFocusTarget;
		return true;
	}

	return false;
}

bool UFocusComponent::SetFocusWorldLocation(FGameplayTag Tag, const FVector& InWorldLocation)
{
	if (auto Level = GetFocusLevel(Tag))
	{
		Level->Target.SetWorldLocation(InWorldLocation);
		return true;
	}

	return false;
}

bool UFocusComponent::SetFocusRelativeLocation(FGameplayTag Tag, USceneComponent* InComponent, const FVector& InRelativeLocation)
{
	if (auto Level = GetFocusLevel(Tag))
		return Level->Target.SetRelativeLocation(InComponent, InRelativeLocation);

	return false;
}

bool UFocusComponent::SetFocusActorRelativeLocation(FGameplayTag Tag, AActor* InActor, const FVector& InRelativeLocation)
{
	if (auto Level = GetFocusLevel(Tag))
		return Level->Target.SetRelativeLocation(InActor, InRelativeLocation);

	return false;
}

bool UFocusComponent::ClearFocus(FGameplayTag Tag)
{
	if (auto Level = GetFocusLevel(Tag))
	{
		Level->Target.Clear();
		return true;
	}

	return false;
}

void UFocusComponent::ClearAllFocus()
{
	for (auto& Level : FocusLevels)
		Level.Target.Clear();
}

FFocusLevel* UFocusComponent::GetActiveFocusLevel()
{
	for (auto& Level : FocusLevels)
		if (Level.Target.IsValid())
			return &Level;

	return nullptr;
}

FFocusLevel* UFocusComponent::GetFocusLevel(FGameplayTag Tag)
{
	if (!Tag.IsValid())
		return nullptr;

	for (auto& Level : FocusLevels)
		if (Tag.MatchesTag(Level.Tag))
			return &Level;

	return nullptr;
}

void UFocusComponent::InterpolateToFocus(float DeltaTime)
{
	AAIController* AIController = nullptr;

	for (auto Actor = GetOwner(); Actor; Actor = Actor->GetOwner())
		if (auto Pawn = Cast<APawn>(Actor))
			if (auto Controller = Pawn->GetController<AAIController>())
			{
				AIController = Controller;
				break;
			}

	if (bDrivePathfindingFocus && AIController)
		if (auto PathfindingLevel = GetFocusLevel(PathfindingFocusTag))
			if (auto PathFollowingComponent = AIController->GetPathFollowingComponent())
			{
				if (PathFollowingComponent->GetStatus() == EPathFollowingStatus::Moving)
				{
					auto Direction = PathFollowingComponent->GetCurrentDirection();

					PathfindingLevel->Target.SetWorldLocation(GetComponentLocation() + Direction * 100000.0);
				}
				else if (PathFollowingComponent->GetStatus() == EPathFollowingStatus::Idle)
				{
					PathfindingLevel->Target.Clear();
				}
			}

	auto Level = GetActiveFocusLevel();

	if (!Level || !Level->Target.IsValid())
		return;

	auto FocusPoint = Level->Target.GetLocation();

	auto Current = GetComponentQuat();

	auto Goal = (FocusPoint - GetComponentLocation()).Rotation().Quaternion();

	FQuat Interp;

	if (Level->bConstantSpeed)
		Interp = FMath::QInterpConstantTo(Current, Goal, DeltaTime, Level->InterpSpeed);
	else
		Interp = FMath::QInterpTo(Current, Goal, DeltaTime, Level->InterpSpeed);

	SetWorldRotation(Interp);

	if (AIController && bDriveOwnersFocus)
		AIController->SetFocalPoint(GetComponentTransform().TransformPosition(FVector{ 100000.0, 0.0, 0.0 }), EAIFocusPriority::Gameplay);

}

const FFocusLevel* UFocusComponent::GetActiveFocusLevel() const
{
	return const_cast<UFocusComponent*>(this)->GetActiveFocusLevel();
}

const FFocusLevel* UFocusComponent::GetFocusLevel(FGameplayTag Tag) const
{
	return const_cast<UFocusComponent*>(this)->GetFocusLevel(Tag);

}

//PRAGMA_ENABLE_OPTIMIZATION