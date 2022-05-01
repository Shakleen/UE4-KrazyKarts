// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"

// Sets default values
AGoKart::AGoKart()
{
	PrimaryActorTick.bCanEverTick = true;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Collision"));
	SetRootComponent(BoxCollision);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skeletal Mesh"));
	Mesh->SetupAttachment(BoxCollision);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->TargetOffset = FVector(0.f, 0.f, 200.f);
	SpringArm->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 7.f;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;
	SpringArm->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->bUsePawnControlRotation = false;
	Camera->FieldOfView = 90.f;
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	bReplicates = true;
}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGoKart, ServerState);
}

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = CreateMove(DeltaTime);
		SimulateMove(Move);
		UnackMoves.Add(Move);
		Server_SendMove(Move);
	}
	else if (GetLocalRole() == ROLE_Authority && IsLocallyControlled())
	{
		FGoKartMove Move = CreateMove(DeltaTime);
		Server_SendMove(Move);
	}
	else if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		SimulateMove(ServerState.LastMove);
	}

	DrawDebugString(
		GetWorld(),
		FVector(0, 0, 100),
		UEnum::GetValueAsString(GetLocalRole()),
		this,
		FColor::Red,
		DeltaTime
	);
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	VelocityMetersPerSecond = ServerState.VelocityMetersPerSecond;
	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnackMoves)
	{
		SimulateMove(Move);
	}
}

void AGoKart::SimulateMove(const FGoKartMove& Move)
{
	FVector Force = GetActorForwardVector() * Move.Throttle * MaxDrivingForceInNeutons;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Accelartion = Force / CarMassInKG;
	VelocityMetersPerSecond = VelocityMetersPerSecond + Accelartion * Move.DeltaTime;

	ApplyRotation(Move);
	UpdateLocationFromVelocity(Move.DeltaTime);
}

FGoKartMove AGoKart::CreateMove(float DeltaTime)
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	return Move;
}

void AGoKart::ClearAcknowledgedMoves(const FGoKartMove& LastMove)
{
	UnackMoves.RemoveAll([LastMove](const FGoKartMove& Move) {
		return Move.Time < LastMove.Time;
	});
}

FVector AGoKart::GetAirResistance()
{
	return -VelocityMetersPerSecond.SizeSquared() * DragCoefficient * VelocityMetersPerSecond.GetSafeNormal();
}

FVector AGoKart::GetRollingResistance()
{
	float AccelarationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = CarMassInKG * AccelarationDueToGravity;
	return - VelocityMetersPerSecond.GetSafeNormal() * NormalForce * RollingResistanceCoefficient;
}

void AGoKart::ApplyRotation(const FGoKartMove& Move)
{
	float DeltaLocation = FVector::DotProduct(VelocityMetersPerSecond, GetActorForwardVector()) * Move.DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * Move.SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);
	AddActorWorldRotation(RotationDelta);
	VelocityMetersPerSecond = RotationDelta.RotateVector(VelocityMetersPerSecond);
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = VelocityMetersPerSecond * 100 * DeltaTime;

	FHitResult HitResult;
	AddActorWorldOffset(Translation, true, &HitResult);

	if (HitResult.IsValidBlockingHit())
	{
		VelocityMetersPerSecond = FVector::ZeroVector;
	}
}

void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::MoveForward(float AxisValue)
{
	Throttle = AxisValue;
}

void AGoKart::MoveRight(float AxisValue)
{
	SteeringThrow = AxisValue;
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move)
{
	SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.VelocityMetersPerSecond = VelocityMetersPerSecond;
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move)
{
	return true;
}