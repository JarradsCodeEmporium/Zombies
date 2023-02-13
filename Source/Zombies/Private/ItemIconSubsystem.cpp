//Copyright Jarrad Alexander 2022


#include "ItemIconSubsystem.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Item.h"
#include "ItemIconRenderer.h"

UTextureRenderTarget2D* UItemIconSubsystem::CreateIconRenderTarget(const AItem* Item, UObject* Outer) const
{
	if (!Item)
		return nullptr;

	auto Size = Item->GetItemSize() * Item->GetItemIconResolution();

	if (Size.X < 16 || Size.Y < 16)
		return nullptr;
	
	UTextureRenderTarget2D* Texture = NewObject<UTextureRenderTarget2D>(Outer ? Outer : GetTransientPackage());

	check(Texture);

	Texture->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;

	Texture->ClearColor = FLinearColor::Red;// FLinearColor{ 0.f, 0.f, 0.f, 0.f };

	Texture->bAutoGenerateMips = true;

	Texture->InitAutoFormat(Size.X, Size.Y);

	Texture->UpdateResourceImmediate(true);

	return Texture;
}

AItemIconRenderer* UItemIconSubsystem::GetRenderer(TSubclassOf<class AItemIconRenderer> RendererClass)
{
	if (auto Renderer = Renderers.Find(RendererClass))
		if (IsValid(*Renderer))
			return *Renderer;

	if (!RendererClass.Get())
		return nullptr;

	auto Renderer = GetWorld()->SpawnActor<AItemIconRenderer>(RendererClass.Get());

	if (!Renderer)
		return nullptr;

	Renderers.Add(RendererClass, Renderer);

	return Renderer;
}

void UItemIconSubsystem::RenderIcon(class AItem* Item, class UTextureRenderTarget2D* RenderTarget)
{
	if (!RenderTarget || !IsValid(Item))
		return;

	auto Renderer = GetRenderer(Item->GetItemIconRendererClass());

	if (!Renderer)
		return;

	Renderer->SetActorHiddenInGame(false);

	Renderer->RenderIcon(Item, RenderTarget);

	Renderer->SetActorHiddenInGame(true);
}

UTextureRenderTarget2D* UItemIconSubsystem::GetClassIcon(TSubclassOf<class AItem> ItemClass)
{
	if (auto ClassIcon = ClassIcons.Find(ItemClass))
		if (IsValid(*ClassIcon))
			return *ClassIcon;

	if (!ItemClass.Get())
		return nullptr;

	auto Item = GetWorld()->SpawnActor<AItem>(ItemClass.Get());

	auto RenderTarget = CreateIconRenderTarget(Item, this);

	if (!RenderTarget)
		return nullptr;

	ClassIcons.Add(ItemClass, RenderTarget);

	RenderIcon(Item, RenderTarget);

	Item->Destroy();

	return RenderTarget;
}
