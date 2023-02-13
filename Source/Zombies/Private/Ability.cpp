//Copyright Jarrad Alexander 2022


#include "Ability.h"
#include "AbilityComponent.h"
#include "AbilityAction.h"
#include "GameplayTagAssetInterface.h"
#include "AIController.h"
#include "NavigationSystem.h"

// Sets default values
AAbility::AAbility()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAbility::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAbility::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

AAbility* AAbility::CreateAbility(UObject* WorldContextObject, TSubclassOf<AAbility> Class)
{
	if (!WorldContextObject || !Class.Get())
		return nullptr;

	auto World = WorldContextObject->GetWorld();

	if (!World)
		return nullptr;

	//@note: make this use a pooled actor system if necessary
	return World->SpawnActor<AAbility>(Class.Get());
}

void AAbility::SetAbilityComponent(UAbilityComponent* InAbilityComponent)
{
	if (IsValid(InAbilityComponent))
	{
		AbilityComponent = InAbilityComponent;

		//Become owned by the ability component actor, so that our components can use things like UPrimitiveComponent::bOnlyOwnerSee
		SetOwner(AbilityComponent->GetOwner());
	}
	else
	{
		AbilityComponent = nullptr;

		SetOwner(nullptr);
	}
}

void AAbility::DestroyAbility()
{
	CancelAllActions();

	SetAbilityComponent(nullptr);

	Destroy();
	
}


bool AAbility::CanBeginAbility_Implementation(const class UAbilityComponent* OwningAbilityComponent) const
{
	if (!OwningAbilityComponent)
		return false;

	FGameplayTagContainer OwnedTags;

	if (auto Interface = Cast<IGameplayTagAssetInterface>(OwningAbilityComponent->GetAvatarActor()))
		Interface->GetOwnedGameplayTags(OwnedTags);

	if (!OwnedTags.HasAll(RequiredTags))
		return false;

	if (OwnedTags.HasAny(BlockedTags))
		return false;

	return true;
}

void AAbility::OnBeginAbility_Implementation()
{
}

void AAbility::OnEndAbility_Implementation(EEndAbilityReason Reason)
{
}


void AAbility::EndAbility(EEndAbilityReason Reason)
{
	if (auto Component = GetAbilityComponent())
		Component->EndAbility(this, Reason);
}

void AAbility::RegisterAbilityAction(UAbilityAction* AbilityAction)
{
	if (!AbilityAction)
		return;

	AbilityActions.AddUnique(AbilityAction);
}

void AAbility::UnregisterAbilityAction(UAbilityAction* AbilityAction)
{
	if (!AbilityAction)
		return;

	AbilityActions.RemoveSingle(AbilityAction);
}

void AAbility::CancelActionsByName(FName ActionName)
{
	auto CurrentActions = AbilityActions;

	for (auto AbilityAction : CurrentActions)
		if (IsValid(AbilityAction) && AbilityAction->GetActionName() == ActionName)
			AbilityAction->Cancel();
}

void AAbility::CancelAllActions()
{
	auto CurrentActions = AbilityActions;

	for (auto AbilityAction : CurrentActions)
		if (IsValid(AbilityAction))
			AbilityAction->Cancel();
}



UTestAbilityAction* UTestAbilityAction::TestAsyncAbilityAction(UObject* WorldContextObject, FName ActionName, FVector MoveTarget)
{
	UTestAbilityAction* Result = UAbilityAction::NewAction<UTestAbilityAction>(WorldContextObject, ActionName);

	Result->MoveTarget = MoveTarget;

	return Result;
}

void UTestAbilityAction::Activate()
{

	auto Controller = GetAbility()->GetAbilityComponent()->GetController<AAIController>();

	//Controller->MoveToLocation(MoveTarget);

	FAIMoveRequest MoveRequest(MoveTarget);

	//MoveRequest.SetGoalLocation(MoveTarget);

	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAllowPartialPath(false);
	MoveRequest.SetProjectGoalLocation(true);
	MoveRequest.SetNavigationFilter(Controller->GetDefaultNavigationFilterClass());
	MoveRequest.SetReachTestIncludesAgentRadius(false);
	MoveRequest.SetCanStrafe(true);

	Controller->ReceiveMoveCompleted.AddDynamic(this, &UTestAbilityAction::ReceiveMoveComplete);

	auto Result = Controller->MoveTo(MoveRequest);

	MoveRequestID = Result.MoveId;
	

	UE_LOG(LogTemp, Warning, TEXT("Request Result: %s"), *StaticEnum<EPathFollowingRequestResult::Type>()->GetNameStringByValue(Result.Code));

	if (Result.Code != EPathFollowingRequestResult::RequestSuccessful)
		ReceiveMoveComplete(Result.MoveId, EPathFollowingResult::Invalid);

	//Controller->MoveToLocation(MoveTarget, )

	//if (!GetTimerManager())
	//	return;

	//FTimerDelegate Delegate;

	//Delegate.BindWeakLambda(this, [&]() 
	//	{
	//		if (!ShouldBroadcastDelegates())
	//			return;

	//		DelayedEcho.Broadcast(Message);
	//	});

	//GetTimerManager()->SetTimer(Handle, Delegate, 1.0, false);
}
void UTestAbilityAction::ReceiveMoveComplete(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (RequestID != MoveRequestID)
		return;

	if (!ShouldBroadcastDelegates())
		return;
	
	OnMoveComplete.Broadcast(Result);
	
	Cancel();
}


