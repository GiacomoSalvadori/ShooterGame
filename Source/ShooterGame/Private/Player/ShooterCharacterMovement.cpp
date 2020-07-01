// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Player/ShooterCharacterMovement.h"

//----------------------------------------------------------------------//
// UPawnMovementComponent
//----------------------------------------------------------------------//
UShooterCharacterMovement::UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


float UShooterCharacterMovement::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const AShooterCharacter* ShooterCharacterOwner = Cast<AShooterCharacter>(PawnOwner);
	if (ShooterCharacterOwner)
	{
		if (ShooterCharacterOwner->IsTargeting())
		{
			MaxSpeed *= ShooterCharacterOwner->GetTargetingSpeedModifier();
		}
		if (ShooterCharacterOwner->IsRunning())
		{
			MaxSpeed *= ShooterCharacterOwner->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}

void UShooterCharacterMovement::OnMovementUpdated(float DeltaSeconds, const FVector & OldLocation, const FVector & OldVelocity) {
	
	if (bUseTeleport) {
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CUSTOM_Teleport);
	}
}

void UShooterCharacterMovement::PhysCustom(float deltaTime, int32 Iterations) {
	switch (CustomMovementMode) {
		case CUSTOM_Teleport:
			PhysTeleport(deltaTime, Iterations);

		default:
			break;
	}
}

void UShooterCharacterMovement::PhysTeleport(float deltaTime, int32 Iterations) {

	FVector TargetLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * TeleportDistance;
	/** Use this because this function controls if the destination point is inside a volume*/
	GetOwner()->TeleportTo(TargetLocation, GetOwner()->GetActorRotation());
	
	bUseTeleport = false;
	/** This is important or the character will continue to move forward */
	SetMovementMode(EMovementMode::MOVE_Walking);
}

void UShooterCharacterMovement::SetTeleport(bool useRequest) {
	ExecTeleport(useRequest);

	if (!GetOwner()->HasAuthority() && GetPawnOwner()->IsLocallyControlled()) {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, "server branch");
		ServerTeleport(useRequest);
		UE_LOG(LogTemp, Log, TEXT("server branch"));
	}
}

void UShooterCharacterMovement::ExecTeleport(bool useRequest) {
	// Just set thit to true, the OnMovementUpdated() function will change the movement mode
	// The teleport is iplemented in the PhysTeleport(), called by PhysCustom()
	bUseTeleport = useRequest;
}

bool UShooterCharacterMovement::ServerTeleport_Validate(bool useRequest) {
	return true;
}

void UShooterCharacterMovement::ServerTeleport_Implementation(bool useRequest) {
	ExecTeleport(useRequest);
	
}
