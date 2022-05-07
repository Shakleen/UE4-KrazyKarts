// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMoveReplicationComponent.h"
#include "Net/UnrealNetwork.h"

UGoKartMoveReplicationComponent::UGoKartMoveReplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}

void UGoKartMoveReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}

void UGoKartMoveReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MovementComponent) return;

	FGoKartMove Move = MovementComponent->GetLastMove();

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnackMoves.Add(Move);
		Server_SendMove(Move);
	}
	else if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(Move);
	}
	else if (GetOwnerRole() == ROLE_SimulatedProxy)
	{
		ClientTick(DeltaTime);
	}
}

void UGoKartMoveReplicationComponent::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	if (ClientTimeSinceUpdate < KINDA_SMALL_NUMBER) return;
	float Alpha = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;

	FHermiteCubicSpline Spline = CreateSpline();
	InterpolateLocation(Spline, Alpha);
	InterpolateVelocity(Spline, Alpha);
	InterpolateRotation(Alpha);
}

void UGoKartMoveReplicationComponent::InterpolateRotation(float Alpha)
{
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();
	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, Alpha);
	GetOwner()->SetActorRotation(NewRotation);
}

void UGoKartMoveReplicationComponent::InterpolateVelocity(const FHermiteCubicSpline& Spline, float Alpha)
{
	FVector NewDerivative = Spline.InterpolateDerivative(Alpha);
	FVector NewVelocity = NewDerivative / GetVelocityToDerivative();
	MovementComponent->SetVelocity(NewVelocity);
}

void UGoKartMoveReplicationComponent::InterpolateLocation(const FHermiteCubicSpline& Spline, float Alpha)
{
	FVector NewLocation = Spline.InterpolateLocation(Alpha);
	GetOwner()->SetActorLocation(NewLocation);
}

FHermiteCubicSpline UGoKartMoveReplicationComponent::CreateSpline() const
{
	FHermiteCubicSpline Spline;
	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	float VelocityToDerivative = GetVelocityToDerivative();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative;
	Spline.TargetDerivative = ServerState.VelocityMetersPerSecond * VelocityToDerivative;
	return Spline;
}

FORCEINLINE float UGoKartMoveReplicationComponent::GetVelocityToDerivative() const
{
	return 100 * ClientTimeBetweenLastUpdates;
}

void UGoKartMoveReplicationComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGoKartMoveReplicationComponent, ServerState);
}

void UGoKartMoveReplicationComponent::OnRep_ServerState()
{
	if (!MovementComponent) return;

	switch (GetOwnerRole())
	{
	case ROLE_AutonomousProxy:
		AutonomousProxy_OnRep_ServerState();
		break;
	case ROLE_SimulatedProxy:
		SimulatedProxy_OnRep_ServerState();
		break;
	default:
		break;
	}
}

void UGoKartMoveReplicationComponent::AutonomousProxy_OnRep_ServerState()
{
	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.VelocityMetersPerSecond);
	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnackMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UGoKartMoveReplicationComponent::SimulatedProxy_OnRep_ServerState()
{
	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0.f;
	ClientStartTransform = GetOwner()->GetActorTransform();
}

void UGoKartMoveReplicationComponent::ClearAcknowledgedMoves(const FGoKartMove& LastMove)
{
	UnackMoves.RemoveAll([LastMove](const FGoKartMove& Move) {
		return Move.Time < LastMove.Time;
	});
}

void UGoKartMoveReplicationComponent::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.VelocityMetersPerSecond = MovementComponent->GetVelocity();
}

void UGoKartMoveReplicationComponent::Server_SendMove_Implementation(FGoKartMove Move)
{
	if (!MovementComponent) return;

	MovementComponent->SimulateMove(Move);
	UpdateServerState(Move);
}

bool UGoKartMoveReplicationComponent::Server_SendMove_Validate(FGoKartMove Move)
{
	return true;
}

