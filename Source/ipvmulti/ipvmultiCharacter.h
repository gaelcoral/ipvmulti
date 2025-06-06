// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Blueprint/UserWidget.h" // Para usar UUserWidget
#include "IpvmultiCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game, Blueprintable)
class IPVMULTI_API AipvmultiCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	AipvmultiCharacter ();
	/** Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Getter for Max Health.*/
	UFUNCTION(BlueprintPure, Category="Health")
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
 
	/** Getter for Current Health.*/
	UFUNCTION(BlueprintPure, Category="Health")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }
 
	/** Setter for Current Health. Clamps the value between 0 and MaxHealth and calls OnHealthUpdate. Should only be called on the server.*/
	UFUNCTION(BlueprintCallable, Category="Health")
	void SetCurrentHealth(float healthValue);
 
	/** Event for taking damage. Overridden from APawn.*/
	UFUNCTION(BlueprintCallable, Category = "Health")
	float TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth;
 
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth;
 
	UFUNCTION()
	void OnRep_CurrentHealth();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Health")
	void OnHealthUpdate();

	UPROPERTY(EditDefaultsOnly, Category="Gameplay|Projectile")
	TSubclassOf<class AThirdPersonMPProjectile> ProjectileClass;
 
	UPROPERTY(EditDefaultsOnly, Category="Gameplay")
	float FireRate;
 
	bool bIsFiringWeapon;

	UFUNCTION(BlueprintCallable, Category="Gameplay")
	void StartFire();
 
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StopFire();
 
	UFUNCTION(Server, Reliable)
	void HandleFire();
 
	FTimerHandle FiringTimer;

	// ---------------------------
	// AMMO SYSTEM (Replicated)
	// ---------------------------

public:
	/** Getter for Current Ammo. */
	UFUNCTION(BlueprintPure, Category="Ammo")
	FORCEINLINE int32 GetCurrentAmmo() const { return CurrentAmmo; }

	/** Getter for Max Ammo. */
	UFUNCTION(BlueprintPure, Category="Ammo")
	FORCEINLINE int32 GetMaxAmmo() const { return MaxAmmo; }

	virtual void BeginPlay() override;


	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo, VisibleAnywhere, BlueprintReadOnly, Category="Ammo")
	int32 CurrentAmmo = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ammo")
	int32 MaxAmmo = 5;

	UFUNCTION()
	void OnRep_CurrentAmmo();

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> AmmoWidgetClass;

	UPROPERTY()
	UUserWidget* AmmoWidgetInstance;

	UPROPERTY()
	UUserWidget* AmmoUIRef;
	
	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void ReplenishAmmo(int32 AmmoAmount);

};



