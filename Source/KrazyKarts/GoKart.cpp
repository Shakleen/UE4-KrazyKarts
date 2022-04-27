// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

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
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
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
	float RotationAngle = MaxRotationInDegreesPerSecond * DeltaTime * SteeringThrow;
	FQuat RotationDelta(GetActorUpVector(), FMath::DegreesToRadians(RotationAngle));
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

// Called to bind functionality to input
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

