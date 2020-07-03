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

void UShooterCharacterMovement::BeginPlay() {
	Super::BeginPlay();

	JetpackElapsedTime = 0.0f;
	JetpackElapsedTimeFuelConsume = 0.0f;
	ActualFuel = JetpackFuel;
}

void UShooterCharacterMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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

	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
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
	
	AShooterCharacter* ShooterCharacterOwner = Cast<AShooterCharacter>(GetOwner());
	FVector Start = ShooterCharacterOwner->GetMesh()->GetComponentLocation();
	FVector End = Start + (FVector::DownVector * FMath::Abs(CheckDistance));
	
	FCollisionQueryParams Params;
	// Ignore the character's pawn
	Params.AddIgnoredActor(GetOwner());

	FHitResult Hit;

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

	if (bHit) {
		if (Hit.bBlockingHit) { // the character touch something on the ground
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
	/** Just set the use variable to true, the OnMovementUpdated() function will change 
	the movement mode. The teleport is iplemented in the PhysTeleport(), called by PhysCustom() */
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
	
	float JetDir = GetGravityZ() * -1; // Get gravity direction * -1  FMath::Sign(GetGravityZ())
		
	JetpackElapsedTimeFuelConsume += deltaTime;
	if (JetpackElapsedTimeFuelConsume >= JetpackTimeConsume) {
		JetpackElapsedTimeFuelConsume = 0.0f;
		ActualFuel -= JetpackFuelConsume;
		if (ActualFuel <= 0.0f) {
			bFuelOver = true;
			bUseJetpack = false;
			SetMovementMode(EMovementMode::MOVE_Falling);
		}
	}

	JetpackElapsedTime += deltaTime * JetDir;
	float CurveValue = JetpackCurve->GetFloatValue(JetpackElapsedTime); // Evaluate curve

	Velocity.Z = CurveValue * MaxJetpackSpeed * deltaTime;
	AShooterCharacter* ShooterCharacterOwner = Cast<AShooterCharacter>(GetOwner());

	PhysFalling(deltaTime, Iterations); // Use the falling physics	
}

void UShooterCharacterMovement::SetJetpack(bool useRequest) {
	if (!GetOwner()->HasAuthority() && GetPawnOwner()->IsLocallyControlled()) {
		ServerJetpack(useRequest);
	} else {
		ExecJetpack(useRequest);
	}
}

void UShooterCharacterMovement::ExecJetpack(bool useRequest) {
	/** Just set the use variable to true, the OnMovementUpdated() function will change 
	    the movement mode. The jetpack is implemented in the JetpackTeleport(), called 
		by PhysCustom() */
	if (!bFuelOver) {  // Execute if there is fuel
		bUseJetpack = useRequest;
		if (useRequest) {
			bCanUseAbility = useRequest;
			// Clear the timer for fuel recovery
			GetOwner()->GetWorldTimerManager().ClearTimer(FuelRecoverTimerHandle);
		}
	}

	if (!useRequest) {
		bUseJetpack = useRequest;
		GetOwner()->GetWorldTimerManager().SetTimer(FuelRecoverTimerHandle, this, &UShooterCharacterMovement::RecoverJetpackFuel, JetpackTimeRecover, true);
		SetMovementMode(EMovementMode::MOVE_Falling);
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

void UShooterCharacterMovement::RecoverJetpackFuel() {
	float NewFuelAmount = ActualFuel + JetpackFuelRecover;
	ActualFuel = FMath::Clamp(NewFuelAmount, 0.0f, JetpackFuel);
	if(GetOwner()->HasAuthority())
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Black, TEXT("Recover fuel server"));
	else
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Black, TEXT("Recover fuel client"));

	if (ActualFuel == JetpackFuel) {
		GetOwner()->GetWorldTimerManager().ClearTimer(FuelRecoverTimerHandle);
		if (bFuelOver) {
			bFuelOver = false;
		}
	}
}

float UShooterCharacterMovement::RetrieveActualFuel() {

	return ActualFuel;
}

// Network area
FNetworkPredictionData_Client* UShooterCharacterMovement::GetPredictionData_Client() const {
	check(PawnOwner != NULL);

	if (!ClientPredictionData) {
		UShooterCharacterMovement* CharacterMovement = const_cast<UShooterCharacterMovement*>(this);

		CharacterMovement->ClientPredictionData = new FNetworkPredictionData_Client_Shooter(*this);
	}

	return ClientPredictionData;
}

FNetworkPredictionData_Client_Shooter::FNetworkPredictionData_Client_Shooter(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{

}

FSavedMovePtr FNetworkPredictionData_Client_Shooter::AllocateNewMove() {
	return FSavedMovePtr(new FSavedMove_ShooterCharacter());
}

void FSavedMove_ShooterCharacter::Clear() {
	Super::Clear();
	savedUseTeleport = false;
	savedUseJetpack = false;
}

void FSavedMove_ShooterCharacter::SetMoveFor(ACharacter * Character, float InDeltaTime, FVector const & NewAccel, FNetworkPredictionData_Client_Character & ClientData) {
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
	UShooterCharacterMovement* CharacterMovement = Cast<UShooterCharacterMovement>(Character->GetCharacterMovement());
	if (CharacterMovement) {
		savedUseTeleport = CharacterMovement->bUseTeleport;
		savedUseJetpack = CharacterMovement->bUseJetpack;
	}
}

void FSavedMove_ShooterCharacter::PrepMoveFor(ACharacter * Character) {
	Super::PrepMoveFor(Character);

	UShooterCharacterMovement* CharacterMovement = Cast<UShooterCharacterMovement>(Character->GetCharacterMovement());
	if (CharacterMovement) {
		GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Yellow, TEXT("Preparation!!!!"));
		CharacterMovement->ExecTeleport(savedUseTeleport);
		CharacterMovement->ExecJetpack(savedUseJetpack);
	}
}