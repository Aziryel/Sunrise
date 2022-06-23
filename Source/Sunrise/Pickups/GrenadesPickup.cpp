// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadesPickup.h"

#include "Sunrise/Character/SunriseCharacter.h"
#include "Sunrise/SunriseComponents/CombatComponent.h"

void AGrenadesPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
	ASunriseCharacter* SunriseCharacter = Cast<ASunriseCharacter>(OtherActor);
	if (SunriseCharacter)
	{
		UCombatComponent* Combat = SunriseCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupGrenades(GrenadesAmount);
			Destroy();
		}
	}
}
