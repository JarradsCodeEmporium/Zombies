//Copyright Jarrad Alexander 2022


#include "BaseUnit.h"
#include "AbilityComponent.h"
#include "UnitStatsComponent.h"
#include "Ability.h"
#include "NavLinkAbility.h"
#include "Components/CapsuleComponent.h"
#include "Command.h"
#include "FogOfWarVisionComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HighlightSettings.h"
#include "NativeTags.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameplaySelectableComponent.h"
#include "CommandFollowerComponent.h"
#include "CommanderComponent.h"
//#include "FogOfWarDisplayComponent.h"
#include "FocusComponent.h"
#include "FogOfWarSubsystem.h"

//When a TScriptInterface<> refers to an object that implements the interface purely in blueprint, the IInterface pointer inside it must always be null.
//This is because the interface address doesn't actually exist in the compiled program, it is purely a BP construct.
//Even if the object ptr is valid and points to an object that implements the interface, the interface ptr will be null.
//So if this function returns true, IMyInterface::Execute_MyFunc(Interface.GetObject()) syntax will work the same for c++ or BP interface functions.
//Otherwise it is an invalid interface.
template <typename T>
bool IsValidBPInterface(const TScriptInterface<T>& Interface)
{
	return Interface.GetObject() && Interface.GetObject()->Implements<T::UClassType>();
}

ABaseUnit::ABaseUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	FocusComponent = CreateDefaultSubobject<UFocusComponent>("FocusComponent");

	FocusComponent->SetupAttachment(GetRootComponent());

	FocusComponent->SetRelativeLocation(FVector{ 0.0, 0.0, BaseEyeHeight });

	SelectableComponent = CreateDefaultSubobject<UGameplaySelectableComponent>("SelectableComponent");

	SelectionIndicatorComponent = CreateDefaultSubobject<UStaticMeshComponent>("SelectionIndicatorComponent");

	SelectionIndicatorComponent->SetupAttachment(GetRootComponent());

	SelectionIndicatorComponent->SetCollisionProfileName("NoCollision");

	SelectionIndicatorComponent->SetRelativeLocation(FVector{ 0.0, 0.0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.0 });

	SelectionIndicatorComponent->SetHiddenInGame(true, true);

	SelectionIndicatorMaterialColorParameter = "RingColor";

	CommandFollowerComponent = CreateDefaultSubobject<UCommandFollowerComponent>("CommandFollowerComponent");

	AbilityComponent = CreateDefaultSubobject<UAbilityComponent>("AbilityComponent");

	UnitStatsComponent = CreateDefaultSubobject<UUnitStatsComponent>("UnitStatsComponent");

	StimuliComponent = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>("StimuliComponent");

	//Default movement style

	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	bUseControllerRotationYaw = false;

	//Default stats common to player, zombie, and npcs

	UnitStatsComponent->InitializeStat(Tag_Stats_Health, 100.f);
	UnitStatsComponent->InitializeStat(Tag_Stats_MaxHealth, 100.f);

	UnitStatsComponent->InitializeStat(Tag_Stats_MoveSpeed, 450.f);
}

void ABaseUnit::BeginPlay()
{
	if (auto Subsystem = GetWorld()->GetSubsystem<UFogOfWarSubsystem>())
		Subsystem->RegisterFogOfWarActor(this);

	InitSelection();

	InitFogOfWarVisibility();

	if (bAutoRegisterPlayerOwnership)
		if (auto PlayerController = GetWorld()->GetFirstPlayerController())
			if (auto Pawn = PlayerController->GetPawn())
				SetOwner(Pawn);
	
	RunDefaultBehaviorTree();

	Super::BeginPlay();
	
}

void ABaseUnit::EndPlay(EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

	if (auto Subsystem = GetWorld()->GetSubsystem<UFogOfWarSubsystem>())
		Subsystem->UnregisterFogOfWarActor(this);
}

void ABaseUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateIsDead();

	UpdateMoveSpeed();
}

void ABaseUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//Units do not receive input directly, instead the player tactics pawn issues commands which the player unit AI executes
}

void ABaseUnit::SetOwner(AActor* NewOwner)
{
	if (NewOwner == GetOwner())
		return;

	if (auto OldOwner = GetOwner(); IsValid(OldOwner))
	{
		if (auto Commander = OldOwner->FindComponentByClass<UCommanderComponent>())
			Commander->RemoveFollower(CommandFollowerComponent);
	}

	Super::SetOwner(NewOwner);

	if (auto CurrentOwner = GetOwner(); IsValid(CurrentOwner))
	{
		if (auto Commander = CurrentOwner->FindComponentByClass<UCommanderComponent>())
			Commander->AddFollower(CommandFollowerComponent);
	}
}

void ABaseUnit::GetOwnedGameplayTags(FGameplayTagContainer& Container) const
{
	if (AbilityComponent)
		AbilityComponent->GetOwnedGameplayTags(Container);

	if (UnitStatsComponent)
		UnitStatsComponent->GetOwnedGameplayTags(Container);
}

bool ABaseUnit::FollowNavLink_Implementation(const FFollowNavLinkParams& Params)
{
	auto Ability = AAbility::CreateAbility(GetWorld(), GetNavLinkAbilityClass(Params));

	if (!Ability)
		return false;

	if (Ability->Implements<UNavLinkAbility>())
		INavLinkAbility::Execute_SetFollowNavLinkParams(Ability, Params);

	bool bSuccess = AbilityComponent->BeginAbility(Ability);

	if (!bSuccess)
		Ability->DestroyAbility();

	return bSuccess;

}

bool ABaseUnit::IsDead_Implementation() const
{
	//Consider self dead if we have the dead status tag. This tag should be granted by the death ability.
	return HasMatchingGameplayTag(Tag_Status_Dead);
}

UGameplaySelectableComponent* ABaseUnit::GetSelectableComponent_Implementation() const
{
	return SelectableComponent;
}

void ABaseUnit::InitSelection_Implementation()
{
	SetHighlighting(EHighlightType::XRay);

	SelectableComponent->OnDisplayTypeChanged.AddDynamic(this, &ABaseUnit::DisplayTypeChangedHandler);
}

bool ABaseUnit::IsFogOfWarVisible_Implementation(const UFogOfWarVisionComponent* Component) const
{
	if (!Component)
		return false;

	return Component->CanSeePoint(GetActorLocation());
}

void ABaseUnit::SetFogOfWarVisionVisibility_Implementation(UFogOfWarVisionComponent* Component, bool bIsVisible)
{
	if (!IsValid(Component))
		return;

	//Just route directly to show/hide since there's only one player at the moment


	auto Pawn = Component->GetOwner()->GetOwner<APawn>();

	if (!IsValid(Pawn) || !Pawn->IsPlayerControlled())
		return;

	//Make sure nothing went wrong with ref count, would lead to permanent invisible/visible units
	check(FogOfWarVisionRefCount >= 0);

	if (bIsVisible)
	{
		if (FogOfWarVisionRefCount++ == 0)
			SetFogOfWarVisibility(true);
	}
	else
	{
		if (--FogOfWarVisionRefCount == 0)
			SetFogOfWarVisibility(false);
	}
}

void ABaseUnit::SetFogOfWarDisplayVisibility_Implementation(UFogOfWarDisplayComponent* Component, bool bIsVisible)
{
	
}

void ABaseUnit::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (auto Interface = GetController<IGenericTeamAgentInterface>())
		Interface->SetGenericTeamId(NewTeamID);
}

FGenericTeamId ABaseUnit::GetGenericTeamId() const
{
	if (auto Interface = GetController<IGenericTeamAgentInterface>())
		return Interface->GetGenericTeamId();

	return FGenericTeamId{};
}

void ABaseUnit::SetHighlighting_Implementation(EHighlightType HighlightType)
{
	if (!HighlightSettings)
		return;

	HighlightSettings->SetHighlighting(GetMesh(), HighlightType);

	if (HighlightType == EHighlightType::Selected || HighlightType == EHighlightType::Highlighted)
	{
		SelectionIndicatorComponent->SetHiddenInGame(false, true);

		HighlightSettings->SetHighlighting(SelectionIndicatorComponent, HighlightType);

		FVector Color = HighlightSettings->GetHighlightColor(HighlightType).Color;

		SelectionIndicatorComponent->SetVectorParameterValueOnMaterials(SelectionIndicatorMaterialColorParameter, Color);
	}
	else
	{
		SelectionIndicatorComponent->SetHiddenInGame(true, true);

	}
	//@todo: gather extra stuff that might need to be highlighted, such as current weapon

}

TSubclassOf<class AAbility> ABaseUnit::GetNavLinkAbilityClass(const FFollowNavLinkParams& Params) const
{
	if (!Params.LinkTag.IsValid())
		return DefaultNavLinkAbilityClass;

	if (auto Class = NavLinkAbilityClasses.Find(Params.LinkTag))
		return *Class;
	else
		return DefaultNavLinkAbilityClass;
}

void ABaseUnit::SetFogOfWarVisibility_Implementation(bool bIsVisible)
{
}

void ABaseUnit::InitFogOfWarVisibility_Implementation()
{
}


void ABaseUnit::RunDefaultBehaviorTree()
{
	if (!DefaultBehaviorTree)
		return;

	auto AIController = GetController<AAIController>();

	if (!AIController)
		return;

	//auto BehaviorTree = Cast<UBehaviorTreeComponent>(Controller->GetBrainComponent());

	//if (!BehaviorTree)
	//	return;

	//If it's already running, will just ignore this call 
	AIController->RunBehaviorTree(DefaultBehaviorTree);
}

void ABaseUnit::DisplayTypeChangedHandler(UGameplaySelectableComponent* Selectable, UGameplaySelectorComponent* Selector, EGameplaySelectionDisplayType NewDisplayType)
{
	if (!HighlightSettings)
		return;

	EHighlightType HighlightType = EHighlightType::XRay;

	switch (NewDisplayType)
	{
	case EGameplaySelectionDisplayType::None:
		HighlightType = EHighlightType::XRay;
		break;
	case EGameplaySelectionDisplayType::Selected:
		HighlightType = EHighlightType::Selected;
		break;
	case EGameplaySelectionDisplayType::Highlighted:
		HighlightType = EHighlightType::Highlighted;
		break;
	default:
		break;
	}

	SetHighlighting(HighlightType);

}

void ABaseUnit::UpdateIsDead()
{
	if (IUnitInterface::Execute_IsDead(this))
		return;

	auto Health = UnitStatsComponent->GetCurrentStats().StatValues.Find(Tag_Stats_Health);

	if (!Health)
		//Can't die if we don't have a health stat
		return;

	if (Health->ModifiedValue > 0.f)
		return;

	AbilityComponent->BeginAbilityByClass(DeathAbility);

}

void ABaseUnit::UpdateMoveSpeed()
{
	auto MoveSpeed = UnitStatsComponent->GetCurrentStats().StatValues.Find(Tag_Stats_MoveSpeed);

	if (!MoveSpeed)
		return;

	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed->ModifiedValue;
}

//bool ABaseUnit::ShouldFollowCommandQueue_Implementation() const
//{
//	//@todo: this should generally be false for AI and true for player units
//	return true;
//}
