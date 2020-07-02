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
	CUSTOM_Jetpack = 1
};

UCLASS()
class UShooterCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	// For now is useless
	//virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

private:

	/** Variable used for ability cool down. Player could use one ability per time */
	bool bCanUseAbility = true;

	bool bUseTeleport = false;
	bool bUseJetpack = false;

	/** Time handler used for abilities */
	FTimerHandle AbilityTimerHandle;

	virtual float GetMaxSpeed() const override;
	
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector & OldLocation, const FVector & OldVelocity) override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	/** Manage the teleport mechanic's physics */
	void PhysTeleport(float deltaTime, int32 Iterations);

	/** Manage the jetpack mechanic's physics */
	void PhysJetpack(float deltaTime, int32 Iterations);
	
	/** This function set the variables for calling the teleport */
	void ExecTeleport(bool useRequest);

	/** This function set the variables for calling the jetpack */
	void ExecJetpack(bool useRequest);

	/** Check if character is touching ground */
	bool IsTouchingGround(float CheckDistance);

	/** Function used for enabling the use of ability */
	void EnableAbility();

public:
	
	/** Teleport distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport)
	float TeleportDistance = 1000.0f;

	/** Teleport cool down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport)
	float TeleportCoolDown = 1.0f;

	/** Jetpack speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport)
	float JetpackSpeed = 1.0f;

	/** Jetpack speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport)
	float MaxJetpackSpeed = 2000.0f;

	void SetTeleport(bool useRequest);

	void SetJetpack(bool useRequest);

	/**	This function is called from client to server */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void ServerTeleport(bool useRequest);

	/**	This function is called from client to server */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void ServerJetpack(bool useRequest);

	// GETTER

	/** Is it possible to use an ability? */
	UFUNCTION(BlueprintCallable)
	bool CanUseAbility();
};

