//Copyright Jarrad Alexander 2022


#include "CommandFollowerComponent.h"
#include "Command.h"
#include "CommanderComponent.h"

UCommandFollowerComponent::UCommandFollowerComponent()
{

	PrimaryComponentTick.bCanEverTick = true;

}

void UCommandFollowerComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UCommandFollowerComponent::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

	ClearCommandQueue();

	if (Commander)
		Commander->RemoveFollower(this);
}

void UCommandFollowerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCommandQueue();

	if (CurrentCommand)
		CurrentCommand->TickFollowingCommand(this, DeltaTime);

}

void UCommandFollowerComponent::NotifyCommanderChanged(UCommanderComponent* InCommander)
{
	Commander = InCommander;
}

bool UCommandFollowerComponent::IsCommandQueueEnabled() const
{
	//might need to add this functionality later
	return true;
}

void UCommandFollowerComponent::QueueCommand(ACommand* Command, ECommandQueueBehavior Behavior)
{
	if (!IsValid(Command))
		return;

	bool bAlreadyQueued = CommandQueue.Contains(Command);

	switch (Behavior)
	{
	case ECommandQueueBehavior::Replace:
	{
		if (bAlreadyQueued)
		{
			auto OldCommandQueue = CommandQueue;

			for (auto OldCommand : OldCommandQueue)
				if (OldCommand != Command)
					UnqueueCommand(OldCommand, EEndCommandReason::Cancelled);
		}
		else
		{
			ClearCommandQueue();
			CommandQueue.Add(Command);
		}
		break;
	}
	case ECommandQueueBehavior::Append:
	{
		if (bAlreadyQueued)
			CommandQueue.RemoveSingle(Command);

		CommandQueue.Add(Command);
		break;
	}
	case ECommandQueueBehavior::Prepend:
	{
		if (bAlreadyQueued)
			CommandQueue.RemoveSingle(Command);

		CommandQueue.Insert(Command, 0);
		break;
	}
	}
	
	if (!bAlreadyQueued)
		Command->NotifyCommandQueued(this);
}

void UCommandFollowerComponent::UnqueueCommand(ACommand* Command, EEndCommandReason Reason)
{
	if (!IsValid(Command))
		return;

	if (CurrentCommand == Command)
		SetCurrentCommand(nullptr);

	NotifyCommandUnqueued(Command, Reason);

	Command->NotifyCommandUnqueued(this, Reason);
}

void UCommandFollowerComponent::ClearCommandQueue()
{
	SetCurrentCommand(nullptr);
	
	FinishingCommands.Empty();

	auto OldCommandQueue = MoveTemp(CommandQueue);

	for (auto Command : OldCommandQueue)
		if (IsValid(Command))
			Command->NotifyCommandUnqueued(this, EEndCommandReason::Cancelled);

}

void UCommandFollowerComponent::SetIsFinishingCommand(ACommand* Command, bool bIsFinishingCommand)
{
	if (!IsValid(Command))
		return;

	if (bIsFinishingCommand && CommandQueue.Contains(Command))
		FinishingCommands.Add(Command);
	else
		FinishingCommands.Remove(Command);
}

ACommand* UCommandFollowerComponent::GetHeadOfCommandQueue() const
{
	//Find first non finishing command
	for (auto Command : CommandQueue)
		if (IsValid(Command) && !IsFinishingCommand(Command))
			return Command;

	//No non finishing commands, use the current finishing command if there is one
	for (auto Command : CommandQueue)
		if (IsValid(Command))
			return Command;

	return nullptr;
}

ECommandFollowState UCommandFollowerComponent::GetCommandFollowState(ACommand* Command) const
{
	if (!IsValid(Command))
		return ECommandFollowState::None;

	if (CurrentCommand == Command)
	{
		if (FinishingCommands.Contains(Command))
			return ECommandFollowState::Finishing;
		else
			return ECommandFollowState::Following;
	}

	if (CommandQueue.Contains(Command))
		return ECommandFollowState::Queued;

	return ECommandFollowState::None;
}

FVector UCommandFollowerComponent::GetLastCommandLocation() const
{
	if (CommandQueue.Num() > 0 && IsValid(CommandQueue.Last()))
		return CommandQueue.Last()->GetActorLocation();
		
	return GetOwner()->GetActorLocation();
}

//void UCommandFollowerComponent::NotifyCommandQueued(ACommand* Command, ECommandQueueBehavior Behavior)
//{
//
//}

void UCommandFollowerComponent::NotifyCommandUnqueued(ACommand* Command, EEndCommandReason Reason)
{
	check(IsValid(Command));

	FinishingCommands.Remove(Command);

	CommandQueue.RemoveSingle(Command);
}

void UCommandFollowerComponent::UpdateCommandQueue()
{
	if (!IsCommandQueueEnabled())
	{
		SetCurrentCommand(nullptr);

		return;
	}

	CommandQueue.Remove(nullptr);

	auto NextCommand = GetHeadOfCommandQueue();

	if (NextCommand == CurrentCommand)
		return;

	//@todo: this should probably just unqueue all commands before the next command since unqueuing could cause another command to be queued or next command could jump over more than 1 command
	while (CommandQueue.Num() > 0 && CommandQueue[0] != NextCommand)
		UnqueueCommand(CommandQueue[0], EEndCommandReason::Completed);

	////Unqueue the current command if it is before the next command
	////Prepending a command to the queue can cause the head of the queue to change without unqueuing the current command 
	//for (int32 i = 0; i < CommandQueue.Num(); ++i)
	//{
	//	if (CommandQueue[i] == NextCommand)
	//		break;
	//	if (CommandQueue[i] == CurrentCommand)
	//	{
	//		UnqueueCommand(CurrentCommand, EEndCommandReason::Completed);
	//		break;
	//	}
	//}

	SetCurrentCommand(NextCommand);
}

void UCommandFollowerComponent::SetCurrentCommand(ACommand* Command)
{
	if (CurrentCommand == Command)
		return;

	auto OldCommand = CurrentCommand;

	CurrentCommand = Command;

	if (OldCommand)
		OldCommand->EndFollowingCommand(this, CurrentCommand);

	if (CurrentCommand)
		CurrentCommand->BeginFollowingCommand(this, OldCommand);
}

