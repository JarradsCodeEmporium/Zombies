//Copyright Jarrad Alexander 2022


#include "BaseUnitController.h"
#include "ZombiesGameModeBase.h"

void ABaseUnitController::BeginPlay()
{
	Super::BeginPlay();

	SetTeamTag(TeamTag);
}

void ABaseUnitController::SetTeamTag(FGameplayTag NewTeamTag)
{
	TeamTag = NewTeamTag;

	SetGenericTeamId(FGenericTeamId::NoTeam);

	auto GameMode = GetWorld()->GetAuthGameMode<AZombiesGameModeBase>();

	if (!GameMode)
		return;

	auto ID = GameMode->GetGenericTeamIdForTag(NewTeamTag);

	SetGenericTeamId(ID);
}
