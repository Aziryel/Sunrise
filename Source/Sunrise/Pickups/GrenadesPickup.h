// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "GrenadesPickup.generated.h"

/**
 * 
 */
UCLASS()
class SUNRISE_API AGrenadesPickup : public APickup
{
	GENERATED_BODY()
protected:
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlapComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);
private:
	UPROPERTY(EditAnywhere)
	int32 GrenadesAmount = 2;
};
