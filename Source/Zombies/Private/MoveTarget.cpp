//Copyright Jarrad Alexander 2022


#include "MoveTarget.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

AMoveTarget::AMoveTarget()
{
	PrimaryActorTick.bCanEverTick = true;

	//Don't bother ticking very often, since we don't want to be constantly retrying pathfinding every frame when unable to find a valid path
	PrimaryActorTick.TickInterval = 0.5f;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));

}

// Called when the game starts or when spawned
void AMoveTarget::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMoveTarget::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdatePawnActions();

}

void AMoveTarget::BeginPlayPooled_Implementation()
{
}

void AMoveTarget::EndPlayPooled_Implementation(EEndPlayReason::Type Reason)
{
	SetPawn(nullptr);
}

void AMoveTarget::SetPawn(APawn* InPawn)
{
	if (Pawn)
	{
		if (auto AIController = Pawn->GetController<AAIController>())
		{
			AIController->ReceiveMoveCompleted.RemoveAll(this);

			AbortMove(AIController);
		}

		if (auto Focus = Pawn->FindComponentByClass<UFocusComponent>())
			Focus->ClearFocus(FocusTag);
	}
	
	Pawn = InPawn;

	if (Pawn)
	{
		if (auto AIController = Pawn->GetController<AAIController>())
			AIController->ReceiveMoveCompleted.AddDynamic(this, &AMoveTarget::ReceiveMoveCompleted);

		//Generally this actor doesn't tick very often, so we want to immediately start a move if necessary here
		UpdatePawnActions();
	}

}

bool AMoveTarget::WantsFocusing() const
{
	if (!IsValid(Pawn))
		return false;

	//Either close enough in euclidean distance, or close enough based on estimated remaining path cost
	bool bIsInFocusRange = FVector::DistSquared2D(GetActorLocation(), Pawn->GetActorLocation()) < FMath::Square(FocusAtMinDistance);

	auto AIController = Pawn->GetController<AAIController>();

	if (!IsValid(AIController))
		return bIsInFocusRange;

	if (!bIsInFocusRange)
		if (auto PathFollower = AIController->GetPathFollowingComponent())
			bIsInFocusRange = PathFollower->GetRemainingPathCost() < FocusAtRemainingPathCost;

	return bIsInFocusRange;
}

void AMoveTarget::SetFocusOverride(const FTarget& InFocusOverride)
{
	FocusOverride = InFocusOverride;
	UpdateFocus();
}

bool AMoveTarget::IsLocationConditionSatisfied() const
{
	if (!IsValid(Pawn))
		return false;
	
	if (auto AIController = Pawn->GetController<AAIController>())
		if (auto PathFollower = AIController->GetPathFollowingComponent())
			return PathFollower->HasReached(*this, EPathFollowingReachMode::ExactLocation, AcceptanceRadius);

	//No path follower to perform detailed check, just use 2D location in this case
	return FVector::DistSquared2D(GetActorLocation(), Pawn->GetActorLocation()) < FMath::Square(AcceptanceRadius >= 0.f ? AcceptanceRadius : 20.f);
}

bool AMoveTarget::IsFocusConditionSatisfied() const
{
	if (!IsValid(Pawn))
		return false;

	if (!bEnableFocusing || !FocusTag.IsValid() || FocusAtMinDistance < 0.f && FocusAtRemainingPathCost < 0.f)
		//Focusing is not wanted, so we always satisfy it
		return true;

	auto Focus = Pawn->FindComponentByClass<UFocusComponent>();

	if (!Focus)
		return true;

	if (!Focus->IsFocusLevelActive(FocusTag))
		//If our level is not active, it means something else more important took over and we don't care about matching it any more.
		return true;

	return Focus->IsFacingFocus2D(FocusAcceptanceAngleDeg);
}

FTarget AMoveTarget::GetFocusTarget() const
{
	if (FocusOverride)
		return FocusOverride;

	return FTarget{ this, FVector{100000.0, 0.0, 0.0} };
}

EPathFollowingRequestResult::Type AMoveTarget::StartMove(AAIController* AIController)
{
	if (!IsValid(AIController))
		return EPathFollowingRequestResult::Failed;

	//AI controller expects only 1 move active at a time, and this is kind of the gameplay code entry point for specifying AI pathfinding.
	//We set acceptance radius on the actual pathfinding very low so that it keeps trying to match exactly, and then accept the higher radius in IsLocationConditionSatisfied()
	return AIController->MoveToActor(this, -1.f, bReachTestIncludesAgentRadius, bUsePathfinding, bCanStrafe, NavFilter, bAllowPartialPath);
}

void AMoveTarget::AbortMove(AAIController* AIController)
{
	if (!IsValid(AIController))
		return;

	auto PathFollower = AIController->GetPathFollowingComponent();

	if (!IsValid(PathFollower))
		return;

	PathFollower->AbortMove(*this, FPathFollowingResultFlags::UserAbort, FAIRequestID::CurrentRequest, EPathFollowingVelocityMode::Keep);

}

void AMoveTarget::ReceiveMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
}

void AMoveTarget::UpdatePawnActions()
{
	UpdateMove();
	UpdateFocus();
}

void AMoveTarget::UpdateMove()
{
	if (!IsValid(Pawn))
		return;

	if (auto AIController = Pawn->GetController<AAIController>())
		if (AIController->GetMoveStatus() == EPathFollowingStatus::Idle)
			if (!IsLocationConditionSatisfied())
				StartMove(AIController);
}

void AMoveTarget::UpdateFocus()
{
	if (!IsValid(Pawn))
		return;

	if (auto Focus = Pawn->FindComponentByClass<UFocusComponent>())
		if (bEnableFocusing && WantsFocusing())
			Focus->SetFocus(FocusTag, GetFocusTarget());
		else
			Focus->ClearFocus(FocusTag);
}

