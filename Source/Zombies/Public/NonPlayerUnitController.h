//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "BaseUnitController.h"
#include "NonPlayerUnitController.generated.h"

/**
 * Base class for non player unit AI. Uses behaviour tree to instruct the units to move and perform abilities directly.
 */
UCLASS()
class ZOMBIES_API ANonPlayerUnitController : public ABaseUnitController
{
	GENERATED_BODY()
public:

	ANonPlayerUnitController();

protected:

	void BeginPlay() override;

public:

	



protected:


};
