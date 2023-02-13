//Copyright Jarrad Alexander 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PrototypingMaze.generated.h"

USTRUCT(BlueprintType)
struct FPrototypingMazeCell
{
	GENERATED_BODY()
public:

	//Arbitrary ID used to partition the level into blobs
	UPROPERTY(BlueprintReadWrite, Category = Prototyping)
	int32 Label = -1;

	UPROPERTY(BlueprintReadWrite, Category = Prototyping)
	bool bXPDoor = false;

	UPROPERTY(BlueprintReadWrite, Category = Prototyping)
	bool bXNDoor = false;

	UPROPERTY(BlueprintReadWrite, Category = Prototyping)
	bool bYPDoor = false;

	UPROPERTY(BlueprintReadWrite, Category = Prototyping)
	bool bYNDoor = false;

	UPROPERTY(BlueprintReadWrite, Category = Prototyping)
	int32 EnemyCount = 0;

};

struct FPrototypingMazeVoronoiCell
{
	FVector2D Location;

	int32 Label = -1;

	TArray<FIntPoint> Cells;

	//Map other cell label to the pair of cells that are on the boundary
	TMap<int32, TArray<TPair<FIntPoint, FIntPoint>>> Adjacency;
};

//Placed in level to generate a simple maze like level for prototyping the feel of the game.
//Helps me understand how the game plays when players do not know the layout of the level
UCLASS()
class ZOMBIES_API APrototypingMaze : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APrototypingMaze();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FPrototypingMazeCell& GetCell(FVector Location);

	FPrototypingMazeCell& GetCell(FIntPoint Location);

	FVector GetCellWorldLocation(FIntPoint Location) const;

	UPROPERTY(EditAnywhere, Category = Prototyping)
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	FRandomStream RandomStream;

	UPROPERTY(EditAnywhere, Category = Prototyping)
	FIntPoint MazeSize { 32, 32 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	int32 NumVoronoiCells = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	float VoronoiJoinChance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	float DoorChance = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	UStaticMesh* WallMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	UMaterialInterface* WallMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	FVector WallMeshScale{ 1.0, 1.0, 1.0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	TSubclassOf<class ABaseUnit> EnemyUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	int32 NumEnemies = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Prototyping)
	int32 EnemyClusterSize = 3;

	void InitCells();

	void MakeVoronoiCells();

	void CalculateAdjacency();

	void JoinCells();

	void InsertDoors();

	void BuildWalls();

	void SpawnEnemies();

	TArray<FPrototypingMazeCell> Cells;

	TArray<FPrototypingMazeVoronoiCell> VoronoiCells;

	TSet<TPair<FIntPoint, FIntPoint>> Doors;

	void DrawDebugCells();

	FIntPoint ProduceRandomCell();

protected:

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Prototyping, Meta = (AllowPrivateAccess = "True"))
	//class UInstancedStaticMeshComponent* WallMeshes;
};
