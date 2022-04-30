// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UBoxComponent;

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float Time;
};

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	FGoKartMove LastMove;

	UPROPERTY()
	FVector VelocityMetersPerSecond;

	UPROPERTY()
	FTransform Transform;
};

UCLASS(Abstract, Blueprintable)
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	AGoKart();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void SimulateMove(const FGoKartMove& Move);
	void ClearAcknowledgedMoves(const FGoKartMove& LastMove);

	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	void ApplyRotation(const FGoKartMove& Move);
	void UpdateLocationFromVelocity(float DeltaTime);
	FVector GetRollingResistance();
	FVector GetAirResistance();
	FGoKartMove CreateMove(float DeltaTime);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* Camera;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* BoxCollision;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float CarMassInKG = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float MaxDrivingForceInNeutons = 10000.f;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float MinTurningRadius = 10.f;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float DragCoefficient = 16.f;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float RollingResistanceCoefficient = 0.015f;
	
	UFUNCTION()
	void OnRep_ServerState();

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	UPROPERTY()
	FGoKartMove ServerMove;

	float Throttle;
	float SteeringThrow;

	UPROPERTY()
	FVector VelocityMetersPerSecond;

	TArray<FGoKartMove> UnackMoves;
};
