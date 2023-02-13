//Copyright Jarrad Alexander 2022


#include "CommanderComponent.h"
#include "Command.h"
#include "CommandFollowerComponent.h"
#include "PooledActorSubsystem.h"

// Sets default values for this component's properties
UCommanderComponent::UCommanderComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCommanderComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UCommanderComponent::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

	EndAllCommands();

	RemoveAllFollowers();

}


// Called every frame
void UCommanderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UCommanderComponent::AddFollower(UCommandFollowerComponent* Follower)
{
	if (!IsValid(Follower))
		return;

	if (Followers.Contains(Follower))
		return;

	if (Follower->GetCommander())
		Follower->GetCommander()->RemoveFollower(Follower);

	NotifyFollowerAdded(Follower);

	Follower->NotifyCommanderChanged(this);

	OnFollowerAdded.Broadcast(this, Follower);
}

void UCommanderComponent::RemoveFollower(UCommandFollowerComponent* Follower)
{
	if (!IsValid(Follower))
		return;

	if (!Followers.Contains(Follower))
		return;

	NotifyFollowerRemoved(Follower);

	Follower->ClearCommandQueue();

	Follower->NotifyCommanderChanged(nullptr);

	OnFollowerRemoved.Broadcast(this, Follower);
}

void UCommanderComponent::RemoveAllFollowers()
{
	auto OldFollowers = MoveTemp(Followers);

	for (auto Follower : OldFollowers)
	{
		Follower->ClearCommandQueue();
		Follower->NotifyCommanderChanged(nullptr);
	}
}

ACommand* UCommanderComponent::SpawnCommand(const FTransform& Transform, TSubclassOf<ACommand> Class)
{
	auto Command = UPooledActorSubsystem::SpawnPooledActor<ACommand>(this, Class, Transform);

	if (!Command)
		return nullptr;

	Command->NotifyCommanderChanged(this);
	
	Command->InitCommand();

	return Command;
}

FBeginCommandParams UCommanderComponent::GetBeginCommandParams(ACommand* Command) const
{
	if (OnGetBeginCommandParams.IsBound())
		return OnGetBeginCommandParams.Execute(Command);

	return FBeginCommandParams{};
}

void UCommanderComponent::SetBeginCommandParamsDelegate(FGetBeginCommandParamsDelegate Delegate)
{
	OnGetBeginCommandParams = Delegate;
}

void UCommanderComponent::ClearBeginCommandParamsDelegate()
{
	OnGetBeginCommandParams.Clear();
}

void UCommanderComponent::EndAllCommands()
{
	auto OldCommands = MoveTemp(Commands);

	for (auto Command : OldCommands)
		Command->EndCommand(EEndCommandReason::Cancelled);
}

void UCommanderComponent::NotifyFollowerAdded(UCommandFollowerComponent* Follower)
{
	check(Follower);

	Followers.Add(Follower);
}

void UCommanderComponent::NotifyFollowerRemoved(UCommandFollowerComponent* Follower)
{
	check(Follower);

	Followers.Remove(Follower);
}

void UCommanderComponent::NotifyCommandAdded(ACommand* Command)
{
	check(Command);

	Commands.Add(Command);
}

void UCommanderComponent::NotifyCommandRemoved(ACommand* Command)
{
	check(Command);

	Commands.Remove(Command);
}
