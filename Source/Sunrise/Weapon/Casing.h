// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

UCLASS()
class SUNRISE_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();

protected:

	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;

	UPROPERTY(EditAnywhere)
	class USoundCue* ShellSound;

	// Direction of the shell ejection
	UPROPERTY(EditAnywhere)
	FVector2D CasingEjectImpulse_X { 0.0f, 0.2f };
	UPROPERTY(EditAnywhere)
	FVector2D CasingEjectImpulse_Y { -0.125f, -0.225f };
	UPROPERTY(EditAnywhere)
	FVector2D CasingEjectImpulse_Z { 0.0f, 0.1f };

	// Impulse of the shell ejection
	UPROPERTY(EditAnywhere)
	FVector2D CasingEjectImpulseSpeed { 2.0f, 5.0f };
};
