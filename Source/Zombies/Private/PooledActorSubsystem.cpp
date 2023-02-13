//Copyright Jarrad Alexander 2022


#include "PooledActorSubsystem.h"

static TAutoConsoleVariable<float> PooledActorExpiryTime
(
	TEXT("PooledActor.ExpiryTime"),
	300.f,
	TEXT("Time in real seconds before a pooled actor is evicted from pool due to inactivity")
);


static TAutoConsoleVariable<float> PooledActorFlushPeriod
(
	TEXT("PooledActor.FlushPeriod"),
	30.f,
	TEXT("Time between checks for inactive pooled actors")
);



AActor* UPooledActorSubsystem::StaticSpawnPooledActor(UObject* WorldContextObject, TSubclassOf<class AActor> Class, const FTransform& Transform, const FPooledActorSpawnParameters& SpawnParameters)
{
	if (!WorldContextObject)
		return nullptr;

	auto World = WorldContextObject->GetWorld();

	if (!World)
		return nullptr;

	auto Subsystem = World->GetSubsystem<UPooledActorSubsystem>();

	if (!Subsystem)
		return nullptr;

	return Subsystem->SpawnPooledActorInternal(Class, Transform, SpawnParameters);

}

void UPooledActorSubsystem::DestroyPooledActor(AActor* Actor, EEndPlayReason::Type Reason)
{
	if (!Actor)
		return;

	auto World = Actor->GetWorld();

	if (!World)
		return;

	auto Subsystem = World->GetSubsystem<UPooledActorSubsystem>();

	if (!Subsystem)
		return;

	Subsystem->DestroyPooledActorInternal(Actor, Reason);
}

AActor* UPooledActorSubsystem::SpawnPooledActorInternal(TSubclassOf<class AActor> Class, const FTransform& Transform, const FPooledActorSpawnParameters& SpawnParameters)
{
	
	if (!FlushHandle.IsValid())
		//Ideally this would be called in Initialize() but of course UE4 decides to do the most annoying thing possible and clears timers between that call and the start of the game.
		GetWorld()->GetTimerManager().SetTimer(FlushHandle, this, &UPooledActorSubsystem::Flush, PooledActorFlushPeriod.GetValueOnGameThread(), true, PooledActorFlushPeriod.GetValueOnGameThread());

	AActor* ActiveActor = nullptr;
	int32 Index = -1;

	//Try pull from inactive actors of the given type

	auto ExistingPool = PooledActors.Find(Class);

	if (ExistingPool && ExistingPool->Inactive.Num() > 0)
		while (!IsValid(ActiveActor) && ExistingPool->Inactive.Num() > 0)
		{
			auto Inactive = ExistingPool->Inactive.Pop();

			ActiveActor = Inactive.Actor;

			Index = Inactive.Index;
		}
	

	//No inactive actors of the given type, create a new one
	if (!ActiveActor)
	{
		FActorSpawnParameters SP;

		SP.Owner = SpawnParameters.Owner;

		SP.Instigator = SpawnParameters.Instigator;
		
		ActiveActor = GetWorld()->SpawnActor<AActor>(Class, Transform, SP);

		Index = GetNewIndex();
	}

	if (!ActiveActor)
		return nullptr;

	if (!Class->ImplementsInterface(UPooledActor::StaticClass()))
		return ActiveActor;

	auto& Pool = PooledActors.FindOrAdd(ActiveActor->GetClass());

	Pool.Active.Add(ActiveActor, FActivePooledActor{ Index });

	SpawnedFromPool(ActiveActor, Transform, SpawnParameters);
	
	return ActiveActor;
}

void UPooledActorSubsystem::DestroyPooledActorInternal(AActor* Actor, EEndPlayReason::Type Reason)
{
	if (!Actor)
		return;

	if (!Actor->Implements<UPooledActor>())
	{
		//Actor doesnt support pooling, so just destroy it normally.
		Actor->Destroy();
		return;
	}

	auto& Pool = PooledActors.FindOrAdd(Actor->GetClass());

	//Add to pooling if it supports pooling, but wasn't created by SpawnPooledActor()
	auto& ActiveActor = Pool.Active.FindOrAdd(Actor);

	if (ActiveActor.Index < 0)
	{
		//Check that we're not trying to double destroy an actor already in the inactive pool
		for (auto& Inactive : Pool.Inactive)
			if (Inactive.Actor == Actor)
			{
				UE_LOG(LogTemp, Warning, TEXT("Tried to destroy pooled actor while it was already destroyed."));

				return;
			}

		//All good, just add this new actor to the pool
		ActiveActor.Index = GetNewIndex();
	}

	DestroyedToPool(Actor, GetInactiveTransform(ActiveActor.Index), Reason);

	Pool.Inactive.Add(FInactivePooledActor{ Actor, ActiveActor.Index, GetWorld()->GetRealTimeSeconds() });
	
	Pool.Active.Remove(Actor);
}

void UPooledActorSubsystem::SpawnedFromPool(class AActor* Actor, const FTransform& Transform, const FPooledActorSpawnParameters& SpawnParameters)
{
	check(Actor);

	Actor->SetActorTransform(Transform, false, nullptr, ETeleportType::ResetPhysics);

	Actor->SetOwner(SpawnParameters.Owner);

	Actor->SetInstigator(SpawnParameters.Instigator);

	Actor->SetActorTickEnabled(true);

	//This breaks widget component ticking for some reason.
	//It will still tick but sometimes animations will get stuck
	//So just don't even bother, culling should take care of the heavy load anyway
	//Actor->SetActorHiddenInGame(false);

	Actor->SetActorEnableCollision(true);

	IPooledActor::Execute_BeginPlayPooled(Actor);
}

void UPooledActorSubsystem::DestroyedToPool(class AActor* Actor, const FTransform& InactiveTransform, EEndPlayReason::Type Reason)
{
	check(Actor);

	IPooledActor::Execute_EndPlayPooled(Actor, Reason);

	Actor->SetActorTransform(InactiveTransform, false, nullptr, ETeleportType::ResetPhysics);

	Actor->SetOwner(nullptr);

	Actor->SetInstigator(nullptr);

	Actor->SetActorTickEnabled(false);

	//Actor->SetActorHiddenInGame(true);
	
	Actor->SetActorEnableCollision(false);

	//@todo: should unbind all AActor delegates too,

}

void UPooledActorSubsystem::Flush()
{
	int32 Count = 0;

	for (auto& [Class, Actors] : PooledActors)
		for (int32 i = Actors.Inactive.Num() - 1; i >= 0; --i)
		{
			auto& InactiveActor = Actors.Inactive[i];

			if (IsValid(InactiveActor.Actor) && GetWorld()->GetRealTimeSeconds() < InactiveActor.RealTimeAtLastActive + PooledActorExpiryTime.GetValueOnGameThread())
				continue;

			//@todo: wait, did i forget to actually destroy flushed actors? is this necessary?
			if (IsValid(InactiveActor.Actor))
				InactiveActor.Actor->Destroy();

			UnusedIndices.Add(Actors.Inactive[i].Index);

			Actors.Inactive.RemoveAtSwap(i);

			++Count;
		}

	if (Count > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("PooledActorSubsystem flushed %d expired actors"), Count);
	}
}

FTransform UPooledActorSubsystem::GetInactiveTransform(int32 Index)
{
	
	FTransform Result{};

	constexpr double Offset = 480000.0;
	constexpr double Spacing = 1000.0;
	constexpr int32 Stride = Offset * 2.f / Spacing;

	//Line of actors in the sky near world boundary.
	//Is in sky to avoid KillZ plane, and since game is facing down near the origin, will never be seen even if some components don't get hidden by SetActorHiddenInGame().
	Result.SetLocation(FVector{ -Offset + Index * Spacing, -Offset + (Index / Stride) * Spacing, 480000.f });

	return Result;
}


int32 UPooledActorSubsystem::GetNewIndex()
{
	if (UnusedIndices.Num() > 0)
		return UnusedIndices.Pop();

	return NextPooledActorIndex++;
}
