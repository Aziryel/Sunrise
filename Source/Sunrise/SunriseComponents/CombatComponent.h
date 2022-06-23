// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sunrise/HUD/SunriseHUD.h"
#include "Sunrise/Weapon/WeaponTypes.h"
#include "Sunrise/SunriseTypes/CombatState.h"
#include "CombatComponent.generated.h"

class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SUNRISE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ASunriseCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(AWeapon* WeaponToEquip);
	void SwapWeapons();
	void Reload();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	
	void FireButtonPressed(bool bPressed);

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShoutgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinish();

	UFUNCTION(BlueprintCallable)
	void LaunchGrenade();

	UFUNCTION(Server, Reliable)
	void ServerLaunchGrenade(const FVector_NetQuantize& Target);

	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);
	void PickupGrenades(int32 GrenadesAmount);
	
protected:
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable) // Runs only on the Server but...
	void ServerSetAiming(bool bIsAiming);	// bIsAiming is replicated

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();
	void Fire();
	
	UFUNCTION(Server, Reliable) // Runs only on the server
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable) // Runs on the server and all the clients (without another replicated variable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void  TraceUnderCrosshairs(FVector& HitPosition);
	// Distance to start the linetrace ahead of the character to avoid clipping with itself
	float DistanceOffset = 75.f;

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();
	int32 AmountToReload();

	void ThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> GrenadeClass;
	
	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void AttachActorToBack(AActor* ActorToAttach);
	void UpdateCarriedAmmo();
	void PlayEquippedWeaponSound(AWeapon* WeaponToEquip);
	void ReloadEmptyWeapon();
	void ShowAttachedGrenade(bool bShowGrenade);
	void EquipPrimaryWeapon(AWeapon* WeaponToEquip);
	void EquipSecondaryWeapon(AWeapon* WeaponToEquip);

private:
	// Access the character without making use of Cast
	UPROPERTY()
	ASunriseCharacter* Character;
	// Access the player controller
	UPROPERTY()
	class ASunrisePlayerController* Controller;
	// Access the HUD
	UPROPERTY()
	ASunriseHUD* HUD;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_SecondaryWeapon)
	AWeapon* SecondaryWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	/**
	 * HUD and Crosshairs
	 */
	
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;
	
	// Factor to make the crosshair bigger/smaller
	UPROPERTY(EditAnywhere, Category = "Crosshairs' Factors")
	float CrosshairTargetFactor = 0.f;
	// Factor to change the speed at which the crosshair changes
	UPROPERTY(EditAnywhere, Category = "Crosshairs' Factors")
	float CrosshairInterpSpeed = 30.f;

	// Factor to make the crosshair bigger/smaller while falling
	UPROPERTY(EditAnywhere, Category = "Crosshairs' Factors")
	float CrosshairInAirTargetFactor = 2.25f;
	// Factor to change the speed at which the crosshair changes while falling
	UPROPERTY(EditAnywhere, Category = "Crosshairs' Factors")
	float CrosshairInAirInterpSpeed = 2.25f;

	// Factor to make the crosshair bigger/smaller while aiming
	UPROPERTY(EditAnywhere, Category = "Crosshairs' Factors")
	float CrosshairAimTargetFactor = 0.58f;
	// Factor to change the speed at which the crosshair while aiming
	UPROPERTY(EditAnywhere, Category = "Crosshairs' Factors")
	float CrosshairAimInterpSpeed = 30.f;
	
	// Factor to make the crosshair bigger/smaller while shooting
	UPROPERTY(EditAnywhere, Category = "Crosshairs' Factors")
	float CrosshairShootingTargetFactor = 0.f;
	// Factor to change the speed at which the crosshair changes while shooting
	UPROPERTY(EditAnywhere, Category = "Crosshairs' Factors")
	float CrosshairShootingInterpSpeed = 30.f;

	FVector HitTarget;

	FHUDPackage HUDPackage;

	/**
	 * Aiming and FOV
	 */

	// FOV while not aiming; set to the camera's FOV in BeginPlay
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	/*
	 * Automatic Fire
	 */

	FTimerHandle FireTimer;
	
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	// Carried ammo for the currently equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 MaxARAmmo = 80;

	UPROPERTY(EditAnywhere)
	int32 MaxRocketAmmo = 10;

	UPROPERTY(EditAnywhere)
	int32 MaxPistolAmmo = 40;

	UPROPERTY(EditAnywhere)
	int32 MaxSMGAmmo = 60;

	UPROPERTY(EditAnywhere)
	int32 MaxShotgunAmmo = 20;

	UPROPERTY(EditAnywhere)
	int32 MaxSniperAmmo = 15;

	UPROPERTY(EditAnywhere)
	int32 MaxGrenadeLauncherAmmo = 15;
	
	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;

	void InitializeCarriedAmmo();
	
	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ESC_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();
	int32 ShotgunShell = 1;
	FName ShotgunEnd = "ShotgunEnd";
	
	UPROPERTY(ReplicatedUsing = OnRep_Grenades, EditDefaultsOnly)
	int32 Grenades = 1;

	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;
	
	UFUNCTION()
	void OnRep_Grenades();

	void UpdateHUDGrenades();
	
public:	
	FORCEINLINE int32 GetGrenades() const { return Grenades; }
	bool ShouldSwapWeapons();
		
};
