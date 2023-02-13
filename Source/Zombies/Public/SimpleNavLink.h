//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavLinkHostInterface.h"

#include "SimpleNavLink.generated.h"

//Simplified replacement for ANavLinkProxy. Use this instead.
UCLASS()
class ZOMBIES_API ASimpleNavLink : public AActor//, public INavLinkHostInterface//, public INavRelevantInterface
{
	GENERATED_BODY()
	
public:	

	ASimpleNavLink();

protected:

	virtual void BeginPlay() override;

public:

	//FORCEINLINE virtual bool GetNavigationLinksClasses(TArray<TSubclassOf<UNavLinkDefinition> >& OutClasses) const override { return false; }
	//virtual bool GetNavigationLinksArray(TArray<FNavigationLink>& OutLink, TArray<FNavigationSegmentLink>& OutSegments) const override;

protected:

#if WITH_EDITORONLY_DATA

	//UPROPERTY()
	//class UNavLinkRenderingComponent* NavLinkRendererComponent;

	UPROPERTY()
	class UBillboardComponent* SpriteComponent;

#endif // WITH_EDITORONLY_DATA

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = NavLink, Meta = (AllowPrivateAccess = "True"))
	class UUnitNavLinkComponent* UnitNavLinkComponent;

	//ANavLinkProxy clearly suffers from code rot brought on by the move to the Actor/Component model in the transition from UE3 to UE4

	//UNavLinkCustomComponent is also not great, but we can still clean it up
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = NavLink, Meta = (AllowPrivateAccess = "True"))
	//class UNavLinkCustomComponent* NavLinkComponent;

	//void OnMoveReachedLink(UNavLinkCustomComponent* Component, UObject* PathFollowerObject, const FVector& GoalLocation);

};
