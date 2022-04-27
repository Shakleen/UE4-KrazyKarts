// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UBoxComponent;

UCLASS(Abstract, Blueprintable)
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);

	void ApplyRotation(float DeltaTime);
	void UpdateLocationFromVelocity(float DeltaTime);
	FVector GetRollingResistance();
	FVector GetAirResistance();

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* Camera;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* BoxCollision;

	UPROPERTY(VisibleAnywhere, Category = "State variables")
	FVector VelocityMetersPerSecond;

	UPROPERTY(VisibleAnywhere, Category = "State variables")
	float Throttle = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "State variables")
	float SteeringThrow = 0.f;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float CarMassInKG = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float MaxDrivingForceInNeutons = 10000.f;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float MaxRotationInDegreesPerSecond = 90.f;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float DragCoefficient = 16.f;

	UPROPERTY(EditAnywhere, Category = "Configuration Variables")
	float RollingResistanceCoefficient = 0.015f;
};
