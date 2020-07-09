// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

/**
 * Movement component meant for use with Pawns.
 */

#pragma once
#include "Engine/EngineTypes.h"
#include "ShooterCharacterMovement.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CUSTOM_Teleport = 0,
	CUSTOM_Jetpack = 1,
	CUSTOM_WallRun = 2
};

UCLASS()
class UShooterCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/** This function set the variables for calling the teleport */
	void ExecTeleport(bool useRequest);

	/** This function set the variables for calling the jetpack */
	void ExecJetpack(bool useRequest);

	/** This function set the variables for calling the jetpack */
	void ExecWallRun(bool useRequest);

private:

	/** The shooter character associated */
	AShooterCharacter* ShooterCharacterOwner;

	/** This number indicate the side where the hit occurs */
	float PointSide;

	/** Variable used for ability cool down. Player could use one ability per time */
	bool bCanUseAbility = true;

	// Jetpack variables	

	/** Indicate if fuel is over or not */
	bool bFuelOver = false;

	/** Used to control the jetpack curve */
	float JetpackElapsedTime;
		
	/** Actual fuel */
	float ActualFuel;
	
	/** Is the character running on a wall? */
	bool bRunningOnWall = false;

	/** Elapsed time for wall run */
	float WallRunElapsedTime;

	/** Elapsed time for jump button pressed */
	float HoldJumpButtonElapsedTime;

	/** Time handler used for abilities */
	FTimerHandle AbilityTimerHandle;

	virtual float GetMaxSpeed() const override;
	
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector & OldLocation, const FVector & OldVelocity) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	/** Manage the teleport mechanic's physics */
	void PhysTeleport(float deltaTime, int32 Iterations);

	/** Manage the jetpack mechanic's physics */
	void PhysJetpack(float deltaTime, int32 Iterations);

	/** Manage the wall run mechanic's physics */
	void PhysWallRun(float deltaTime, int32 Iterations);
	
	/** Recharge the jetpack fuel */
	void RecoverJetpackFuel(float DeltaTime);

	/** Check if character is touching in the specified direction */
	FHitResult IsTouchingAround();

	/** Function used for enabling the use of ability */
	void EnableAbility();

	/** Store the result of the hit side in HitSide */
	void CheckHitSide(FVector LinePoint1, FVector LinePoint2, FVector Point);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

public:
	
	/** Want to use teleport mechanic? */
	bool bUseTeleport = false;
	
	/** Teleport distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Teleport")
	float TeleportDistance = 1000.0f;

	/** Teleport cool down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Teleport")
	float TeleportCoolDown = 1.0f;

	/** Want to use jetpack mechanic? */
	bool bUseJetpack = false;

	/** Jetpack max speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Jetpack")
	float MaxJetpackSpeed = 2000.0f;

	/** Jetpack fuel amount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Jetpack")
	float JetpackFuel = 100.0f;

	/** Jetpack fuel consume per time (specified in JetpackTimeConsume) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Jetpack")
	float JetpackFuelConsume = 20.0f;

	/** Time to wait for consuming the JetpackFuelConsume amount*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Jetpack")
	float JetpackTimeConsume = 0.5f;

	/** Jetpack fuel recover per time (specified in JetpackTimeRecover) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Jetpack")
	float JetpackFuelRecover = 20.0f;

	/** Time to wait for consuming the JetpackFuelRecover amount*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Jetpack")
	float JetpackTimeRecover = 0.5f;
	
	/** Personalized curve for jetpack movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Jetpack")
	UCurveFloat* JetpackCurve;

	/** Want to use walk run? */
	bool bUseWallRun = false;

	/** Set time length for walk run, 3.0 seconds is the default (like Lucio's ability) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Wall Run")
	float WallRunTimeLength = 3.0f;

	/** Set time length for holding button and then start the wall run */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Wall Run")
	float HoldJumpButtonTime = 0.25f;

	/** Set time length for holding button and then start the wall run */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Wall Run")
	float WallRunSpeed = 5.0f;

	/** Set time length for holding button and then start the wall run */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Wall Run")
	float DistanceFromWall = 5.0f;

	/** Set the upper force after a wall run */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Wall Run")
	float LaunchUpperVelocity = 1000.0f;

	// FUNCTIONS
	void SetTeleport(bool useRequest);

	void SetJetpack(bool useRequest);

	void SetWallRun(bool useRequest);

	/**	This function is called from client to server */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void ServerTeleport(bool useRequest);

	/**	This function is called from client to server */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void ServerJetpack(bool useRequest);

	/**	This function is called from client to server */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void ServerWallRun(bool useRequest);

	UFUNCTION(BlueprintCallable, Category = "Jetpack")
	float RetrieveActualFuel();


	// GETTER

	/** Is it possible to use an ability? */
	UFUNCTION(BlueprintCallable)
	bool CanUseAbility();

	/** Retrieve the hit side of the last hit */
	float GetHitSide();
	
};

/** represents a saved move on the client that has been sent to the server and might need 
	to be played back. */
class FSavedMove_ShooterCharacter : public FSavedMove_Character
{

	friend class UShooterCharacterMovement;

public:
	typedef FSavedMove_Character Super;
	/** Clear saved move properties, so it can be re-used. */
	virtual void Clear() override;
	/** Called to set up this saved move (when initially created) to make a predictive correction. */
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData) override;
	
	/** Called before ClientUpdatePosition uses this SavedMove to make a predictive correction */
	virtual void PrepMoveFor(ACharacter* Character) override;

	bool savedUseTeleport = false;
	bool savedUseJetpack = false;
	bool savedUseWallRun = false;
};

/** Network data representation on the client. */
class FNetworkPredictionData_Client_Shooter : public FNetworkPredictionData_Client_Character
{
public:
	FNetworkPredictionData_Client_Shooter(const UCharacterMovementComponent& ClientMovement);
	typedef FNetworkPredictionData_Client_Character Super;
	virtual FSavedMovePtr AllocateNewMove() override;
};