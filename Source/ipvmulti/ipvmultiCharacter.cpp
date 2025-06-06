// Copyright Epic Games, Inc. All Rights Reserved.

#include "ipvmultiCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "ThirdPersonMPProjectile.h"
#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AipvmultiCharacter  

AipvmultiCharacter::AipvmultiCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

	ProjectileClass = AThirdPersonMPProjectile::StaticClass();
	FireRate = 0.25f;
	bIsFiringWeapon = false;

	CurrentAmmo = 5;
	MaxAmmo = 5;
}

void AipvmultiCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled() && AmmoWidgetClass)
	{
		AmmoWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), AmmoWidgetClass);
		if (AmmoWidgetInstance)
		{
			AmmoWidgetInstance->AddToViewport();
			AmmoUIRef = AmmoWidgetInstance;

			FString args = FString::Printf(TEXT("%d"), CurrentAmmo);
			AmmoUIRef->CallFunctionByNameWithArguments(*FString::Printf(TEXT("UpdateAmmo %s"), *args), *GLog, nullptr, true);
		}
	}
}

void AipvmultiCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AipvmultiCharacter, CurrentHealth);
	DOREPLIFETIME(AipvmultiCharacter, CurrentAmmo);
}

void AipvmultiCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AipvmultiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AipvmultiCharacter::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AipvmultiCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AipvmultiCharacter::StartFire);
}

void AipvmultiCharacter::SetCurrentHealth(float healthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
		OnHealthUpdate();
	}
}

float AipvmultiCharacter::TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth(damageApplied);
	return damageApplied;
}

void AipvmultiCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void AipvmultiCharacter::OnHealthUpdate_Implementation()
{
	if (IsLocallyControlled())
	{
		FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

		if (CurrentHealth <= 0)
		{
			FString deathMessage = FString::Printf(TEXT("You have been killed."));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
		}
	}

	if (GetLocalRole() == ROLE_Authority)
	{
		FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
	}
}

void AipvmultiCharacter::StartFire()
{
	if (!bIsFiringWeapon && CurrentAmmo > 0)
	{
		bIsFiringWeapon = true;
		UWorld* World = GetWorld();
		World->GetTimerManager().SetTimer(FiringTimer, this, &AipvmultiCharacter::StopFire, FireRate, false);
		HandleFire();
	}
	else if (CurrentAmmo <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("¡Sin munición!"));
	}
}

void AipvmultiCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

void AipvmultiCharacter::HandleFire_Implementation()
{
	if (CurrentAmmo <= 0) return;

	FVector spawnLocation = GetActorLocation() + (GetActorRotation().Vector() * 100.0f) + (GetActorUpVector() * 50.0f);
	FRotator spawnRotation = GetActorRotation();

	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	AThirdPersonMPProjectile* spawnedProjectile = GetWorld()->SpawnActor<AThirdPersonMPProjectile>(spawnLocation, spawnRotation, spawnParameters);

	CurrentAmmo = FMath::Clamp(CurrentAmmo - 1, 0, MaxAmmo);
	OnRep_CurrentAmmo();
}

void AipvmultiCharacter::OnRep_CurrentAmmo()
{
	if (IsLocallyControlled())
	{
		if (AmmoUIRef)
		{
			FString args = FString::Printf(TEXT("%d"), CurrentAmmo);
			AmmoUIRef->CallFunctionByNameWithArguments(*FString::Printf(TEXT("UpdateAmmo %s"), *args), *GLog, nullptr, true);
		}

		FString ammoMessage = FString::Printf(TEXT("Municion: %d"), CurrentAmmo);
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, ammoMessage);
	}
}

void AipvmultiCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AipvmultiCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

	void AipvmultiCharacter::ReplenishAmmo(int32 AmmoAmount)
	{
		if (GetLocalRole() == ROLE_Authority)
		{
			CurrentAmmo = FMath::Clamp(CurrentAmmo + AmmoAmount, 0, MaxAmmo);
			OnRep_CurrentAmmo();
		}
	}




