// Fill out your copyright notice in the Description page of Project Settings.


#include "SunriseGameMode.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Sunrise/Character/SunriseCharacter.h"
#include "Sunrise/GameState/SunriseGameState.h"
#include "Sunrise/PlayerController/SunrisePlayerController.h"
#include "Sunrise/PlayerState/SunrisePlayerState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ASunriseGameMode::ASunriseGameMode()
{
	//FurthestPoint = 0.f;
	//DeltaDistance = 1.f;
	bDelayedStart = true;
}

void ASunriseGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ASunriseGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ASunriseGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	// Loop through all the player controllers in the world
	for (FConstPlayerControllerIterator  It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ASunrisePlayerController* SunrisePlayer = Cast<ASunrisePlayerController>(*It);
		if (SunrisePlayer)
		{
			SunrisePlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ASunriseGameMode::PlayerEliminated(ASunriseCharacter* EliminatedCharacter, ASunrisePlayerController* VictimController,
                                        ASunrisePlayerController* AttackerController)
{
	ASunrisePlayerState* AttackerPlayerState = AttackerController ? Cast<ASunrisePlayerState>(AttackerController->PlayerState) : nullptr;
	ASunrisePlayerState* VictimPlayerState = VictimController ? Cast<ASunrisePlayerState>(VictimController->PlayerState) : nullptr;

	ASunriseGameState* SunriseGameState = GetGameState<ASunriseGameState>();
	
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && SunriseGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
		SunriseGameState->UpdateTopScore(AttackerPlayerState); // Add the attacker player to the stop scoring list
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (EliminatedCharacter)
	{
		EliminatedCharacter->Elim();
	}
}

void ASunriseGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}
	if (EliminatedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[Selection]);

		/**
		TArray<AActor*> SunriseCharacters;
		UGameplayStatics::GetAllActorsOfClass(this, ACharacter::StaticClass(), SunriseCharacters);

		for (int32 i = 0; i < PlayerStarts.Num(); i++)
		{
			for (int32 j = 0; j < SunriseCharacters.Num(); j++)
			{
				DeltaDistance = (PlayerStarts[i]->GetActorLocation() - SunriseCharacters[j]->GetActorLocation()).Length();
				if (DeltaDistance > FurthestPoint)
				{
					FurthestPoint = DeltaDistance;
					Selection = i;
				}
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("Furthest Point: %f, PLayer Start: %d"), FurthestPoint, Selection);
		RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[Selection]);
		FurthestPoint = 0.f;
		DeltaDistance = 1.f;
		*/
	}
}


