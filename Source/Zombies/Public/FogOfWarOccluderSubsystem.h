//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FogOfWarCommon.h"
#include "FogOfWarOccluderSubsystem.generated.h"


/**
 * 
 */
UCLASS()
class ZOMBIES_API UFogOfWarOccluderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:

	//Gets the occluder mesh for the given static mesh.
	//The static mesh itself must have bAllowCPUAccess == true in order to create this data.
	TFogOfWarSharedPtr<FFogOfWarOccluderMesh> GetOccluderMesh(class UStaticMesh* StaticMesh);

protected:

	//Use soft object ptr to static mesh, so that we don't force it to be loaded if the game itself is not using it.
	//Use shared ptr to derived data so that game code can safely hold on to it without worrying about mesh load/unload
	TMap<TSoftObjectPtr<class UStaticMesh>, TFogOfWarSharedPtr<FFogOfWarOccluderMesh>> OccluderMeshes;
};
