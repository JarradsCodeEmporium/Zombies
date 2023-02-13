//Copyright Jarrad Alexander 2022


#include "PlayerUnitController.h"
#include "UtilityComponent.h"

APlayerUnitController::APlayerUnitController()
{

	UtilityComponent = CreateDefaultSubobject<UUtilityComponent>("UtilityComponent");

	//Default to using utility component as brain, since most of the thinking for player units will be done by the player.
	BrainComponent = UtilityComponent;
}

void APlayerUnitController::BeginPlay()
{
	Super::BeginPlay();

	UtilityComponent->StartLogic();
}
