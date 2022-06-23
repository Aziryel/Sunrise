// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "Sunrise/Character/SunriseCharacter.h"
#include "Sunrise/SunriseComponents/CombatComponent.h"
#include "Sunrise/Weapon/Weapon.h"

void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	ASunriseCharacter* SunriseCharacter = Cast<ASunriseCharacter>(OtherActor);
	if (SunriseCharacter && SunriseCharacter->GetEquippedWeapon() != nullptr && SunriseCharacter->GetEquippedWeapon()->GetWeaponType() == WeaponType)
	{
		UCombatComponent* Combat = SunriseCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
			Destroy();
		}
	}
}
