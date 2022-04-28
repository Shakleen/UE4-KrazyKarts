// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

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

void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGoKart, ReplicatedLocation);
	DOREPLIFETIME(AGoKart, ReplicatedRotation);
}

void AGoKart::BeginPlay()
{
	Super::BeginPlay();
}

void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	FVector Force = GetActorForwardVector() * Throttle * MaxDrivingForceInNeutons;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Accelartion = Force / CarMassInKG;
	VelocityMetersPerSecond = VelocityMetersPerSecond + Accelartion * DeltaTime;

	ApplyRotation(DeltaTime);
	UpdateLocationFromVelocity(DeltaTime);

	if (HasAuthority())
	{
		ReplicatedLocation = GetActorLocation();
		ReplicatedRotation = GetActorRotation();
	}
	else
	{
		SetActorLocation(ReplicatedLocation);
		SetActorRotation(ReplicatedRotation);
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

void AGoKart::ApplyRotation(float DeltaTime)
{
	float DeltaLocation = FVector::DotProduct(VelocityMetersPerSecond, GetActorForwardVector()) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
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
	Server_MoveForward(AxisValue);
}

void AGoKart::MoveRight(float AxisValue)
{
	SteeringThrow = AxisValue;
	Server_MoveRight(AxisValue);
}

void AGoKart::Server_MoveForward_Implementation(float AxisValue)
{
	Throttle = AxisValue;
}

bool AGoKart::Server_MoveForward_Validate(float AxisValue)
{
	return true;
}

void AGoKart::Server_MoveRight_Implementation(float AxisValue)
{
	SteeringThrow = AxisValue;
}

bool AGoKart::Server_MoveRight_Validate(float AxisValue)
{
	return FMath::Abs(AxisValue) <= 1;
}