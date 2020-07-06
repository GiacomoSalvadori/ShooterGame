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
	WallRunElapsedTime = 0.0f;
	HoldJumpButtonElapsedTime = 0.0f;
}

void UShooterCharacterMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUseWallRun) {
		HoldJumpButtonElapsedTime += DeltaTime;
	}
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

	if (HoldJumpButtonElapsedTime >= HoldJumpButtonTime) {
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CUSTOM_WallRun);
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

		case CUSTOM_WallRun:
			PhysWallRun(deltaTime, Iterations);
			break;

		default:
			break;
	}
}

FHitResult UShooterCharacterMovement::IsTouchingAround() {
	
	AShooterCharacter* ShooterCharacterOwner = Cast<AShooterCharacter>(GetOwner());
	//FVector Center = ShooterCharacterOwner->GetMesh()->GetComponentLocation();
	FVector Center = GetOwner()->GetActorLocation();
	
	FCollisionQueryParams Params;
	// Ignore the character's pawn
	Params.AddIgnoredActor(GetOwner());

	FCollisionShape CollShape = FCollisionShape::MakeSphere(DistanceFromWall);

	FHitResult Hit;
	DrawDebugSphere(GetWorld(), Center, DistanceFromWall, 12, FColor::Orange, true);
	bool bHit = GetWorld()->SweepSingleByChannel(Hit, Center, Center, FQuat::Identity, ECC_Pawn, CollShape, Params);
	
	return Hit;
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

//////////////////////////////////////////////////////////////////////////
///// WALL RUNS
void UShooterCharacterMovement::PhysWallRun(float deltaTime, int32 Iterations) {
	
	WallRunElapsedTime += deltaTime;

	if (WallRunElapsedTime < WallRunTimeLength) {
		FHitResult Hit = IsTouchingAround();

		if (Hit.bBlockingHit) {
			FVector Adjusted = Velocity.GetSafeNormal() * deltaTime * WallRunSpeed;
			Adjusted.Z = 0.0f; // Set Z = 0 so the player will run only left or right on the wall
			
			SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
			return;
		}
	}

	// Here interrupt the wall run
	bUseWallRun = false;
	WallRunElapsedTime = 0.0f;
	HoldJumpButtonElapsedTime = 0.0f;
	EnableAbility();
	SetMovementMode(EMovementMode::MOVE_Falling);
}

void UShooterCharacterMovement::SetWallRun(bool useRequest) {
	if (!GetOwner()->HasAuthority() && GetPawnOwner()->IsLocallyControlled()) {
		ServerWallRun(useRequest);
	} else {
		ExecWallRun(useRequest);
	}
}

void UShooterCharacterMovement::ExecWallRun(bool useRequest) {
	bUseWallRun = useRequest;
	bCanUseAbility = !useRequest;

	if (!useRequest) {
		WallRunElapsedTime = 0.0f;
		HoldJumpButtonElapsedTime = 0.0f;
		SetMovementMode(EMovementMode::MOVE_Falling);
	}
}

bool UShooterCharacterMovement::ServerWallRun_Validate(bool useRequest) {
	return true;
}

void UShooterCharacterMovement::ServerWallRun_Implementation(bool useRequest) {
	ExecWallRun(useRequest);
}

//////////////////////////////////////////////////////////////////////////
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
	savedUseWallRun = false;
}

void FSavedMove_ShooterCharacter::SetMoveFor(ACharacter * Character, float InDeltaTime, FVector const & NewAccel, FNetworkPredictionData_Client_Character & ClientData) {
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
	UShooterCharacterMovement* CharacterMovement = Cast<UShooterCharacterMovement>(Character->GetCharacterMovement());
	if (CharacterMovement) {
		savedUseTeleport = CharacterMovement->bUseTeleport;
		savedUseJetpack = CharacterMovement->bUseJetpack;
		savedUseWallRun = CharacterMovement->bUseWallRun;
	}
}

void FSavedMove_ShooterCharacter::PrepMoveFor(ACharacter * Character) {
	Super::PrepMoveFor(Character);

	UShooterCharacterMovement* CharacterMovement = Cast<UShooterCharacterMovement>(Character->GetCharacterMovement());
	if (CharacterMovement) {
		CharacterMovement->ExecTeleport(savedUseTeleport);
		CharacterMovement->ExecJetpack(savedUseJetpack);
		CharacterMovement->ExecJetpack(savedUseWallRun);
	}
}