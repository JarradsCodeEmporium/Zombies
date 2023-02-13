//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpatialHashMap.h"
#include "FogOfWarCommon.h"
#include "FogOfWarManager.generated.h"



UCLASS()
class ZOMBIES_API AFogOfWarManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFogOfWarManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	//Static function to find the fog of war manager for this game world
	UFUNCTION(BlueprintCallable, Category = FogOfWar, Meta = (WorldContextPin = "WorldContext"))
	static AFogOfWarManager* GetFogOfWarManager(UObject* WorldContext);

	//Create a render target for a player to store their discovered areas in this level
	//Render target is created as a subobject of the given Outer object, or the transient package if not specified.
	UFUNCTION(BlueprintCallable, Category = FogOfWar)
	class UTextureRenderTarget2D* CreateDiscoveredAreaRenderTarget(UObject* Outer) const;

	//Get the transform that maps world space to the canvas space of the discovered areas render target
	UFUNCTION(BlueprintCallable, Category = FogOfWar)
	FTransform GetDiscoveredAreaCanvasTransform() const;

	//Resolution of texture used by player to store their discovered areas
	//Should be power of 2.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	int32 DiscoveredAreasResolution = 128;

	//Meshes that block or overlap this channel will be considered to be fog of war occluders
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	TEnumAsByte<ECollisionChannel> StaticFogOfWarOccluderChannel;

	FORCEINLINE const auto& GetVisionComponents() const { return VisionComponents; }

	void RegisterVisionComponent(class UFogOfWarVisionComponent* Component);

	void UnregisterVisionComponent(class UFogOfWarVisionComponent* Component);

	//FORCEINLINE const auto& GetStaticOccluders() const { return StaticOccluders; }

	//FORCEINLINE const auto& GetFogOfWarActors() const { return FogOfWarActors; }

	//void RegisterFogOfWarActor(AActor* Actor);

	//void UnregisterFogOfWarActor(AActor* Actor);

	//void UpdateFogOfWarActors();

protected:

	//The area in world space that is covered by the discovered areas texture
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = FogOfWar, Meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* DiscoveredAreaBounds;

	//The active vision components in game world
	UPROPERTY(Transient, BlueprintReadOnly, Category = FogOfWar, Meta = (AllowPrivateAccess = "true"))
	TArray<class UFogOfWarVisionComponent*> VisionComponents;

	////Gathers all static meshes in the level that overlap the fog of war collision channel
	//void GatherStaticOccluders();

	//TSpatialHashMap<FBox2D, FFogOfWarOccluderInstance> StaticOccluders;

	//TSpatialHashMap<FVector2D, TWeakObjectPtr<AActor>> FogOfWarActors;
};
