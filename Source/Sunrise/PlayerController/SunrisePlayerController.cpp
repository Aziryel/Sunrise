// Fill out your copyright notice in the Description page of Project Settings.


#include "SunrisePlayerController.h"

#include "Components/ProgressBar.h"
#include "Sunrise/HUD/SunriseHUD.h"
#include "Sunrise/HUD/CharacterOverlay.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Sunrise/Character/SunriseCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Sunrise/GameMode/SunriseGameMode.h"
#include "Sunrise/GameState/SunriseGameState.h"
#include "Sunrise/HUD/Announcement.h"
#include "Sunrise/PlayerState/SunrisePlayerState.h"
#include "Sunrise/SunriseComponents/CombatComponent.h"


void ASunrisePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	SunriseHUD = Cast<ASunriseHUD>(GetHUD());
	ServerCheckMatchState();
}

void ASunrisePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASunrisePlayerController, MatchState);
}

void ASunrisePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();
	CheckTimeSync(DeltaSeconds);
	PollInit();
}

void ASunrisePlayerController::CheckTimeSync(float DeltaSeconds)
{
	TimeSyncRunningTime += DeltaSeconds;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncedFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ASunrisePlayerController::ServerCheckMatchState_Implementation()
{
	ASunriseGameMode* GameMode = Cast<ASunriseGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);
	}
}

void ASunrisePlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	
	if (SunriseHUD && MatchState == MatchState::WaitingToStart)
	{
		SunriseHUD->AddAnnouncement();
	}
}

void ASunrisePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ASunriseCharacter* SunriseCharacter = Cast<ASunriseCharacter>(InPawn);
	if (SunriseCharacter)
	{
		SetHUDHealth(SunriseCharacter->GetHealth(), SunriseCharacter->GetMaxHealth());
	}
}

void ASunrisePlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;

	bool bHUDValid = (SunriseHUD &&
		SunriseHUD->CharacterOverlay &&
		SunriseHUD->CharacterOverlay->HealthBar &&
		SunriseHUD->CharacterOverlay->HealthText);
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		SunriseHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		SunriseHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ASunrisePlayerController::SetHUDShield(float Shield, float MaxShield)
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;

	bool bHUDValid = (SunriseHUD &&
		SunriseHUD->CharacterOverlay &&
		SunriseHUD->CharacterOverlay->ShieldBar &&
		SunriseHUD->CharacterOverlay->ShieldText);
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		SunriseHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		SunriseHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ASunrisePlayerController::SetHUDScore(float Score)
{	
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;
	bool bHUDValid = (SunriseHUD &&
		SunriseHUD->CharacterOverlay &&
		SunriseHUD->CharacterOverlay->ScoreAmount);
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		SunriseHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ASunrisePlayerController::SetHUDDefeats(int32 Defeats)
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;
	bool bHUDValid = (SunriseHUD &&
		SunriseHUD->CharacterOverlay &&
		SunriseHUD->CharacterOverlay->DefeatsAmount);
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		SunriseHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ASunrisePlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;
	bool bHUDValid = (SunriseHUD &&
		SunriseHUD->CharacterOverlay &&
		SunriseHUD->CharacterOverlay->WeaponAmmoAmount);
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		SunriseHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ASunrisePlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;
	bool bHUDValid = (SunriseHUD &&
		SunriseHUD->CharacterOverlay &&
		SunriseHUD->CharacterOverlay->CarriedAmmoAmount);
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		SunriseHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ASunrisePlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;
	bool bHUDValid = (SunriseHUD &&
		SunriseHUD->CharacterOverlay &&
		SunriseHUD->CharacterOverlay->MatchCountdownText &&
		SunriseHUD->CharacterOverlay->EndingWarningTime);
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			SunriseHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		SunriseHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));

		if (CountdownTime <= SunriseHUD->CharacterOverlay->EndingWarningTime)
		{
			if (!this->SunriseHUD->CharacterOverlay->IsAnimationPlaying(this->SunriseHUD->CharacterOverlay->EndingWarning))
			{
				this->SunriseHUD->CharacterOverlay->PlayAnimation(SunriseHUD->CharacterOverlay->EndingWarning);
			}
		}
	}
}

void ASunrisePlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;
	bool bHUDValid = (SunriseHUD &&
		SunriseHUD->Announcement &&
		SunriseHUD->Announcement->WarmupTime);
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			SunriseHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		SunriseHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ASunrisePlayerController::SetHUDGrenades(int32 Grenades)
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;
	bool bHUDValid = (SunriseHUD &&
		SunriseHUD->CharacterOverlay &&
		SunriseHUD->CharacterOverlay->GrenadesText);
	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		SunriseHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ASunrisePlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	// Added to fix the issue with the LevelStartingTime starting after reaching the lobby instead of the actual Match
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	if (HasAuthority())
	{
		SunriseGameMode = SunriseGameMode == nullptr ? Cast<ASunriseGameMode>(UGameplayStatics::GetGameMode(this)) : SunriseGameMode;
		if (SunriseGameMode)
		{
			LevelStartingTime = SunriseGameMode->LevelStartingTime;
			SecondsLeft = FMath::CeilToInt(SunriseGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}
	
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
}

void ASunrisePlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (SunriseHUD && SunriseHUD->CharacterOverlay)
		{
			CharacterOverlay = SunriseHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);

				ASunriseCharacter* SunriseCharacter = Cast<ASunriseCharacter>(GetPawn());
				if (SunriseCharacter && SunriseCharacter->GetCombat())
				{
					if (bInitializeGrenades) SetHUDGrenades(HUDGrenades);
				}
			}
		}
	}
}

void ASunrisePlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ASunrisePlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ASunrisePlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	/* else */ return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ASunrisePlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
	
}

void ASunrisePlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ASunrisePlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ASunrisePlayerController::HandleMatchHasStarted()
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;
	if (SunriseHUD)
	{
		if (SunriseHUD->CharacterOverlay == nullptr) SunriseHUD->AddCharacterOverlay();
		if (SunriseHUD->Announcement)
		{
			SunriseHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ASunrisePlayerController::HandleCooldown()
{
	SunriseHUD = SunriseHUD == nullptr ? Cast<ASunriseHUD>(GetHUD()) : SunriseHUD;
	if (SunriseHUD)
	{
		SunriseHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = (SunriseHUD->Announcement &&
			SunriseHUD->Announcement->AnnouncementText &&
			SunriseHUD->Announcement->InfoText);
		
		if (bHUDValid)
		{
			SunriseHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts In:");
			SunriseHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ASunriseGameState* SunriseGameState = Cast<ASunriseGameState>(UGameplayStatics::GetGameState(this));
			ASunrisePlayerState* SunrisePlayerState = GetPlayerState<ASunrisePlayerState>();
			if (SunriseGameState && SunrisePlayerState)
			{
				TArray<ASunrisePlayerState*> TopPlayers = SunriseGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0) // No winners
				{
					InfoTextString = FString("There is no winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == SunrisePlayerState) // Only one players wins the match
				{
					InfoTextString = FString("You are the winner!!"); 
				}
				else if (TopPlayers.Num() == 1) // One player wins the match but is not the one being controlled
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1) // More than one player wins the match
				{
					InfoTextString = FString("Players tide for the win: \n");
					for (auto TiedPlayers : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayers->GetPlayerName()));
					}
				}
				
				SunriseHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
			
		}
	}
	ASunriseCharacter* SunriseCharacter = Cast<ASunriseCharacter>(GetPawn());
	if (SunriseCharacter && SunriseCharacter->GetCombat())
	{
		SunriseCharacter->bDisableGameplay = true; // Disable the movement
		SunriseCharacter->GetCombat()->FireButtonPressed(false); // Stop the Fire functionality
	}
}
