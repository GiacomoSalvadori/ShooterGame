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

// GETTER

bool UShooterCharacterMovement::CanUseAbility() {

	return bCanUseAbility;
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

	if (bUseJetpack) {
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CUSTOM_Jetpack);
	}
}

void UShooterCharacterMovement::PhysCustom(float deltaTime, int32 Iterations) {
	switch (CustomMovementMode) {
		case CUSTOM_Teleport:
			PhysTeleport(deltaTime, Iterations);
			break;

		case CUSTOM_Jetpack:
			PhysJetpack(deltaTime, Iterations);
			break;

		default:
			break;
	}
}

bool UShooterCharacterMovement::IsTouchingGround(float CheckDistance) {
	
	AShooterCharacter* CharacterOwner = Cast<AShooterCharacter>(GetOwner());
	FVector Start = CharacterOwner->GetMesh()->GetComponentLocation();
	FVector End = Start + (FVector::DownVector * FMath::Abs(CheckDistance));
	//FVector Start = GetOwner()->GetActorLocation();
	//FVector End = Start + GetOwner()->GetActorForwardVector() * 1000.0f;
	DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 3.0f);

	FCollisionQueryParams Params;
	// Ignore the character's pawn
	Params.AddIgnoredActor(GetOwner());

	FHitResult Hit;

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

	if (bHit) {
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow, "Actor "+ (Hit.Actor.Get()->GetName()));
		if (Hit.bBlockingHit) {
			return true;
		}
	}

	return false;
}

void UShooterCharacterMovement::EnableAbility() {
	bCanUseAbility = true;
}


//////////////////////////////////////////////////////////////////////////
///// TELEPORT

void UShooterCharacterMovement::PhysTeleport(float deltaTime, int32 Iterations) {

	FVector TargetLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * TeleportDistance;
	/** Use this because controls if the destination point is inside a volume*/
	GetOwner()->TeleportTo(TargetLocation, GetOwner()->GetActorRotation());
	
	bUseTeleport = false;
	/** This is important, without the character will continue to move forward */
	SetMovementMode(EMovementMode::MOVE_Walking);
	GetOwner()->GetWorldTimerManager().SetTimer(AbilityTimerHandle, this, &UShooterCharacterMovement::EnableAbility, TeleportCoolDown, false);
}

void UShooterCharacterMovement::SetTeleport(bool useRequest) {
	
	if (!GetOwner()->HasAuthority() && GetPawnOwner()->IsLocallyControlled()) {
		ServerTeleport(useRequest);
	} else {
		ExecTeleport(useRequest);
	}
}

void UShooterCharacterMovement::ExecTeleport(bool useRequest) {
	// Just set thit to true, the OnMovementUpdated() function will change the movement mode
	// The teleport is iplemented in the PhysTeleport(), called by PhysCustom()
	bUseTeleport = useRequest;
	bCanUseAbility = !useRequest;
}

bool UShooterCharacterMovement::ServerTeleport_Validate(bool useRequest) {
	return true;
}

void UShooterCharacterMovement::ServerTeleport_Implementation(bool useRequest) {
	if (bCanUseAbility) {
		ExecTeleport(useRequest);
	}
}

//////////////////////////////////////////////////////////////////////////
///// JETPACK

void UShooterCharacterMovement::PhysJetpack(float deltaTime, int32 Iterations) {
	float JetDir = FMath::Sign(GetGravityZ()) * -1; // Get gravity direction * -1
	bool isInAir = true;
	if (!bUseJetpack) {
		EnableAbility();
		JetDir *= -1.0f;
		isInAir = IsTouchingGround(1.0f);
	}
	
	if (!isInAir) {
		Velocity.Z = 0.0f;
		SetMovementMode(EMovementMode::MOVE_Walking);
	} else {
		float NewVelocityZ = Velocity.Z + (JetDir * JetpackSpeed * deltaTime);
		Velocity.Z = FMath::Clamp(NewVelocityZ, -MaxJetpackSpeed, MaxJetpackSpeed);
		PhysFalling(deltaTime, Iterations);
	}
}

void UShooterCharacterMovement::SetJetpack(bool useRequest) {
	if (!GetOwner()->HasAuthority() && GetPawnOwner()->IsLocallyControlled()) {
		ServerJetpack(useRequest);
	} else {
		ExecJetpack(useRequest);
	}
}

void UShooterCharacterMovement::ExecJetpack(bool useRequest) {
	// Just set this to true, the OnMovementUpdated() function will change the movement mode
	// The jetpack is iplemented in the JetpackTeleport(), called by PhysCustom()
	bUseJetpack = useRequest;
	if (useRequest) {
		bCanUseAbility = useRequest;
	}
}

bool UShooterCharacterMovement::ServerJetpack_Validate(bool useRequest) {
	return true;
}

void UShooterCharacterMovement::ServerJetpack_Implementation(bool useRequest) {
	if (bCanUseAbility) {
		ExecJetpack(useRequest);
	} else if(!useRequest) { // stop the jetpack emission
		ExecJetpack(useRequest);
	}
}
