// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementComponent.h"
#include "GoKart.h"
#include "GameFramework/GameStateBase.h"

UGoKartMovementComponent::UGoKartMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move)
{
	FVector Force = GetOwner()->GetActorForwardVector() * Move.Throttle * MaxDrivingForceInNeutons;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Accelartion = Force / CarMassInKG;
	VelocityMetersPerSecond = VelocityMetersPerSecond + Accelartion * Move.DeltaTime;

	ApplyRotation(Move);
	UpdateLocationFromVelocity(Move.DeltaTime);
}

FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	return Move;
}

void UGoKartMovementComponent::SetVelocity(FVector NewValue)
{
	VelocityMetersPerSecond = NewValue;
}

FVector UGoKartMovementComponent::GetVelocity()
{
	return VelocityMetersPerSecond;
}

void UGoKartMovementComponent::SetThrottle(float NewValue)
{
	Throttle = NewValue;
}

void UGoKartMovementComponent::SetSteeringThrow(float NewValue)
{
	SteeringThrow = NewValue;
}

FVector UGoKartMovementComponent::GetAirResistance()
{
	return -VelocityMetersPerSecond.SizeSquared() * DragCoefficient * VelocityMetersPerSecond.GetSafeNormal();
}

FVector UGoKartMovementComponent::GetRollingResistance()
{
	float AccelarationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = CarMassInKG * AccelarationDueToGravity;
	return -VelocityMetersPerSecond.GetSafeNormal() * NormalForce * RollingResistanceCoefficient;
}

void UGoKartMovementComponent::ApplyRotation(const FGoKartMove& Move)
{
	float DeltaLocation = FVector::DotProduct(VelocityMetersPerSecond, GetOwner()->GetActorForwardVector()) * Move.DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * Move.SteeringThrow;
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);
	GetOwner()->AddActorWorldRotation(RotationDelta);
	VelocityMetersPerSecond = RotationDelta.RotateVector(VelocityMetersPerSecond);
}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = VelocityMetersPerSecond * 100 * DeltaTime;

	FHitResult HitResult;
	GetOwner()->AddActorWorldOffset(Translation, true, &HitResult);

	if (HitResult.IsValidBlockingHit())
	{
		VelocityMetersPerSecond = FVector::ZeroVector;
	}
}

