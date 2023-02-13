//Copyright Jarrad Alexander 2022


#include "SimpleNavLink.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UnitNavLinkComponent.h"
#include "NavLinkRenderingComponent.h"

// Sets default values
ASimpleNavLink::ASimpleNavLink()
{
	PrimaryActorTick.bCanEverTick = false;

	UnitNavLinkComponent = CreateDefaultSubobject<UUnitNavLinkComponent>(TEXT("UnitNavLinkComponent"));

	UnitNavLinkComponent->SetMobility(EComponentMobility::Static);

	SetRootComponent(UnitNavLinkComponent);

	SetHidden(true);

#if WITH_EDITORONLY_DATA

	//NavLinkRendererComponent = CreateEditorOnlyDefaultSubobject<UNavLinkRenderingComponent>(TEXT("NavLinkRenderer"));
	//NavLinkRendererComponent->SetupAttachment(RootComponent);

	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));

	if (!IsRunningCommandlet() && (SpriteComponent != NULL))
	{
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture;
			FName ID_Decals;
			FText NAME_Decals;
			FConstructorStatics()
				: SpriteTexture(TEXT("/Engine/EditorResources/AI/S_NavLink"))
				, ID_Decals(TEXT("Navigation"))
				, NAME_Decals(NSLOCTEXT("SpriteCategory", "Navigation", "Navigation"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		SpriteComponent->Sprite = ConstructorStatics.SpriteTexture.Get();
		SpriteComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->SetVisibleFlag(true);
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Decals;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Decals;
		SpriteComponent->SetupAttachment(RootComponent);
		SpriteComponent->SetAbsolute(false, false, true);
		SpriteComponent->bIsScreenSizeScaled = true;
	}
#endif



	SetActorEnableCollision(false);

}

// Called when the game starts or when spawned
void ASimpleNavLink::BeginPlay()
{
	Super::BeginPlay();
	
}

//bool ASimpleNavLink::GetNavigationLinksArray(TArray<FNavigationLink>& OutLink, TArray<FNavigationSegmentLink>& OutSegments) const
//{
//	//auto Modifier = NavLinkComponent->GetLinkModifier();
//
//	//Modifier.bUseSnapHeight = true;
//	//Modifier.SnapHeight = 1000.f;
//	//OutLink.Add(Modifier);
//
//	OutLink.Add(UnitNavLinkComponent->Link);
//
//	return true;
//}

//void ASimpleNavLink::OnMoveReachedLink(UNavLinkCustomComponent* Component, UObject* PathFollowerObject, const FVector& GoalLocation)
//{
//	UE_LOG(LogTemp, Warning, TEXT("%s reached nav link %s"), PathFollowerObject ? *PathFollowerObject->GetPathName() : TEXT("null"), *GetPathName());
//}

//
//// Called every frame
//void ASimpleNavLink::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//}

