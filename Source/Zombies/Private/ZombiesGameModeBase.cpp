// Copyright Epic Games, Inc. All Rights Reserved.


#include "ZombiesGameModeBase.h"



void AZombiesGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UpdateAttitudeTable();

	TWeakObjectPtr<AZombiesGameModeBase> WeakThis(this);

	FGenericTeamId::SetAttitudeSolver(
		[WeakThis](FGenericTeamId TeamA, FGenericTeamId TeamB)->ETeamAttitude::Type
		{
			if (WeakThis.IsValid())
				return WeakThis->GetTeamAttitude(TeamA, TeamB);

			return ETeamAttitude::Hostile;
		});
}

FGenericTeamId AZombiesGameModeBase::GetGenericTeamIdForTag(FGameplayTag Tag) const
{
	for (int32 i = 0; i < TeamTags.Num() && i != FGenericTeamId::NoTeam.GetId(); ++i)
	{
		if (Tag.MatchesTag(TeamTags[i].TeamTag))
			return FGenericTeamId{ (uint8)i };
	}

	return FGenericTeamId{};
}

void AZombiesGameModeBase::UpdateAttitudeTable()
{
	AttitudeTable.SetNumUninitialized(FMath::Square(TeamTags.Num()));

	AttitudeTableStride = TeamTags.Num();

	for (int32 TeamA = 0; TeamA < TeamTags.Num(); ++TeamA)
		for (int32 TeamB = 0; TeamB < TeamTags.Num(); ++TeamB)
		{
			//How does TeamA feel about TeamB?
			uint8& Attitude = AttitudeTable[TeamA + TeamB * TeamTags.Num()];

			Attitude = (uint8)ETeamAttitude::Neutral;

			if (TeamTags[TeamB].TeamTag.MatchesAny(TeamTags[TeamA].Friendly))
				Attitude = (uint8)ETeamAttitude::Friendly;

			//If tag is present in friendly and hostile, prioritise hostile attitude
			if (TeamTags[TeamB].TeamTag.MatchesAny(TeamTags[TeamA].Hostile))
				Attitude = (uint8)ETeamAttitude::Hostile;
		}

	//@todo: this is an asymmetric matrix but I don't know if UE supports that or if it requires symmetric attitudes.
}

ETeamAttitude::Type AZombiesGameModeBase::GetTeamAttitude_Implementation(FGenericTeamId TeamA, FGenericTeamId TeamB) const
{
	if (TeamA.GetId() < AttitudeTableStride && TeamB.GetId() < AttitudeTableStride)
		//Attitude is specified with tags, use attitude table
		return (ETeamAttitude::Type)AttitudeTable[TeamA.GetId() + TeamB.GetId() * AttitudeTableStride];

	if (TeamA == FGenericTeamId::NoTeam && TeamB == FGenericTeamId::NoTeam)
		//No teams == free for all
		return ETeamAttitude::Hostile;

	//Generically friendly to same team, hostile to others
	return TeamA == TeamB ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;

}
