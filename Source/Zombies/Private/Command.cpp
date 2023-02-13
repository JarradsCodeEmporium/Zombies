//Copyright Jarrad Alexander 2022


#include "Command.h"
#include "PooledActorSubsystem.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameplaySelectorComponent.h"
#include "GameplaySelectableComponent.h"
#include "CommandFollowerComponent.h"
#include "CommanderComponent.h"
#include "WorldCursorComponent.h"

ACommand::ACommand()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ACommand::BeginPlay()
{
	Super::BeginPlay();
}

void ACommand::EndPlay(EEndPlayReason::Type Reason)
{
	FinalizeEndCommand(EEndCommandReason::Cancelled);

	Super::EndPlay(Reason);
}

void ACommand::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateVisualization();

	TickVisualization(DeltaTime);

}

void ACommand::BeginPlayPooled_Implementation()
{
	CurrentState = ECommandState::Pending;
}

void ACommand::EndPlayPooled_Implementation(EEndPlayReason::Type Reason)
{	
	FinalizeEndCommand(EEndCommandReason::Cancelled);
}

void ACommand::NotifyCommanderChanged(UCommanderComponent* InCommander)
{
	Commander = InCommander;
}

FBeginCommandParams ACommand::GetBeginCommandParams_Implementation()
{
	if (IsValid(Commander))
		return Commander->GetBeginCommandParams(this);

	return FBeginCommandParams{};
}

void ACommand::InitCommand_Implementation()
{
	UpdateVisualization();
}

bool ACommand::BeginCommand()
{
	if (CurrentState != ECommandState::Pending)
		return false;

	if (!IsValid(Commander))
		return false;
	
	CurrentState = ECommandState::Begun;

	auto Params = ReceiveBeginCommand();

	for (auto Follower : Params.Followers)
		if (IsValid(Follower) && Follower->GetCommander() == GetCommander())
			Follower->QueueCommand(this, Params.Behavior);

	auto Delegate = OnBeginCommand;

	Delegate.Broadcast(this);

	return true;
}

void ACommand::EndCommand(EEndCommandReason Reason)
{
	if (CurrentState == ECommandState::Ended)
		return;

	FinalizeEndCommand(Reason);

	UPooledActorSubsystem::DestroyPooledActor(this, EEndPlayReason::Destroyed);
}

void ACommand::NotifyCommandQueued(UCommandFollowerComponent* Follower)
{
	check(Follower);

	Followers.Add(Follower);
}

void ACommand::NotifyCommandUnqueued(UCommandFollowerComponent* Follower, EEndCommandReason Reason)
{
	check(Follower);

	Followers.Remove(Follower);

	if (Followers.Num() == 0)
		EndCommand(Reason);
}

void ACommand::BeginFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* PreviousCommand)
{
}

void ACommand::TickFollowingCommand_Implementation(UCommandFollowerComponent* Follower, float DeltaTime)
{
}

void ACommand::EndFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* NextCommand)
{
}

AAIController* ACommand::GetAIController(const UCommandFollowerComponent* Follower)
{
	if (!IsValid(Follower))
		return nullptr;

	auto Pawn = Cast<APawn>(Follower->GetOwner());

	if (!IsValid(Pawn))
		return nullptr;

	auto AIController = Pawn->GetController<AAIController>();

	return IsValid(AIController) ? AIController : nullptr;
}

UBlackboardComponent* ACommand::GetBlackboard(const UCommandFollowerComponent* Follower)
{
	auto AIController = GetAIController(Follower);

	if (!AIController)
		return nullptr;

	return AIController->GetBlackboardComponent();
}

UBehaviorTreeComponent* ACommand::GetBehaviorTree(const UCommandFollowerComponent* Follower)
{
	auto AIController = GetAIController(Follower);

	if (!AIController)
		return nullptr;

	return Cast<UBehaviorTreeComponent>(AIController->GetBrainComponent());
}

UWorldCursorComponent* ACommand::GetWorldCursorComponent() const
{
	if (!IsValid(Commander))
		return nullptr;

	auto CommanderOwner = Commander->GetOwner();

	if (!IsValid(CommanderOwner))
		return nullptr;

	return CommanderOwner->FindComponentByClass<UWorldCursorComponent>();
}

bool ACommand::GetCursorDirectionAndDistance(const FWorldCursorLocation& Cursor, FVector& OutDirection, double& OutDistance) const
{
	FVector Intersection;

	float IntersectionDistance;

	if (!UKismetMathLibrary::LinePlaneIntersection_OriginNormal(Cursor.WorldLocation, Cursor.WorldLocation + Cursor.WorldDirection * 100000.0, GetActorLocation(), FVector::UpVector, IntersectionDistance, Intersection))
		return false;

	(Intersection - GetActorLocation()).ToDirectionAndLength(OutDirection, OutDistance);

	return true;
}

bool ACommand::IsAnyFollowerSelected() const
{
	if (!IsValid(Commander))
		return false;

	auto Selector = Commander->GetOwner()->FindComponentByClass<UGameplaySelectorComponent>();

	if (!IsValid(Selector))
		return false;
	
	for (auto Follower : Followers)
	{
		if (!IsValid(Follower))
			continue;

		auto Selectable = Follower->GetOwner()->FindComponentByClass<UGameplaySelectableComponent>();

		if (!IsValid(Selectable))
			continue;

		if (!Selectable->IsSelectedBy(Selector))
			continue;

		return true;
	}

	return false;
}

void ACommand::BeginVisualization()
{
	if (bIsVisualizing)
		return;

	bIsVisualizing = true;

	ReceiveBeginVisualization();
}

void ACommand::TickVisualization(float DeltaTime)
{
	if (!bIsVisualizing)
		return;

	ReceiveTickVisualization(DeltaTime);
}

void ACommand::EndVisualization()
{
	if (!bIsVisualizing)
		return;

	bIsVisualizing = false;

	ReceiveEndVisualization();
}

bool ACommand::ShouldVisualize() const
{
	if (!bEnableVisualization)
		return false;

	if (!IsValid(Commander))
		return false;

	if (bOnlyVisibleWhenSelected && !IsAnyFollowerSelected())
		return false;

	//Passed all visibility conditions
	return true;
}

double ACommand::GetProximityFadeOpacity() const
{
	TOptional<float> Closest;

	for (auto Follower : Followers)
	{
		if (!IsValid(Follower))
			continue;
		
		//Forms a cylinder of ProximityFadeRadius and ProximityFadeHeight
		
		auto From = GetActorLocation();

		auto To = Follower->GetOwner()->GetActorLocation();

		auto XYDist = FMath::Max(0.0, FVector::Dist2D(From, To) - ProximityFadeRadius);

		auto HeightDist = FMath::Max(0.0, FMath::Abs(To.Z - From.Z) - ProximityFadeHeight);

		auto Dist = FMath::Max(XYDist, HeightDist);

		if (!Closest || *Closest > Dist)
			Closest = Dist;
	}

	if (!Closest)
		//No followers at all, use full opacity
		return 1.f;

	return FMath::Clamp(*Closest / ProximityFadeDistance, 0.0, 1.0);
}

FBeginCommandParams ACommand::ReceiveBeginCommand_Implementation()
{
	return GetBeginCommandParams();
}

void ACommand::ReceiveEndCommand_Implementation()
{
}

void ACommand::FinalizeEndCommand(EEndCommandReason Reason)
{
	if (CurrentState == ECommandState::Ended)
		return;

	ReceiveEndCommand();

	EndVisualization();

	CurrentState = ECommandState::Ended;

	auto Delegate = OnEndCommand;

	Delegate.Broadcast(this, Reason);

	auto TempFollowers = Followers;

	for (auto Follower : TempFollowers)
		if (IsValid(Follower))
			Follower->UnqueueCommand(this, Reason);

	NotifyCommanderChanged(nullptr);
}

void ACommand::ReceiveBeginVisualization_Implementation()
{
}

void ACommand::ReceiveTickVisualization_Implementation(float DeltaTime)
{
	//Derived classes will implement this behaviour
	//E.G a command for a grenade throw ability will reach into the ability and see its projectile properties, then use that to calculate and display the throwing arc
}

void ACommand::ReceiveEndVisualization_Implementation()
{
}

void ACommand::UpdateVisualization()
{
	bool bShouldVisualize = ShouldVisualize();

	if (bShouldVisualize != IsVisualizing())
	{
		if (bShouldVisualize)
			BeginVisualization();
		else
			EndVisualization();
	}
}








void AAICommand::BeginFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* PreviousCommand)
{
	Super::BeginFollowingCommand_Implementation(Follower, PreviousCommand);

	auto Controller = GetAIController(Follower);

	if (!Controller)
		return;

	//This does not reset the behavior tree if its the same, so we can just call without checking bResetBehaviorTree
	Controller->RunBehaviorTree(BehaviorTree);

	if (auto Blackboard = Controller->GetBlackboardComponent())
		Blackboard->SetValueAsObject(CommandKey, this);
}

void AAICommand::EndFollowingCommand_Implementation(UCommandFollowerComponent* Follower, ACommand* NextCommand)
{
	Super::EndFollowingCommand_Implementation(Follower, NextCommand);

	auto Controller = GetAIController(Follower);

	if (!Controller)
		return;

	auto Blackboard = Controller->GetBlackboardComponent();

	if (Blackboard->GetValueAsObject(CommandKey) != this)
		return;

	auto BehaviorTreeComponent = Cast<UBehaviorTreeComponent>(Controller->GetBrainComponent());

	if (bResetBehaviorTree || !IsValid(NextCommand))
	{
		BehaviorTreeComponent->StopTree();

		Blackboard->ClearValue(CommandKey);
	}
}
