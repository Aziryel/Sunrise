// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SunriseGameState.generated.h"

/**
 * 
 */
UCLASS()
class SUNRISE_API ASunriseGameState : public AGameState
{
	GENERATED_BODY()
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void UpdateTopScore(class ASunrisePlayerState* ScoringPlayer);

	UPROPERTY(Replicated)
	TArray<ASunrisePlayerState*> TopScoringPlayers;
private:

	float TopScore = 0.f;
};
