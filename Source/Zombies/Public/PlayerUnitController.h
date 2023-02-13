//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "BaseUnitController.h"
#include "PlayerUnitController.generated.h"

/**
 * AI controller for player controlled units. Follows player commands.
 */
UCLASS()
class ZOMBIES_API APlayerUnitController : public ABaseUnitController
{
	GENERATED_BODY()
public:

	APlayerUnitController();

protected:

	virtual void BeginPlay() override;

public:

	FORCEINLINE auto GetUtilityComponent() const { return UtilityComponent; }

protected:

	//Utility AI component that implements low level behaviors like how and when to shoot and reload a gun
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Utility, Meta = (AllowPrivateAccess = "True"))
	class UUtilityComponent* UtilityComponent;

};
