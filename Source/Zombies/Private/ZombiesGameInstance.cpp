//Copyright Jarrad Alexander 2022


#include "ZombiesGameInstance.h"
#include "HighlightSettings.h"
#include "Camera/CameraComponent.h"
//#include "Engine/ObjectLibrary.h"
#include "AssetRegistryModule.h"
#include "Rendering/Texture2DResource.h"


void UZombiesGameInstance::Init()
{
	Super::Init();

	UpdateStencil();
}

void UZombiesGameInstance::UpdatePostProcessMaterial(UMaterialInstanceDynamic* DynamicMaterial)
{
	if (!DynamicMaterial)
		return;

	UpdateStencil();

	DynamicMaterial->SetTextureParameterValue("StencilMap", StencilMap);

	DynamicMaterial->SetScalarParameterValue("StencilOutlineCutoff", StencilOutlineCutoff);

	DynamicMaterial->SetScalarParameterValue("OutlineThickness", OutlineThickness);

	DynamicMaterial->SetScalarParameterValue("XRayBrightness", XRayBrightness);

}

void UZombiesGameInstance::UpdateAllPostProcessMaterials(AActor* Actor, UMaterialInterface* Material)
{
	if (!Actor)
		return;

	for (auto Component : Actor->GetComponents())
	{
		auto Camera = Cast<UCameraComponent>(Component);

		if (!Camera)
			continue;

		for (auto& [Weight, Object] : Camera->PostProcessSettings.WeightedBlendables.Array)
		{
			auto MaterialInterface = Cast<UMaterialInterface>(Object);
			
			if (!MaterialInterface)
				continue;

			UMaterialInstanceDynamic* DynamicMaterial = nullptr;

			if (MaterialInterface == Material)
			{
				DynamicMaterial = UMaterialInstanceDynamic::Create(MaterialInterface, Camera);
				Object = DynamicMaterial;
			}
			else
				DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);

			if (!DynamicMaterial)
				continue;
			
			UpdatePostProcessMaterial(DynamicMaterial);
		}
	}
}


void UZombiesGameInstance::UpdateStencil()
{
	//Encode all combinations of highlight type and color into the 256 values of custom stencil buffer

	//Sort non-outlined into the lowest values, and outlined into the highest values
	//This way our stencil sobol can do a simple comparison to a constant rather than having to read from an "outline weight" texture.

	TArray<FColor> PixelData;

	PixelData.Reserve(256);

	//Start at 1 because 0 is reserved for all EHighlightType::None states
	int32 CurrentStencilValue = 1;

	//Insert Single pixel for all EHighlightType::None states
	PixelData.Add(FColor::Black);

	//Highlight settings we need to be able to display
	TArray<UHighlightSettings*> HighlightSettings;


	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	TArray<FAssetData> HighlightSettingsAssetData;

	const UClass* Class = UStaticMesh::StaticClass();

	AssetRegistryModule.Get().GetAssetsByClass(UHighlightSettings::StaticClass()->GetFName(), HighlightSettingsAssetData, true);

	for (auto& AssetData : HighlightSettingsAssetData)
	{
		auto Highlight = Cast<UHighlightSettings>(AssetData.GetAsset());

		if (!Highlight || !Highlight->bEnablePostProcess)
			return;

		HighlightSettings.Add(Highlight);
	}

	//auto Library = UObjectLibrary::CreateLibrary(UHighlightSettings::StaticClass(), true, true);

	//Library->LoadAssetDataFromPath(TEXT("/Game/"));
	//
	//Library->GetObjects(HighlightSettings);
	
	//HighlightSettings.RemoveAll([](UHighlightSettings* HighlightSetting) -> bool { return !HighlightSetting || !HighlightSetting->bEnablePostProcess; });

	static constexpr int32 MaxHighlightSettings = (256 - 1) / 3;

	if (HighlightSettings.Num() > MaxHighlightSettings)
	{
		FMessageLog("AssetCheck").Error(FText::FromString(FString::Printf(
			TEXT("Exceeded maximum number of UHighlightSettings assets (%d > %d). Some highlights will not work."),
			HighlightSettings.Num(),
			MaxHighlightSettings
		)));

		HighlightSettings.SetNum(MaxHighlightSettings);
	}

	auto MaybeAppendColor = [&](FHighlightColor& HighlightColor, bool bAllowOutlines)
	{
		if (HighlightColor.bOutline != bAllowOutlines)
			return;

		HighlightColor.StencilValue = CurrentStencilValue++;

		PixelData.Add(HighlightColor.Color.ToFColor(true));
	};

	auto MaybeAppendColorSettings = [&](UHighlightSettings* HighlightSettings, bool bAllowOutlines)
	{
		MaybeAppendColor(HighlightSettings->XRay, bAllowOutlines);
		MaybeAppendColor(HighlightSettings->Selected, bAllowOutlines);
		MaybeAppendColor(HighlightSettings->Highlighted, bAllowOutlines);
	};

	for (auto HighlightSetting : HighlightSettings)
		//Only allow non-outline colors before the cutoff
		MaybeAppendColorSettings(HighlightSetting, false);

	StencilOutlineCutoff = CurrentStencilValue;

	for (auto& HighlightSetting : HighlightSettings)
		//Now add all the outlined colors after the cutoff
		MaybeAppendColorSettings(HighlightSetting, true);

	//Pad remaining unused pixels
	auto UnusedPixels = 256 - PixelData.Num();

	if (UnusedPixels > 0)
		PixelData.AddZeroed(UnusedPixels);

	UpdateStencilMapTexture(PixelData);
}


void UZombiesGameInstance::UpdateStencilMapTexture(TArray<FColor>& PixelData)
{

	if (!StencilMap)
	{
		StencilMap = UTexture2D::CreateTransient(HighlightTextureResolution, HighlightTextureResolution, HighlightPixelFormat);

		StencilMap->Filter = TextureFilter::TF_Nearest;

		StencilMap->AddressX = TextureAddress::TA_Wrap;

		StencilMap->AddressY = TextureAddress::TA_Wrap;

		StencilMap->UpdateResource();

		StencilMap->RefreshSamplerStates();
	}
	
	ENQUEUE_RENDER_COMMAND(UpdateTexture)([=, Resource = (FTexture2DResource*)StencilMap->GetResource(), Pixels = std::move(PixelData)](FRHICommandListImmediate& RHICmdList)
	{
		if (!Resource)
			return;

		if (!Resource->TextureRHI.IsValid())
			return;

		FUpdateTextureRegion2D Region;
		Region.DestX = 0;
		Region.DestY = 0;
		Region.SrcX = 0;
		Region.SrcY = 0;
		Region.Width = HighlightTextureResolution;
		Region.Height = HighlightTextureResolution;

		RHICmdList.UpdateTexture2D(Resource->GetTexture2DRHI(), 0, Region, Region.Width * HighlightPixelSizeInBytes, (uint8*)Pixels.GetData());
	});

	//Prevent texture resource from being deleted until the previous command that uses it is completed
	StencilMap->ReleaseFence.BeginFence();

}

