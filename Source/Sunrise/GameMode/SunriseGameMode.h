// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SunriseGameMode.generated.h"

namespace MatchState
{
	extern SUNRISE_API  const FName Cooldown; // Match duration has been reached. Display winner and begin cooldown timer.
}

/**
 * 
 */
UCLASS()
class SUNRISE_API ASunriseGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ASunriseGameMode();
	virtual void Tick(float DeltaSeconds) override;
	virtual void PlayerEliminated(class ASunriseCharacter* EliminatedCharacter, class ASunrisePlayerController* VictimController, ASunrisePlayerController* AttackerController);
	virtual void RequestRespawn(class ACharacter* EliminatedCharacter, AController* EliminatedController);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
private:
	float CountdownTime = 0.f;
public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }

	//float FurthestPoint = 0.f;
	//float DeltaDistance = 0.f;
};

