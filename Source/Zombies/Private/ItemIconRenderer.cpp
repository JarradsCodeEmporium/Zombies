//Copyright Jarrad Alexander 2022


#include "ItemIconRenderer.h"
#include "Components/SceneCaptureComponent2D.h" 
#include "Item.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Components/LightComponent.h"

// Sets default values
AItemIconRenderer::AItemIconRenderer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("SceneCapture");

	SceneCapture->SetupAttachment(GetRootComponent());

	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	SceneCapture->bCaptureEveryFrame = false;

	SceneCapture->bCaptureOnMovement = false;

	SceneCapture->bAlwaysPersistRenderingState = true;

}

// Called when the game starts or when spawned
void AItemIconRenderer::BeginPlay()
{
	Super::BeginPlay();

	for (auto Component : GetComponents())
		if (auto Light = Cast<ULightComponent>(Component))
			Light->SetLightingChannels(OverrideLightingChannels.bChannel0, OverrideLightingChannels.bChannel1, OverrideLightingChannels.bChannel2);
}

// Called every frame
void AItemIconRenderer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AItemIconRenderer::RenderIcon(AItem* Item, UTextureRenderTarget2D* RenderTarget)
{
	if (!IsValid(Item) || !RenderTarget)
		return;

	//Moves lighting rig with item in center
	SetActorTransform(Item->GetActorTransform());

	//Camera moves to take the photo
	SceneCapture->SetRelativeTransform(Item->GetItemIconTransform());

	SceneCapture->ShowOnlyActors = Item->GetIconVisibleActors();

	SceneCapture->ShowOnlyActors.Add(this);

	TMap<UPrimitiveComponent*, FLightingChannels> PreviousLightingChannels;

	for (auto Actor : SceneCapture->ShowOnlyActors)
	{
		if (!IsValid(Actor))
			continue;

		for (auto Component : Actor->GetComponents())
			if (auto Primitive = Cast<UPrimitiveComponent>(Component))
			{
				PreviousLightingChannels.Add(Primitive, Primitive->LightingChannels);
				Primitive->SetLightingChannels(OverrideLightingChannels.bChannel0, OverrideLightingChannels.bChannel1, OverrideLightingChannels.bChannel2);
			}
	}

	SceneCapture->TextureTarget = RenderTarget;

	SceneCapture->CaptureScene();

	for (auto [Primitive, LightingChannels] : PreviousLightingChannels)
		Primitive->SetLightingChannels(LightingChannels.bChannel0, LightingChannels.bChannel1, LightingChannels.bChannel2);

}

