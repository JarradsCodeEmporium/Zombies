//Copyright Jarrad Alexander 2022


#include "HighlightSettings.h"
#include "Camera/CameraComponent.h"
#include "Rendering/Texture2DResource.h"


FHighlightColor UHighlightSettings::GetHighlightColor(EHighlightType Type) const
{
	switch (Type)
	{
	case EHighlightType::None:
		return {};
	case EHighlightType::XRay:
		return XRay;
	case EHighlightType::Selected:
		return Selected;
	case EHighlightType::Highlighted:
		return Highlighted;
	default:
		return {};
	}
}


void UHighlightSettings::SetHighlighting(UPrimitiveComponent* Primitive, EHighlightType Type)
{
	if (!Primitive)
		return;

	auto HighlightColor = GetHighlightColor(Type);

	Primitive->SetRenderCustomDepth(HighlightColor.StencilValue > 0);

	Primitive->SetCustomDepthStencilValue(HighlightColor.StencilValue);

	Primitive->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_Default);

}
