//Copyright Jarrad Alexander 2022


#include "RangedWeaponItem.h"
#include "WeaponItem.h"
#include "GenericTeamAgentInterface.h"
#include "UnitInterface.h"
#include "AbilityComponent.h"
#include "ShootAbility.h"
#include "FocusComponent.h"
#include "DrawDebugHelpers.h"

void UUtilityTask_RangedWeapon::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	if (IsValid(Pawn))
	{
		PawnAbilityComponent = Pawn->FindComponentByClass<UAbilityComponent>();

		PawnFocusComponent = Pawn->FindComponentByClass<UFocusComponent>();
	}
	else
	{
		PawnAbilityComponent = nullptr;
		PawnFocusComponent = nullptr;
	}

}

void UUtilityTask_RangedWeapon::UpdateScore_Implementation()
{
	auto OldShootTarget = ShootTarget;

	Score = 0.f;

	ShootTarget = nullptr;

	auto WeaponItem = Cast<AWeaponItem>(GetOuter());

	if (!WeaponItem)
		return;

	if (!Pawn)
		return;

	FGenericTeamId PawnTeamId;

	if (auto TeamAgent = Cast<IGenericTeamAgentInterface>(Pawn))
		PawnTeamId = TeamAgent->GetGenericTeamId();

	TOptional<double> BestScore;

	AActor* BestTarget = nullptr;

	auto NormalizedDistanceWeight = DistanceWeight;

	auto NormalizedDirectionWeight = PawnFocusComponent ? DirectionWeight : 0.0;

	auto TotalWeight = NormalizedDirectionWeight + NormalizedDistanceWeight;

	NormalizedDirectionWeight /= TotalWeight;

	NormalizedDistanceWeight /= TotalWeight;

	FCollisionQueryParams CQP;

	CQP.AddIgnoredActor(Pawn);

	for (auto Actor : WeaponItem->RelevantActors)
	{
		if (!IsValid(Actor))
			continue;

		if (Actor->Implements<UUnitInterface>())
			if (IUnitInterface::Execute_IsDead(Actor))
				continue;

		FGenericTeamId ActorTeamId;

		if (auto TeamAgent = Cast<IGenericTeamAgentInterface>(Actor))
			ActorTeamId = TeamAgent->GetGenericTeamId();

		if (FGenericTeamId::GetAttitude(PawnTeamId, ActorTeamId) != ETeamAttitude::Hostile)
			continue;

		FVector Direction;
		
		double Distance;

		(Actor->GetActorLocation() - Pawn->GetActorLocation()).ToDirectionAndLength(Direction, Distance);

		if (Distance > MaxDistance)
			continue;

		auto DistanceScore = FMath::Max((MaxDistance - Distance) / MaxDistance, 0.0) * NormalizedDistanceWeight;

		/*
		@todo: should possibly consider commanded direction here
		E.G. Player commands unit to hold an angle, unit focuses on a stream of zombies coming in that direction. A zombie attacks from behind the unit and forces them to turn around.
		Unit sees another stream of zombies heading in a different direction and then focuses on that instead, neglecting the zombie stream that the player commanded them to attack.
		The way to implement this would be to add a command focus tag property to this task, and then check that focus level specifically to see if theres a desired scoring.
		If so, don't attack units facing away from the commanded direction unless they are focused on the unit.
		*/
		
		auto DirectionScore = (PawnFocusComponent ? (PawnFocusComponent->GetForwardVector() | Direction) * 0.5 + 0.5 : 0.0) * NormalizedDirectionWeight;

		auto CurrentScore = DistanceScore + DirectionScore;

		if (Actor == OldShootTarget)
			CurrentScore += CurrentTargetBonus;

		if (BestScore && CurrentScore <= *BestScore)
			continue;

		//@note: slight differences between fog of war shadows and actual collision meshes may effect this.
		//So figure out what exact metric we want later on.
		bool bHasLineOfSight = false;

		FHitResult HitResult;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Pawn->GetActorLocation(), Actor->GetActorLocation(), TraceChannel, CQP))
			bHasLineOfSight = HitResult.GetActor() == Actor;

		if (!bHasLineOfSight)
			continue;

		BestScore = CurrentScore;
		BestTarget = Actor;
		
	}

	if (BestScore && BestTarget)
	{
		Score = *BestScore;
		ShootTarget = BestTarget;

		//DrawDebugString(GetWorld(), FVector{ 0.0, 0.0, 100.0 }, FString::Printf(TEXT("%.2f"), Score), ShootTarget, FColor::Green, UpdateScoreFreq);
		//DrawDebugDirectionalArrow(GetWorld(), Pawn->GetActorLocation(), ShootTarget->GetActorLocation(), 40.f, FColor::Green, false, UpdateScoreFreq);
	}

}

void UUtilityTask_RangedWeapon::BeginTask_Implementation()
{
	Super::BeginTask_Implementation();

	if (IsValid(PawnFocusComponent) && IsValid(ShootTarget))
		PawnFocusComponent->SetFocusActorRelativeLocation(AimingFocusLevel, ShootTarget, FVector::ZeroVector);

	TryBeginAbility();
}

void UUtilityTask_RangedWeapon::TickTask_Implementation(float DeltaTime)
{
	Super::TickTask_Implementation(DeltaTime);

	if (IsValid(PawnFocusComponent) && IsValid(ShootTarget))
		PawnFocusComponent->SetFocusActorRelativeLocation(AimingFocusLevel, ShootTarget, FVector::ZeroVector);

	TryBeginAbility();
}

void UUtilityTask_RangedWeapon::EndTask_Implementation()
{
	Super::EndTask_Implementation();

	if (IsValid(PawnFocusComponent))
		PawnFocusComponent->ClearFocus(AimingFocusLevel);
}

void UUtilityTask_RangedWeapon::TryBeginAbility()
{
	if (!IsValid(PawnAbilityComponent) || !IsValid(ShootTarget))
		return;

	if (ShootTarget->Implements<UUnitInterface>())
		if (IUnitInterface::Execute_IsDead(ShootTarget))
		{
			//The target has died since we last looked for targets, so clear it and wait for the next target search.
			ShootTarget = nullptr;
			Score = 0.f;
			return;
		}

	if (!PawnAbilityComponent->CanBeginAbilityByClass(ShootAbilityClass))
		return;

	auto Ability = AAbility::CreateAbility<AShootAbility>(this, ShootAbilityClass);

	if (!Ability)
		return;

	ITargetedAbility::Execute_SetTarget(Ability, ShootTarget);

	if (!PawnAbilityComponent->BeginAbility(Ability))
	{
		Ability->DestroyAbility();
		Ability = nullptr;
	}

}

