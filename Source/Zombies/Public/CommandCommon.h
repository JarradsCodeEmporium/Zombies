//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "CommandCommon.generated.h"

class ACommand;
class UCommandFollowerComponent;
class UCommanderComponent;

UENUM(BlueprintType)
enum class ECommandQueueBehavior : uint8
{
	//The command clears the exiting queue and adds itself
	Replace,

	//The command adds itself to the end of the existing queue
	Append,

	//The command adds itself to the beginning of the existing queue
	Prepend,
};

USTRUCT(BlueprintType)
struct ZOMBIES_API FBeginCommandParams
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, Category = Command)
	TSet<UCommandFollowerComponent*> Followers;

	UPROPERTY(BlueprintReadWrite, Category = Command)
	ECommandQueueBehavior Behavior = ECommandQueueBehavior::Replace;
};

UENUM(BlueprintType)
enum class EEndCommandReason : uint8
{
	//Command completed successfully
	Completed,

	//Command was cancelled before completing
	Cancelled,

	//Command failed to complete
	Failed,
};

//Lifecycle of command actor itself
UENUM(BlueprintType)
enum class ECommandState : uint8
{
	//The command has not been confirmed, it is not added to any followers
	Pending,

	//The command has been confirmed, there is at least 1 follower with it in its queue
	Begun,

	//The command has fully ended, there are no followers with this command
	Ended,
};

//Lifecycle of command followers interacting with a command actor
UENUM(BlueprintType)
enum class ECommandFollowState : uint8
{
	//The command is not relevant to this follower
	None,

	//The command is queued but not being followed
	Queued,

	//The command is being followed by the command follower
	Following,

	//The command has satisfied its goal but does not need to end if there are no more commands
	Finishing,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCommandGenericDelegate, class ACommand*, Command);
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FBeginCommandParams, FGetBeginCommandParamsDelegate, class ACommand*, Command);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCommandEndedDelegate, class ACommand*, Command, EEndCommandReason, Reason);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCommandFollowerChangedDelegate, class UCommanderComponent*, Commander, class UCommandFollowerComponent*, Follower);