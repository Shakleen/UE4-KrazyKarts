// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GoKartMovementComponent.h"
#include "GoKartMoveReplicationComponent.generated.h"

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

struct FHermiteCubicSpline
{
	FVector StartLocation;
	FVector StartDerivative;
	FVector TargetLocation;
	FVector TargetDerivative;

	FVector InterpolateLocation(float Alpha) const
	{
		return FMath::CubicInterp(
			StartLocation,
			StartDerivative,
			TargetLocation,
			TargetDerivative,
			Alpha
		);
	}

	FVector InterpolateDerivative(float Alpha) const
	{
		return FMath::CubicInterpDerivative(
			StartLocation,
			StartDerivative,
			TargetLocation,
			TargetDerivative,
			Alpha
		);
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KRAZYKARTS_API UGoKartMoveReplicationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGoKartMoveReplicationComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ClearAcknowledgedMoves(const FGoKartMove& LastMove);
	void UpdateServerState(const FGoKartMove& Move);
	FHermiteCubicSpline CreateSpline() const;
	float GetVelocityToDerivative() const;
	void ClientTick(float DeltaTime);
	void InterpolateRotation(float Alpha);
	void InterpolateVelocity(const FHermiteCubicSpline& Spline, float Alpha);
	void InterpolateLocation(const FHermiteCubicSpline& Spline, float Alpha);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	UFUNCTION()
	void OnRep_ServerState();
	void AutonomousProxy_OnRep_ServerState();
	void SimulatedProxy_OnRep_ServerState();

	UPROPERTY()
	FGoKartMove ServerMove;

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	TArray<FGoKartMove> UnackMoves;

	float ClientTimeSinceUpdate = 0.f;
	float ClientTimeBetweenLastUpdates = 0.f;
	FTransform ClientStartTransform;
	FVector ClientStartVelocity;

	UGoKartMovementComponent* MovementComponent;
};
