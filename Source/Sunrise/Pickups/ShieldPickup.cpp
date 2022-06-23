// Fill out your copyright notice in the Description page of Project Settings.


#include "ShieldPickup.h"
#include "Sunrise/Character/SunriseCharacter.h"
#include "Sunrise/SunriseComponents/BuffComponent.h"

void AShieldPickup::OnSphereOverlap(UPrimitiveComponent* OverlapComponent, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlapComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ASunriseCharacter* SunriseCharacter = Cast<ASunriseCharacter>(OtherActor);
	if (SunriseCharacter)
	{
		UBuffComponent* Buff = SunriseCharacter->GetBuff();
		if (Buff)
		{
			Buff->ReplenishShield(ShieldReplenishAmount, ShieldReplenishTime);
		}
	}
	Destroy();
}
