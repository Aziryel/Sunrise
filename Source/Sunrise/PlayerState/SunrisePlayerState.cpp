// Fill out your copyright notice in the Description page of Project Settings.


#include "SunrisePlayerState.h"

#include "Sunrise/Character/SunriseCharacter.h"
#include "Sunrise/PlayerController/SunrisePlayerController.h"
#include "Net/UnrealNetwork.h"

void ASunrisePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASunrisePlayerState, Defeats);
}

void ASunrisePlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount); // Set the Score using the new variable AddScore
	Character = Character == nullptr ? Cast<ASunriseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ASunrisePlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore()); // Get the Score variable by using the getter in the PlayerState class
		}
	}
}

void ASunrisePlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	Character = Character == nullptr ? Cast<ASunriseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ASunrisePlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(GetScore()); // Same as the other one
		}
	}
}

void ASunrisePlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = Character == nullptr ? Cast<ASunriseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ASunrisePlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ASunrisePlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<ASunriseCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ASunrisePlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

