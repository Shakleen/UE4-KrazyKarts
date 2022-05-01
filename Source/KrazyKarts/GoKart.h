// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKartMovementComponent.h"
#include "GoKart.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UBoxComponent;

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
	void ClearAcknowledgedMoves(const FGoKartMove& LastMove);

	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

public:
	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* Camera;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* BoxCollision;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UGoKartMovementComponent* MovementComponent;
	
	UFUNCTION()
	void OnRep_ServerState();

	UPROPERTY()
	FGoKartMove ServerMove;

	TArray<FGoKartMove> UnackMoves;
};
