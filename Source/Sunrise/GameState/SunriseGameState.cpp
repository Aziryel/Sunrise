// Fill out your copyright notice in the Description page of Project Settings.


#include "SunriseGameState.h"
#include "Net/UnrealNetwork.h"
#include "Sunrise/PlayerState/SunrisePlayerState.h"

void ASunriseGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASunriseGameState, TopScoringPlayers);
}

void ASunriseGameState::UpdateTopScore(ASunrisePlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0) // Add the top player to the list when the list is empty
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if	(ScoringPlayer->GetScore() == TopScore) // Add a player to the list if the score is the same as the top player
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore) // Remove all players when a new top scoring player arrives and add it to the list
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}
