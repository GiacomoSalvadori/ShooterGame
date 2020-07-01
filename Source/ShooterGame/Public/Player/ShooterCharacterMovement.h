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
	CUSTOM_Teleport    UMETA(DisplayName = "Teleport")
};

UCLASS()
class UShooterCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	// For now is useless
	//virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

private:

	/** Variable used fort the cool down */
	bool bCanTeleport = true;

	/** Time handler used for abilities*/
	FTimerHandle AbilityTimerHandle;

	virtual float GetMaxSpeed() const override;
	
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	void PhysTeleport(float deltaTime, int32 Iterations);

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector & OldLocation, const FVector & OldVelocity) override;

	/**
	This function set the variables for calling the teleport
	*/
	void ExecTeleport(bool useRequest);

	/** Function used for cooldown */
	void EnableTeleport();

public:
	bool bUseTeleport = false;

	/** Teleport distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport)
	float TeleportDistance = 1000.0f;

	/** Teleport cool down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Teleport)
	float TeleportCoolDown = 1.0f;

	void SetTeleport(bool useRequest);

	/**
	This function is called from client to server
	*/
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void ServerTeleport(bool useRequest);

};

