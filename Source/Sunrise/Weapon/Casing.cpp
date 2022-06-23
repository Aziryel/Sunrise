// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

// Sets default values
ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;
	
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);

}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	FVector RandomRotation = GetActorForwardVector() + FVector(
		FMath::RandRange(CasingEjectImpulse_X.X, CasingEjectImpulse_X.Y),
		FMath::RandRange(CasingEjectImpulse_Y.X, CasingEjectImpulse_Y.Y),
		FMath::RandRange(CasingEjectImpulse_Z.X, CasingEjectImpulse_Z.Y)
	);

	float RandomImpulse = FMath::RandRange(CasingEjectImpulseSpeed.X, CasingEjectImpulseSpeed.Y);

	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit); // Bind the created function to the virtual function
	CasingMesh->AddImpulse(RandomRotation * RandomImpulse);

	SetLifeSpan(10.f);
	
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
	}

	CasingMesh->SetNotifyRigidBodyCollision(false); // Disable the sound after the first hit event to avoid loop-like sounds
}


