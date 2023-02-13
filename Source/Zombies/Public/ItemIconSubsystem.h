//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ItemIconSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class ZOMBIES_API UItemIconSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:

	//Creates a render target for the given item
	UFUNCTION(BlueprintCallable, Category = Item)
	class UTextureRenderTarget2D* CreateIconRenderTarget(const class AItem* Item, UObject* Outer) const;

	UFUNCTION(BlueprintCallable, Category = Item)
	class AItemIconRenderer* GetRenderer(TSubclassOf<class AItemIconRenderer> RendererClass);

	UFUNCTION(BlueprintCallable, Category = Item)
	void RenderIcon(class AItem* Item, class UTextureRenderTarget2D* RenderTarget);

	UFUNCTION(BlueprintCallable, Category = Item)
	class UTextureRenderTarget2D* GetClassIcon(TSubclassOf<class AItem> ItemClass);

protected:

	UPROPERTY(Transient)
	TMap<TSubclassOf<class AItem>, class UTextureRenderTarget2D*> ClassIcons;

	UPROPERTY(Transient)
	TMap<TSubclassOf<class AItemIconRenderer>, class AItemIconRenderer*> Renderers;
};
