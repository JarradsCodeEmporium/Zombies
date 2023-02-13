//Copyright Jarrad Alexander 2022


#include "NonPlayerUnitController.h"
#include "Perception/AIPerceptionComponent.h"

ANonPlayerUnitController::ANonPlayerUnitController()
{
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>("PerceptionComponent");
}

void ANonPlayerUnitController::BeginPlay()
{
	Super::BeginPlay();

	//@todo: default to enemy team
	SetGenericTeamId(1);
}
