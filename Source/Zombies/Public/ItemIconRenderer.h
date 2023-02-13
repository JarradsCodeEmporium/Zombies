//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemIconRenderer.generated.h"

UCLASS()
class ZOMBIES_API AItemIconRenderer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItemIconRenderer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = Item)
	void RenderIcon(class AItem* Item, class UTextureRenderTarget2D* RenderTarget);

	//The lighting channels to use when rendering item icon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Item)
	FLightingChannels OverrideLightingChannels;

protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Item, Meta = (AllowPrivateAccess = "True"))
	class USceneCaptureComponent2D* SceneCapture;

};
