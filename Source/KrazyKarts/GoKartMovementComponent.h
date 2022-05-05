// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.generated.h"

class AGoKart;

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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGoKartMovementComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SimulateMove(const FGoKartMove& Move);
	
	void SetVelocity(FVector NewValue);
	FVector GetVelocity() const;
	void SetThrottle(float NewValue);
	void SetSteeringThrow(float NewValue);
	FGoKartMove GetLastMove() const;

private:
	FGoKartMove CreateMove(float DeltaTime);
	void ApplyRotation(const FGoKartMove& Move);
	void UpdateLocationFromVelocity(float DeltaTime);
	FVector GetRollingResistance();
	FVector GetAirResistance();

private:
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

	float Throttle = 0.f;
	float SteeringThrow = 0.f;

	UPROPERTY()
	FVector VelocityMetersPerSecond;

	FGoKartMove LastMove;
};
