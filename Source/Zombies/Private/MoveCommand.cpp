//Copyright Jarrad Alexander 2022


#include "MoveCommand.h"
//#include "Kismet/KismetMathLibrary.h"
#include "WorldCursorComponent.h"
#include "CommanderComponent.h"
#include "CommandFollowerComponent.h"
//#include "BaseUnit.h"
#include "AIController.h"
//#include "BehaviorTree/BlackboardComponent.h"
#include "MoveTarget.h"
#include "PooledActorSubsystem.h"


AMoveCommand::AMoveCommand()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));

	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AMoveCommand::BeginPlayPooled_Implementation()
{
	Super::BeginPlayPooled_Implementation();
}

void AMoveCommand::InitCommand_Implementation()
{
	Super::InitCommand_Implementation();

	if (auto WorldCursor = GetWorldCursorComponent())
	{
		FWorldCursorEventDelegate Delegate;

		Delegate.BindDynamic(this, &AMoveCommand::CursorHandler);

		CursorHandle = WorldCursor->AddDelegate(Delegate);
	}
}

void AMoveCommand::BeginFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* PreviousCommand)
{
	Super::BeginFollowingCommand_Implementation(Follower, PreviousCommand);

	AssignMoveTarget(Follower);

}

void AMoveCommand::TickFollowingCommand_Implementation(UCommandFollowerComponent* Follower, float DeltaTime)
{
	Super::TickFollowingCommand_Implementation(Follower, DeltaTime);

	if (!IsValid(Follower))
		return;
	
	AssignMoveTarget(Follower);

	//Allow finishing command when location and focus conditions of the target are satisfied
	if (auto MoveTarget = MoveTargets.Find(Follower); MoveTarget && *MoveTarget)
		Follower->SetIsFinishingCommand(this, (*MoveTarget)->IsLocationConditionSatisfied() && (*MoveTarget)->IsFocusConditionSatisfied());
	
}

void AMoveCommand::EndFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* NextCommand)
{
	Super::EndFollowingCommand_Implementation(Follower, NextCommand);

	ClearMoveTarget(Follower);
}

void AMoveCommand::SetMoveCommandType(EMoveCommandType NewType)
{
	if (Type == NewType)
		return;

	auto OldType = Type;

	Type = NewType;

	OnMoveCommandTypeChanged(OldType);
}

void AMoveCommand::ReceiveEndCommand_Implementation()
{
	Super::ReceiveEndCommand_Implementation();

	SetMoveCommandType(EMoveCommandType::Location);

	UWorldCursorComponent::RemoveDelegate(CursorHandle);

	CursorHandle.Reset();

	for (auto [Follower, MoveTarget] : MoveTargets)
		if (IsValid(MoveTarget))
			UPooledActorSubsystem::DestroyPooledActor(MoveTarget, EEndPlayReason::Destroyed);

}

void AMoveCommand::UpdateFromCursorLocation(const FWorldCursorLocation& Cursor)
{
	//This is more of an "angle selection" widget rather than a "face towards exact point in world" widget
	//So instead of tracing against world, we intersect a single horizontal plane placed at the actors location.

	FVector Direction;

	double Distance;

	if (!GetCursorDirectionAndDistance(Cursor, Direction, Distance))
		return;

	auto CursorAngle = FMath::Atan2(Direction.Y, Direction.X);

	bool bWantsFacingMove = true;

	bWantsFacingMove &= Distance > FacingMoveDistance;

	bWantsFacingMove &= (InitialCursorScreenLocation - Cursor.ScreenLocation).SizeSquared() > FMath::Square(FacingMoveScreenDistance);

	//Once we have already become a facing move, we don't go back even if the distance falls below the threshold.
	bWantsFacingMove |= Type != EMoveCommandType::Location;


	EMoveCommandType RangeType = EMoveCommandType::Location;

	if (bWantsFacingMove)
	{
		RangeType = EMoveCommandType::Facing;

		SetActorRotation(FRotator{ 0.0, FMath::RadiansToDegrees(CursorAngle), 0.0 });
	}

	SetMoveCommandType(RangeType);
}

EWorldCursorEventResult AMoveCommand::CursorHandler(const FWorldCursorEventParams& Params)
{
	switch (Params.Type)
	{
	case EWorldCursorEventType::Location:
	{
		if (Params.Input == IE_Pressed)
			InitialCursorScreenLocation = Params.Location.ScreenLocation;

		UpdateFromCursorLocation(Params.Location);

		return EWorldCursorEventResult::Captured;
	}
	case EWorldCursorEventType::Selection:
	{
		if (Params.Input == EInputEvent::IE_Pressed)
		{
			Params.Cursor->RemoveDelegate(Params.Handle);

			EndCommand(EEndCommandReason::Cancelled);
		}

		return EWorldCursorEventResult::Captured;
	}
	case EWorldCursorEventType::ContextAction:
	{
		if (Params.Input == EInputEvent::IE_Released)
		{
			Params.Cursor->RemoveDelegate(Params.Handle);

			UpdateFromCursorLocation(Params.Location);

			BeginCommand();
		}

		return EWorldCursorEventResult::Captured;
	}
	default:
		return EWorldCursorEventResult::Ignore;
	}
}

void AMoveCommand::AssignMoveTarget(UCommandFollowerComponent* Follower)
{
	if (!IsValid(Follower))
		return;

	if (auto Pawn = Follower->GetOwner<APawn>())
	{
		auto& MoveTarget = MoveTargets.FindOrAdd(Follower);

		if (!IsValid(MoveTarget))
			MoveTarget = UPooledActorSubsystem::SpawnPooledActor<AMoveTarget>(this, MoveTargetClass, GetActorTransform());

		if (IsValid(MoveTarget))
		{
			MoveTarget->bEnableFocusing = Type == EMoveCommandType::Facing;

			if (MoveTarget->GetPawn() != Pawn)
				MoveTarget->SetPawn(Pawn);
		}
		else
			//Creating move target failed somehow
			MoveTargets.Remove(Follower);
	}
}

void AMoveCommand::ClearMoveTarget(UCommandFollowerComponent* Follower)
{
	if (!IsValid(Follower))
		return;

	auto MoveTarget = MoveTargets.Find(Follower);

	if (!MoveTarget)
		return;

	if (IsValid(*MoveTarget))
		UPooledActorSubsystem::DestroyPooledActor(*MoveTarget, EEndPlayReason::Destroyed);

	MoveTargets.Remove(Follower);
}


void AMoveCommand::OnMoveCommandTypeChanged_Implementation(EMoveCommandType OldType)
{
}



