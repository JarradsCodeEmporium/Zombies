//Copyright Jarrad Alexander 2022


#include "TargetedCommand.h"
#include "WorldCursorComponent.h"
#include "CommanderComponent.h"
#include "CommandFollowerComponent.h"

void ATargetedCommand::BeginPlayPooled_Implementation()
{
	Super::BeginPlayPooled_Implementation();
}

void ATargetedCommand::EndPlayPooled_Implementation(EEndPlayReason::Type Reason)
{
	Super::EndPlayPooled_Implementation(Reason);

}

void ATargetedCommand::InitCommand_Implementation()
{
	//Immediately start the command so that followers already at the location can quickly prepare the aiming.
	BeginCommand();
}

void ATargetedCommand::BeginFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* PreviousCommand)
{
	Super::BeginFollowingCommand_Implementation(Follower, PreviousCommand);
}

void ATargetedCommand::EndFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* NextCommand)
{
	Super::EndFollowingCommand_Implementation(Follower, NextCommand);
}

EWorldCursorEventResult ATargetedCommand::CursorHandler(const FWorldCursorEventParams& Params)
{
	return EWorldCursorEventResult::Captured;
}

FBeginCommandParams ATargetedCommand::ReceiveBeginCommand_Implementation()
{
	//Targeted commands are really only useful for a single follower, so pick the one closest to the world cursor on the commanders actor (if any)

	auto Params = Super::GetBeginCommandParams_Implementation();

	//Result params should contain 0 or 1 follower
	auto FollowerCandidates = MoveTemp(Params.Followers);

	FTimerHandle Handle;

	FTimerDelegate Delegate;

	Delegate.BindWeakLambda(this, 
		[&]() {
			EndCommand(EEndCommandReason::Completed);
		});

	GetWorld()->GetTimerManager().SetTimer(Handle, Delegate, 3.f, false);

	//Never fully replace the command queue, this command is designed to work with existing command queue
	if (Params.Behavior == ECommandQueueBehavior::Replace)
		Params.Behavior = ECommandQueueBehavior::Prepend;
	

	if (!IsValid(Commander))
		return Params;

	auto WorldCursorComponent = Commander->GetOwner()->FindComponentByClass<UWorldCursorComponent>();

	if (!IsValid(WorldCursorComponent))
		return Params;

	auto Cursor = WorldCursorComponent->GetCursorLocation();

	UCommandFollowerComponent* ClosestFollower = nullptr;

	TOptional<double> ClosestFollowerDot;
	TOptional<FVector> ClosestFollowerLocation;

	for (auto Follower : FollowerCandidates)
	{
		if (!IsValid(Follower))
			continue;

		auto FollowerLocation = Follower->GetOwner()->GetActorLocation();

		if (Params.Behavior == ECommandQueueBehavior::Append)
			//Use the location at the end of their queue
			FollowerLocation = Follower->GetLastCommandLocation();	

		auto Dot = (FollowerLocation - Cursor.WorldLocation).GetSafeNormal() | Cursor.WorldDirection;

		DrawDebugPoint(GetWorld(), FollowerLocation, 20.f, FColor::Purple, false, 3.f);

		if (!ClosestFollowerDot || *ClosestFollowerDot < Dot)
		{
			ClosestFollower = Follower;
			ClosestFollowerLocation = FollowerLocation;
			ClosestFollowerDot = Dot;
		}
	}

	if (!ClosestFollower)
		return Params;

	Params.Followers.Add(ClosestFollower);

	SetActorLocation(*ClosestFollowerLocation);

	DrawDebugPoint(GetWorld(), *ClosestFollowerLocation + FVector{0.0, 0.0, 20.0f}, 20.f, FColor::Magenta, false, 3.f);

	return Params;
}