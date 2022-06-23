// Fill out your copyright notice in the Description page of Project Settings.


#include "SunriseCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Sunrise/SunriseComponents/CombatComponent.h"
#include "Sunrise/SunriseComponents/BuffComponent.h"
#include "Sunrise/Weapon/Weapon.h"
#include "SunriseAnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Sunrise/Sunrise.h"
#include "Sunrise/GameMode/SunriseGameMode.h"
#include "Sunrise/PlayerController/SunrisePlayerController.h"
#include "Sunrise/PlayerState/SunrisePlayerState.h"
#include "Sunrise/Weapon/WeaponTypes.h"

// Sets default values
ASunriseCharacter::ASunriseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 500.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	TurningInPlace = ETurningInPlace::ETIP_NoTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}


void ASunriseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASunriseCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ASunriseCharacter, Health);
	DOREPLIFETIME(ASunriseCharacter, Shield);
	DOREPLIFETIME(ASunriseCharacter, bDisableGameplay);
	DOREPLIFETIME(ASunriseCharacter, bRecoverShield);
}


void ASunriseCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ASunriseCharacter::Elim()
{
	DropOrDestroyWeapons();
	MulticastElim();
	GetWorldTimerManager().SetTimer(
	ElimTimer,
	this,
	&ASunriseCharacter::ElimTargetFinished,
	ElimDelay
	);
}

void ASunriseCharacter::MulticastElim_Implementation()
{
	if (SunrisePlayerController)
	{
		SunrisePlayerController->SetHUDWeaponAmmo(0);
	}
	bEliminated = true;
	PlayElimMontage();

	// Start dissolve effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);

		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	//Disable character movement
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//Spawn elimination bot
	if (EliminationBotEffect)
	{
		FVector EliminationBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		EliminationBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EliminationBotEffect, EliminationBotSpawnPoint, GetActorRotation());
	}
	if (EliminationBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, EliminationBotSound, GetActorLocation());
	}
	bool bHideSniperScope = (IsLocallyControlled() &&
		Combat &&
		Combat->bAiming &&
		Combat->EquippedWeapon &&
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle);
	
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}
}

void ASunriseCharacter::ElimTargetFinished()
{
	ASunriseGameMode* SunriseGameMode = GetWorld()->GetAuthGameMode<ASunriseGameMode>();
	if (SunriseGameMode)
	{
		SunriseGameMode->RequestRespawn(this, Controller);
	}
}

void ASunriseCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr) return;
	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

void ASunriseCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon(Combat->EquippedWeapon);
		}
		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon(Combat->SecondaryWeapon);
		}
	}
}

void ASunriseCharacter::Destroyed()
{
	Super::Destroyed();

	if (EliminationBotComponent)
	{
		EliminationBotComponent->DestroyComponent();
	}

	ASunriseGameMode* SunriseGameMode = Cast<ASunriseGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = SunriseGameMode && SunriseGameMode->GetMatchState() != MatchState::InProgress;
	
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy(); // Destroy the weapon if the game has ended to avoid having the old weapon in a new map
	}
}

// Called when the game starts or when spawned
void ASunriseCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateHUDHealth();
	if (HasAuthority())
	{
		// Call the function ReceiveDamage after taking damage
		OnTakeAnyDamage.AddDynamic(this, &ASunriseCharacter::ReceiveDamage);
	}
	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

// Called every frame
void ASunriseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	
	HideCameraIfCharacterClose();
	PollInit();
	if (Shield < MaxShield)
	{
		RecoverShieldOverTime(DeltaTime);
	}
}

void ASunriseCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NoTurning;
		return;
	}
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

// Called to bind functionality to input
void ASunriseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASunriseCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ASunriseCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASunriseCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ASunriseCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ASunriseCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASunriseCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASunriseCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASunriseCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ASunriseCharacter::GrenadeButtonPressed);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASunriseCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASunriseCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ASunriseCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASunriseCharacter::LookUp);

}

void ASunriseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched);
		Buff->SetInitialJumpVelociy(GetCharacterMovement()->JumpZVelocity);
	}
}

void ASunriseCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleCombat") : FName("RifleNonCombat");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ASunriseCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ASunriseCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ASunriseCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ASunriseCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ASunriseCharacter::ReceiveDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType,
                                      AController* InstigatorController, AActor* DamageCauser)
{
	if (Health == 0.f || bEliminated) return; // Protect against receiving damage after being eliminated
	float DamageToHealth = Damage;
	if(bRecoverShield == true)
	{
		StartShieldTimer();
	}
	if (Shield > 0.f)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
			Shield = 0.f;
		}
	}
	
	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);
	
	UpdateHUDHealth();
	UpdateHUDShield();
	if (Health > 0.f && Shield <= 0.f)
	{
		PlayHitReactMontage();
	}

	if (Health == 0.f)
	{
		ASunriseGameMode* SunriseGameMode = GetWorld()->GetAuthGameMode<ASunriseGameMode>();
		if (SunriseGameMode)
		{
			SunrisePlayerController = SunrisePlayerController == nullptr ? Cast<ASunrisePlayerController>(Controller) : SunrisePlayerController;
			ASunrisePlayerController* AttackerController = Cast<ASunrisePlayerController>(InstigatorController);
			SunriseGameMode->PlayerEliminated(this, SunrisePlayerController, AttackerController);
		}
	}
	
	
}

void ASunriseCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	if (Controller != nullptr && abs(Value) > 0.01) // Value != 0.f
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ASunriseCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	if (Controller != nullptr && abs(Value) > 0.01)  // Value != 0.f
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ASunriseCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASunriseCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASunriseCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	// Only the server handles the equipping
	if (Combat)
	{
		// Call the RPC to let the server knows a client wants to equip the weapon
		ServerEquipButtonPressed();
	}
}


void ASunriseCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}
	}
}

void ASunriseCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ASunriseCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	if (Combat)
	{
		Combat->Reload();
	}
}

void ASunriseCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ASunriseCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ASunriseCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270 to 360) to [-90 to 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

float ASunriseCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ASunriseCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	
	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir) // When the character is standing still and not jumping
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		// Difference in rotation between the starting and current aim rotation
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NoTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) // When the character is running or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NoTurning;
	}

	CalculateAO_Pitch();
}

void ASunriseCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	bRotateRootBone = false;

	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NoTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NoTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NoTurning;
}

void ASunriseCharacter::Jump()
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ASunriseCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ASunriseCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return; // Easy disable this input using a bool
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ASunriseCharacter::GrenadeButtonPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void ASunriseCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		// Turn Right
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		// Turn Left
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NoTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NoTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ASunriseCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ASunriseCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (Health > 0.f && Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void ASunriseCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
}

void ASunriseCharacter::RecoverShieldOverTime(float DeltaTime)
{
	if (!bRecoverShield || IsEliminated() || bRecoverShield == false) return;
	
	const float ReplenishThisFrame = (RecoverAmount / RecoverTime) * DeltaTime;
	
	SetShield(FMath::Clamp(Shield + ReplenishThisFrame, 0.f, MaxShield));
	
	UpdateHUDShield();
}

void ASunriseCharacter::StartShieldTimer()
{
	bRecoverShield = false;
	
	GetWorldTimerManager().SetTimer(
	ShieldTimer,
	this,
	&ASunriseCharacter::ShieldTimerFinished,
	ShieldDelay
	);
}

void ASunriseCharacter::ShieldTimerFinished()
{
	bRecoverShield = true;
}

void ASunriseCharacter::UpdateHUDHealth()
{
	SunrisePlayerController = SunrisePlayerController == nullptr ? Cast<ASunrisePlayerController>(Controller) : SunrisePlayerController;
	if (SunrisePlayerController)
	{
		SunrisePlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ASunriseCharacter::UpdateHUDShield()
{
	SunrisePlayerController = SunrisePlayerController == nullptr ? Cast<ASunrisePlayerController>(Controller) : SunrisePlayerController;
	if (SunrisePlayerController)
	{
		SunrisePlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ASunriseCharacter::UpdateHUDAmmo()
{
	SunrisePlayerController = SunrisePlayerController == nullptr ? Cast<ASunrisePlayerController>(Controller) : SunrisePlayerController;
	if (SunrisePlayerController && Combat && Combat->EquippedWeapon)
	{
		SunrisePlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		SunrisePlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
	}
}

void ASunriseCharacter::SpawnDefaultWeapon()
{
	ASunriseGameMode* SunriseGameMode = Cast<ASunriseGameMode>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if (SunriseGameMode && World && !bEliminated && DefaultWeaponClass)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true;
		if (Combat)
		{
			Combat->EquipWeapon(StartingWeapon);
		}
	}
}

void ASunriseCharacter::PollInit()
{
	if (SunrisePlayerState == nullptr)
	{
		SunrisePlayerState = GetPlayerState<ASunrisePlayerState>();
		if (SunrisePlayerState)
		{
			SunrisePlayerState->AddToScore(0.f);
			SunrisePlayerState->AddToDefeats(0);
		}
	}
	if (SunrisePlayerController == nullptr)
	{
		SunrisePlayerController = SunrisePlayerController == nullptr ? Cast<ASunrisePlayerController>(Controller) : SunrisePlayerController;
		if (SunrisePlayerController)
		{
			SpawnDefaultWeapon();
			UpdateHUDAmmo();
			UpdateHUDHealth();
			UpdateHUDShield();
			if (Combat)
			{
				SunrisePlayerController->SetHUDGrenades(Combat->Grenades);
			}
		}
	}
}

void ASunriseCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ASunriseCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ASunriseCharacter::UpdateDissolveMaterial); // Bound the callback function
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ASunriseCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// Check if Overlapping Weapon is valid to hide the widget on the server
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	// Only enters this check only if the controlled pawn is on the server
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}


void ASunriseCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

bool ASunriseCharacter::IsWeaponEquipped()
{
	// Return true or false on weather we have or not the weapon equipped
	return (Combat && Combat->EquippedWeapon);
}

bool ASunriseCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* ASunriseCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector ASunriseCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState ASunriseCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ESC_MAX;
	return Combat->CombatState;
}
