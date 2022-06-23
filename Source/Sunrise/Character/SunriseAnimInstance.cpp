// Fill out your copyright notice in the Description page of Project Settings.


#include "SunriseAnimInstance.h"

#include "SunriseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sunrise/Weapon/Weapon.h"
#include "Sunrise/SunriseTypes/CombatState.h"

void USunriseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	SunriseCharacter = Cast<ASunriseCharacter>(TryGetPawnOwner());
}

void USunriseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (SunriseCharacter == nullptr)
	{
		SunriseCharacter = Cast<ASunriseCharacter>(TryGetPawnOwner());
	}
	if (SunriseCharacter == nullptr) return;

	FVector Velocity = SunriseCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = SunriseCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = SunriseCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = SunriseCharacter->IsWeaponEquipped(); // Called from the AnimInstanceBP to change the pose
	EquippedWeapon = SunriseCharacter->GetEquippedWeapon();
	bIsCrouched = SunriseCharacter->bIsCrouched; // bIsCrouched is replicated from the Character.h
	bAiming = SunriseCharacter->IsAiming();
	TurningInPlace = SunriseCharacter->GetTurningInPlace();
	bRotateRootBone = SunriseCharacter->ShouldRotateRootBone();
	bEliminated = SunriseCharacter->IsEliminated();

	// Offset Yaw for strafing
	FRotator AimRotation = SunriseCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(SunriseCharacter->GetVelocity());
	
	// Interpolate the animation for a smoother transition between two different animations
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 6.f);
	YawOffset = DeltaRotation.Yaw;
	// End of strafing

	// Character leaning
	CharacterRotatorLastFrame = CharacterRotation;
	CharacterRotation = SunriseCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotatorLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);
	// End of leaning

	AO_Yaw = SunriseCharacter->GetAO_Yaw();
	AO_Pitch = SunriseCharacter ->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && SunriseCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		SunriseCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (SunriseCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = SunriseCharacter->GetMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			
			FRotator OldRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - SunriseCharacter->GetHitTarget()));
			// Create a new vector to change the position of the Roll and Pitch to match the difference in rotations
			FRotator LookAtRotation = FRotator(OldRotation.Roll, OldRotation.Yaw, -OldRotation.Pitch);
			LookAtRotation.Roll += SunriseCharacter->RightHandRotationRoll;
			LookAtRotation.Yaw += SunriseCharacter->RightHandRotationYaw;
			LookAtRotation.Pitch += SunriseCharacter->RightHandRotationPitch;
			// To smoothly rotate the weapon towards the hit target when it's very close
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaSeconds, 30.f);
		}
	}

	bUseFABRIK = SunriseCharacter->GetCombatState() == ECombatState::ESC_Unoccupied;
	bUseAimOffsets = SunriseCharacter->GetCombatState() == ECombatState::ESC_Unoccupied && !SunriseCharacter->GetDisableGameplay();
	bTransformRightHand = SunriseCharacter->GetCombatState() == ECombatState::ESC_Unoccupied && !SunriseCharacter->GetDisableGameplay();
}
