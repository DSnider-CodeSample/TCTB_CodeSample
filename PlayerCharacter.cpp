#include "PlayerCharacter.h"

#include <PlatformFilemanager.h>

#include "MysterySystems/EvidenceSite.h"
#include "SpookyGameInstance.h"
#include "MysterySystems/SupernaturalScene.h"
#include "UI/WidgetComponentOverride.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "ImageWriteQueue/Public/ImageWriteBlueprintLibrary.h"
#include "Interactables/AlarmClock.h"
#include "Interactables/Interactable.h"
#include "Interactables/InteractableDoor.h"
#include "Interactables/InteractableHoldable.h"
#include "Interactables/InteractableKey.h"
#include "Interactables/InteractableMap.h"
#include "Interactables/InteractableNote.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MonsterAI/MonsterAIController.h"
#include "Particles/ParticleSystemComponent.h"
#include "UI/FolderWidget.h"
#include "LevelManager.h"
#include "UI/MapWidget.h"
#include "UI/PlayerHUD.h"
#include "MysterySystems/RadioSound.h"
#include "Blueprint/Userwidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UI/MysteryWeb/MysteryWebWidget.h"
#include "PlayerCharacterComponent.h"

void APlayerCharacter::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	FString Message = "Equipment State: ";
	switch (CurrentPlayerEquipment)
	{
	case EPlayerEquipmentStates::Camera:
		Message += "Camera";
		TickCamera(DeltaTime);
			break;
	case EPlayerEquipmentStates::Folder:
		Message += "Folder";
		TickFolder(DeltaTime);
		break;
	case EPlayerEquipmentStates::Radio:
		Message += "Radio";
		TickRadio(DeltaTime);
		break;
	case EPlayerEquipmentStates::HoldableObject:
		Message += "HoldableObject";
		if (AInteractableHoldable* CurrentInteractableHoldable = Cast<AInteractableHoldable>(CurrentInteractable))
		{
			CurrentInteractableHoldable->AddOffset(GetInputAxisValue(FName("FolderZoom")) * 10.0f );

			if(AAlarmClock* AlarmClock = Cast<AAlarmClock>(CurrentInteractable))
			{
				if(IsFocusingHoldable)
				{
					AlarmClock->SetBeingWound(true);
				}
				else
				{
					AlarmClock->SetBeingWound(false);
				}
				AlarmClock->UpdatePlayerReticle();
			}
		}
		else
		{
			CurrentPlayerEquipment = PreviousPlayerEquipment;
		}
		break;
	default:
		Message += "something went wrong";
		break;
	}
	GEngine->AddOnScreenDebugMessage(-1, -1.0f, FColor::Cyan, Message);

	// Show safety volume state
	GEngine->AddOnScreenDebugMessage(-1, -1.0f, FColor::Cyan, FString::Printf(TEXT("Player is safe : %s"), PlayerCharacterComponent->IsPlayerProtectedBySafetyVolume() ? TEXT("true") : TEXT("false")));

	// Death animation handling
	if (PlayerCharacterComponent->GetIsCurrentlyDead())
	{
		return;
	}
	
	// Footsteps handling
	ScaledTimeSinceLastFootstep += DeltaTime;

	//update stamina and end sprinting if necessary
	const FVector Velocity = GetVelocity();
	if (!FVector::ZeroVector.Equals(Velocity) && PlayerMovementState == EPlayerMovementStates::Sprinting)
	{
		Stamina -= DeltaTime;
		TimeSinceSprinting = 0;
		if (PlayerHUD) 
		{
			PlayerHUD->UpdateStanima(Stamina);
		}
	}
	else
	{
		TimeSinceSprinting += DeltaTime;
	}
	if (TimeSinceSprinting >= 3.5f && Stamina < MaxStamina)
	{
		Stamina += DeltaTime * 3;
		if (PlayerHUD)
		{
			PlayerHUD->UpdateStanima(Stamina);
		}
	}
	if (Stamina <= 0)
	{
		EndSprint();
		// exhaustion sfx
		FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
		if (AudioDevice && GetWorld() && TimeSinceSprinting == 0.0f)
		{
			AudioDevice->PostEvent("PlayExhaustedBreathing", FootstepAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
		}
	}

	//set max walk speed
	SetWalkSpeed();

	//determine if there is an object that can be interacted with
	InteractionCheck();

	// Run interaction code on "active" interactable
	if (CurrentInteractable && CurrentInteractable->IsBeingInteractedWith())
	{
		CurrentInteractable->DuringInteraction();
	}
	
	FString Health;
	switch (PlayerCharacterComponent->GetHealth())
	{
	case 1:
		LowHealthTickBehavior(DeltaTime);
		Health = "Current health: 1";
		break;
	case 2:
		Health = "Current health: 2";
		break;
	case 3:
		Health = "Current health: 3";
		break;
	default:
		Health = "Current Health is wrong!";
		break;
	}
	GEngine->AddOnScreenDebugMessage(-1, -1.0f, FColor::Orange, Health);

#pragma region Control Reminders
	PlayerHUD->ClearControlReminders();
	PlayerHUD->SetControlRemindersVisible(true);
	switch (CurrentPlayerEquipment)
	{
	case EPlayerEquipmentStates::Camera:
		if (IsLookingAtRightHand)
		{
			for (FText text : ControlReminders_CameraAim)
			{
				PlayerHUD->AddControlReminder(text);
			}
		}
		else if (IsLookingAtLeftHand)
		{
			for (FText text : ControlReminders_CameraLookAtPhoto)
			{
				PlayerHUD->AddControlReminder(text);
			}
		}
		else
		{
			for (FText text : ControlReminders_CameraHip)
			{
				PlayerHUD->AddControlReminder(text);
			}
		}
		break;
	case EPlayerEquipmentStates::Folder:
		if (IsLookingAtLeftHand)
		{
			for (FText text : ControlReminders_FolderLookClose)
			{
				PlayerHUD->AddControlReminder(text);
			}
		}
		else
		{
			for (FText text : ControlReminders_FolderHip)
			{
				PlayerHUD->AddControlReminder(text);
			}
		}
		break;
	case EPlayerEquipmentStates::Radio:
		for (FText text : ControlReminders_RadioHip)
		{
			PlayerHUD->AddControlReminder(text);
		}
		break;
	default:
		break;
	}
	if (CurrentInteractable)
	{
		if (AAlarmClock* alarmClock = Cast<AAlarmClock>(CurrentInteractable))
		{
			for (FText text : ControlReminders_InteractableAlarmClock)
			{
				PlayerHUD->AddControlReminder(text);
			}
		}
		else if (AInteractableDoor* door = Cast<AInteractableDoor>(CurrentInteractable))
		{

			for (FText text : ControlReminders_InteractableDoor)
			{
				PlayerHUD->AddControlReminder(text);
			}
		}
		else if (AInteractableHoldable* holdable = Cast<AInteractableHoldable>(CurrentInteractable))
		{
			for (FText text : ControlReminders_InteractableHoldable)
			{
				PlayerHUD->AddControlReminder(text);
			}
		}
	}
#pragma endregion
}


#pragma region Setup

APlayerCharacter::APlayerCharacter()
{
	//setup capsule
	GetCapsuleComponent()->InitCapsuleSize(45.f, 85.0f);

	//setup main camera
	PlayerCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f));
	PlayerCameraComponent->SetupAttachment(RootComponent);
	PlayerCameraComponent->bUsePawnControlRotation = true;

	MasterPlayerRiggedMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MasterPlayerRiggedMesh"));
	MasterPlayerRiggedMesh->SetupAttachment(PlayerCameraComponent);

	CameraSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CameraSkeletalMesh"));
	CameraSkeletalMesh->SetupAttachment(MasterPlayerRiggedMesh);

	CameraFrontPlate_Temp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CameraFrontPlate_Temp"));
	//CameraFrontPlate_Temp->SetupAttachment(CameraSkeletalMesh, FName(TEXT("Camera")));
	CameraFrontPlate_Temp->SetupAttachment(PlayerCameraComponent);

	PhotoSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PhotoSkeletalMesh"));
	PhotoSkeletalMesh->SetupAttachment(MasterPlayerRiggedMesh);

	//setup handheld camera
	HandheldCameraCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("HandheldCamera"));
	HandheldCameraCapture->SetupAttachment(CameraFrontPlate_Temp); // attach to the camera socket
	HandheldCameraCapture->bCaptureEveryFrame = false;
	HandheldCameraCapture->bCaptureOnMovement = false;

	//setup handheld camera flash
	HandheldCameraFlash = CreateDefaultSubobject<USpotLightComponent>(TEXT("HandheldCameraFlash"));
	//HandheldCameraFlash->SetupAttachment(CameraSkeletalMesh, FName(TEXT("Camera"))); // attach to the camera socket
	HandheldCameraFlash->SetupAttachment(HandheldCameraCapture); 
	HandheldCameraFlash->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	HandheldCameraFlash->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0f, 0.0f, 0.0f)));

	//setup supernatural sensor
	SupernaturalSensor = CreateDefaultSubobject<UPointLightComponent>(TEXT("SupernaturalSensor"));
	SupernaturalSensor->SetupAttachment(CameraSkeletalMesh, FName(TEXT("Camera"))); // attach to the camera socket
	SupernaturalSensor->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	SupernaturalSensor->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0f, 0.0f, 0.0f)));

	//setup sensor particle system
	SensorParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SensorParticleSystem"));
	SensorParticleSystem->SetupAttachment(SupernaturalSensor); //attach to the light
	SensorParticleSystem->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	//setup Polaroid mesh (this is the plane that renders the photo)
	PhotoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhotoMesh"));
	PhotoMesh->SetupAttachment(PhotoSkeletalMesh, FName(TEXT("Photo")));
	PhotoMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	PhotoMesh->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0f, 0.0f, 0.0f)));

	RadioStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>("RadioStaticMesh");
	RadioStaticMesh->SetupAttachment(PhotoSkeletalMesh, FName(TEXT("Photo")));

	RadioAkAudio = CreateDefaultSubobject<UAkComponent>("RadioAkAudio");
	RadioAkAudio->SetupAttachment(RadioStaticMesh);

	// setup radio signal readout
	RadioSignalReadoutStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RadioSignalReadoutMesh"));
	RadioSignalReadoutStaticMesh->SetupAttachment(RadioStaticMesh); // attach to the camera socket
	RadioSignalReadoutStaticMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	RadioSignalReadoutStaticMesh->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0f, 0.0f, 0.0f)));

	RadioHistogramReadoutMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RadioHistogramReadoutMesh"));
	RadioHistogramReadoutMesh->SetupAttachment(RadioStaticMesh); // attach to the camera socket
	RadioHistogramReadoutMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	RadioHistogramReadoutMesh->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0f, 0.0f, 0.0f)));

	//setup folder widget component
	FolderWidgetComponent = CreateDefaultSubobject<UWidgetComponentOverride>(TEXT("FolderWidgetComponent"));
	FolderWidgetComponent->SetupAttachment(RootComponent);

	//setup folder mesh
	FolderUISkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FolderUISkeletalMesh"));
	FolderUISkeletalMesh->SetupAttachment(MasterPlayerRiggedMesh, FName(TEXT("Photo"))); // attach to the left hand socket
	FolderUISkeletalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	FolderUISkeletalMesh->SetRelativeRotation(FRotator::MakeFromEuler(FVector(0.0f, 0.0f, 0.0f)));
	FolderUISkeletalMesh->SetVisibility(false);
	
	// setup camera audio source
	CameraAudioComponent = CreateDefaultSubobject<UAkComponent>(TEXT("CameraAudioComponent"));
	CameraAudioComponent->SetupAttachment(PlayerCameraComponent);
	CameraAudioComponent->SetRelativeLocation(FVector(140.0f, 80.f, -26.f));

	// setup footsteps audio source
	FootstepAudioComponent = CreateDefaultSubobject<UAkComponent>(TEXT("FootstepAudioComponent"));
	FootstepAudioComponent->SetupAttachment(RootComponent);
	FootstepAudioComponent->SetRelativeLocation(FVector(0, 0, -80.f));

	// setup player character component
	PlayerCharacterComponent = CreateDefaultSubobject<UPlayerCharacterComponent>(TEXT("PlayerCharacterComponent"));
	PlayerCharacterComponent->SetupCameraReferences(SupernaturalSensor, HandheldCameraCapture, HandheldCameraFlash, CameraAudioComponent);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//movement bindings
	PlayerInputComponent->BindAxis("MoveForward", this, &APlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlayerCharacter::MoveRight);

	//mouse control bindings
	PlayerInputComponent->BindAxis("Turn", this, &APlayerCharacter::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APlayerCharacter::AddControllerPitchInput);

	//sprint bindings
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &APlayerCharacter::StartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &APlayerCharacter::EndSprint);

	//sneak bindings
	PlayerInputComponent->BindAction("Sneak", IE_Pressed, this, &APlayerCharacter::ToggleSneak);

	//take photo binding
	PlayerInputComponent->BindAction("TakePhoto", IE_Pressed, this, &APlayerCharacter::StartChargingCamera);
	PlayerInputComponent->BindAction("TakePhoto", IE_Released, this, &APlayerCharacter::TakePhoto);

	//open folder binding
	PlayerInputComponent->BindAction("OpenFolder", IE_Pressed, this, &APlayerCharacter::OnPressFolderButton);

	//open radio binding
	PlayerInputComponent->BindAction("OpenRadio", IE_Pressed, this, &APlayerCharacter::OnPressRadioButton);

	//interact binding
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &APlayerCharacter::Interact);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &APlayerCharacter::ReleaseInteract);

	//aim down sights bindings
	PlayerInputComponent->BindAction("ADS", IE_Pressed, this, &APlayerCharacter::StartAimDownSights);
	PlayerInputComponent->BindAction("ADS", IE_Released, this, &APlayerCharacter::EndAimDownSights);

	//toggle show accessible note text binding
	PlayerInputComponent->BindAction("ToggleShowNoteText", IE_Pressed, this, &APlayerCharacter::LookCloselyAtLeftHand);

	// folder controls
	PlayerInputComponent->BindAction("FolderNavigateUp", IE_Pressed, this, &APlayerCharacter::FolderNavigateUp);
	PlayerInputComponent->BindAction("FolderNavigateDown", IE_Pressed, this, &APlayerCharacter::FolderNavigateDown);

	PlayerInputComponent->BindAction("FolderSelect", IE_Pressed, this, &APlayerCharacter::FolderSelect);
	
	//pause controls
	PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &APlayerCharacter::OnPressPause).bExecuteWhenPaused = true;

	PlayerInputComponent->BindAxis(TEXT("FolderPanRight"));
	PlayerInputComponent->BindAxis(TEXT("FolderPanUp"));
	PlayerInputComponent->BindAxis(TEXT("FolderZoom"));
	
	PlayerInputComponent->BindAxis("TuneRadioFrequencyUp", this, &APlayerCharacter::TuneRadioFrequencyUp);

	// setup mappings
	CreateControlFormatArguments();
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Get reference to the LevelManager for this map
	LevelManager = ALevelManager::GetLevelManager(GetWorld());
	
	//setup folder to use a dynamic material instance
	FolderMaterialInstance = FolderUISkeletalMesh->CreateAndSetMaterialInstanceDynamic(0);


	//set up and display the PlayerHUD
	PlayerHUD = Cast<UPlayerHUD>(CreateWidget<UUserWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PlayerHUDClass));
	if (PlayerHUD)
	{
		PlayerHUD->AddToViewport();
		PlayerHUD->MaxStamina = MaxStamina;
		PlayerHUD->UpdateStanima(MaxStamina);
		PlayerHUD->PlayerHUDInstance = PlayerHUD;
	}

	PlayerCharacterComponent->SetupFolder(LevelManager->LevelMysteryWebClass, FolderWidgetComponent);
	PlayerCharacterComponent->SetupRadio(RadioAkAudio, GetCapsuleComponent(), RadioHistogramReadoutMesh);
	PlayerCharacterComponent->OnPlayerDeath.AddDynamic(this, &APlayerCharacter::OnDeath);
	PlayerCharacterComponent->OnPlayerEndDeath.AddDynamic(this, &APlayerCharacter::OnEndDeath);
	PlayerCharacterComponent->OnPlayerTakeDamage.AddDynamic(this, &APlayerCharacter::TakeHealthDamage);
	PlayerCharacterComponent->OnPlayerHeal.AddDynamic(this, &APlayerCharacter::HealDamage);
	PlayerCharacterComponent->NotifyPlayerOfPickup.AddDynamic(PlayerHUD, &UPlayerHUD::AddPickupPopup);
	PlayerCharacterComponent->OnFolderUnreadUpdated.AddDynamic(PlayerHUD, &UPlayerHUD::SetUnreadNotificationVisible);
	   	  
	//set default player movement 
	PlayerMovementState = EPlayerMovementStates::Walking;

	//set default player equipment
	CurrentPlayerEquipment = EPlayerEquipmentStates::Camera; // TODO : Should start as none! We dont have the animations yet tho

	//setup camera position info
	CameraDefaultPosition = PlayerCameraComponent->GetRelativeLocation();
	CameraCrouchPosition = CameraDefaultPosition - FVector(0, 0, 40.f);

	//setup player hands to use a dynamic material instance
	PlayerHandsMaterialInstance = MasterPlayerRiggedMesh->CreateAndSetMaterialInstanceDynamic(0);
	PlayerHandsMaterialInstance->SetScalarParameterValue(FName("HP_Amount"), PlayerCharacterComponent->GetHealth());
}

#pragma endregion

#pragma region Movement

FVector APlayerCharacter::GetLookMovement() const
{
	return FVector(GetInputAxisValue("Turn"), GetInputAxisValue("LookUp"), 0);
}

FVector APlayerCharacter::GetWalkMovement(const bool bIsLocal) const
{
	FVector RelativeAttemptedVelocity = FVector(GetInputAxisValue("MoveForward") * MaxWalkSpeed, GetInputAxisValue("MoveRight") * MaxWalkSpeed, 0);
	if (!bIsLocal)
	{
		RelativeAttemptedVelocity = RelativeAttemptedVelocity.RotateAngleAxis(GetActorRotation().Euler().Z, FVector::UpVector);
	}
	return RelativeAttemptedVelocity;
}

void APlayerCharacter::MoveForward(float Value)
{
	// can't walk forward when looking closely at folder
	if (IsLookingAtLeftHand && CurrentPlayerEquipment == EPlayerEquipmentStates::Folder)
	{
		return;
	}
	
	//ensure controller exists and movement value is not 0
	if ((Controller != NULL) && (Value != 0.0f))
	{
		//get forward rotation
		FRotator Rotation = Controller->GetControlRotation();
		//limit pitch when walking or falling
		if (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling())
		{
			Rotation.Pitch = 0.0f;
		}

		//add movement in that direction
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);
		
		if (PlayerMovementState == EPlayerMovementStates::Sprinting)
		{
			if (Value > 0) { Value = 1.0f; } //should only sprint if player is moving forward
		}

		if (GetWalkMovement().Size() != 0 && GetCharacterMovement()->Velocity.Size() < 0.5f)
		{
			PlayWalkingSound();
		}
		AddMovementInput(Direction, Value);
	}
}

void APlayerCharacter::MoveRight(const float Value)
{
	// can't move when looking closely at folder
	if (IsLookingAtLeftHand && CurrentPlayerEquipment == EPlayerEquipmentStates::Folder)
	{
		return;
	}
	//ensure controller exists and movement value is not 0
	if ((Controller != NULL) && (Value != 0.0f))
	{
		//get right rotation
		const FRotator Rotation = Controller->GetControlRotation();
		const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);

		//if (PlayerMovementState == EPlayerMovementStates::Sprinting && GetWalkMovement().Y < 0) { return; }

		if (GetWalkMovement().Size() != 0 && GetCharacterMovement()->Velocity.Size() < 0.5f)
		{
			PlayWalkingSound();
		}
		//add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void APlayerCharacter::StartSprint()
{
	//can't sprint while aiming down sights or player has no stamina
	if (IsLookingAtRightHand || IsLookingAtLeftHand || Stamina <= 0) { return; }

	//move camera up if player was sneaking
	if (PlayerMovementState == EPlayerMovementStates::Sneaking) { LerpCamera(CameraDefaultPosition); }

	//set player to sprinting
	PlayerMovementState = EPlayerMovementStates::Sprinting;
}

void APlayerCharacter::EndSprint()
{
	//not necessary if player is not sprinting
	if (PlayerMovementState.GetValue() != EPlayerMovementStates::Sprinting) { return; }

	//set player to walking state
	PlayerMovementState = EPlayerMovementStates::Walking;
}

void APlayerCharacter::ToggleSneak()
{
	//end sneak if player is sneaking
	if (PlayerMovementState.GetValue() == EPlayerMovementStates::Sneaking)
	{
		PlayerMovementState = EPlayerMovementStates::Walking;
		LerpCamera(CameraDefaultPosition);
		FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
		if (AudioDevice && GetWorld())
		{
			AudioDevice->PostEvent("PlayEnterCrouch", CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
		}
		return;
	}

	//begin sneak
	PlayerMovementState = EPlayerMovementStates::Sneaking;
	LerpCamera(CameraCrouchPosition);
	FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
	if (AudioDevice && GetWorld())
	{
		AudioDevice->PostEvent("PlayExitCrouch", CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
	}
}

void APlayerCharacter::SetWalkSpeed() const
{
	//get character velocity and return early if it is zero
	FVector Velocity = GetVelocity();
	if (FVector::ZeroVector.Equals(Velocity)) { return; }

	//get forward vector and normalize vectors
	FVector Forward = GetActorForwardVector();
	Velocity.Normalize();
	Forward.Normalize();

	//calculate new speed (from 200 - 600 based on direction facing)
	float NewSpeed = FVector::DotProduct(Velocity, Forward);
	NewSpeed += 2;
	NewSpeed /= 3;

	float BaseSpeed = MaxWalkSpeed;
	if (PlayerMovementState.GetValue() == EPlayerMovementStates::Sprinting) { BaseSpeed = SprintMaxWalkSpeed; }
	if (PlayerMovementState.GetValue() == EPlayerMovementStates::Sneaking) { BaseSpeed = SneakMaxWalkSpeed; }

	NewSpeed = BaseSpeed * NewSpeed;

	//set new speed
	Cast<UCharacterMovementComponent>(GetCharacterMovement())->MaxWalkSpeed = NewSpeed;
	Cast<UCharacterMovementComponent>(GetCharacterMovement())->MaxWalkSpeedCrouched = NewSpeed;
}

void APlayerCharacter::PlayWalkingSound()
{
	if (ScaledTimeSinceLastFootstep < 0.1f) //prevent same-frame steps due to animation bp duplication
	{
		return;
	}
	ScaledTimeSinceLastFootstep = 0.0f;

	// select correct footstep audio settings
	float Radius;
	FString AudioEventString;
	switch (PlayerMovementState) // TODO: magic walk/sneak/run sound radius
	{
		case EPlayerMovementStates::Sneaking:
			Radius = 250;
			AudioEventString = AkEvent_Footstep_Sneak_Hardwood;
			break;
		case EPlayerMovementStates::Sprinting:
			Radius = 2000;
			AudioEventString = AkEvent_Footstep_Sprint_Hardwood;
			break;
		default:
		case EPlayerMovementStates::Walking:
			Radius = 1000;
			AudioEventString = AkEvent_Footstep_Walk_Hardwood;
			break;
	}

	FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
	if (AudioDevice && GetWorld())
	{
		AudioDevice->PostEvent(AudioEventString, FootstepAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
	}
	PlayerCharacterComponent->MonsterController->ReportSound(GetActorLocation(), Radius, false, false, 1.5f);
}

#pragma endregion

#pragma region Polaroid

void APlayerCharacter::StartChargingCamera()
{
	//can't charge camera if camera isn't out
	if (CurrentPlayerEquipment != EPlayerEquipmentStates::Camera) { return; }
	if (IsLookingAtLeftHand) { return; }
	
	if (CurrentPlayerEquipment == EPlayerEquipmentStates::Camera)
	{
		PlayerCharacterComponent->StartChargingCamera();
	}
}

void APlayerCharacter::TakePhoto()
{
	//TODO : we're using the same input for throwing, for now. Should have separate bindings eventually? - YES
	if (CurrentPlayerEquipment == EPlayerEquipmentStates::HoldableObject)
	{
		Cast<AInteractableHoldable>(CurrentInteractable)->Throw(PlayerCameraComponent->GetForwardVector() * HoldableThrowMagnitude, this);
	}
	
	// can't take photo if camera isn't out
	if (CurrentPlayerEquipment != EPlayerEquipmentStates::Camera) { return; }
	if (IsLookingAtLeftHand) { return; }

	FString WhyBadPhoto;
	PlayerCharacterComponent->TakePhoto(WhyBadPhoto);
	if (WhyBadPhoto.Len() > 0 && !PlayerCharacterComponent->GetWasGoodPhotoTakenThisFrame())
	{
		PlayerHUD->AddBadPhotoPopup(WhyBadPhoto);
	}
}

#pragma endregion

#pragma region Interaction

void APlayerCharacter::SetControllingDoor(const bool IsControlling)
{
	IsControllingDoor = IsControlling;
}

void APlayerCharacter::KickHoldable(AInteractableHoldable* Holdable, FVector HitLocation)
{
	FVector DirectionVec = HitLocation - GetActorLocation();
	DirectionVec.Normalize();

	if (Holdable == CurrentInteractable)
	{
		return;
	}
	
	switch (PlayerMovementState)
	{
	case EPlayerMovementStates::Sneaking:
		DirectionVec += GetWalkMovement(); //nudges the force vector forward and up a bit, to toss objects in the air and more in the direction of monster motion
		DirectionVec.Normalize();
		Holdable->Kick(DirectionVec * HoldableKickSneakMagnitude, 1, this);
		break;
	case EPlayerMovementStates::Walking:	
		DirectionVec += GetWalkMovement() * 2.0f + FVector::UpVector; //nudges the force vector forward and up a bit, to toss objects in the air and more in the direction of monster motion
		DirectionVec.Normalize();
		Holdable->Kick(DirectionVec * HoldableKickWalkMagnitude, 2, this);
		break;
	case EPlayerMovementStates::Sprinting:
		DirectionVec += GetWalkMovement() * 3.0f + FVector::UpVector; //nudges the force vector forward and up a bit, to toss objects in the air and more in the direction of monster motion
		DirectionVec.Normalize();
		Holdable->Kick(DirectionVec * HoldableKickRunMagnitude, 3, this);
		break;
	default: // ??? possible even?
		break;
	}
}

void APlayerCharacter::InteractionCheck()
{
	//define trace parameters
	FVector Start = PlayerCameraComponent->GetComponentToWorld().GetLocation();
	float Distance = GetMovementComponent()->Velocity.Size() > (MaxWalkSpeed + 1.0f) ? 400 : 200;
	FVector End = PlayerCameraComponent->GetComponentToWorld().GetLocation() + (PlayerCameraComponent->GetForwardVector() * Distance);
	FHitResult OutHit;
	FCollisionQueryParams CollisionParams;

	//hide prompt and special reticles in case we we don't see an interactable object
	//it will be redisplayed if we need it
	PlayerHUD->HidePrompt();

	//trace forward to check for objects
	if (!IsLookingAtRightHand && !IsLookingAtLeftHand && GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, CollisionParams))
	{
		//determine if the hit object is interactable
		AInteractableObject* InteractableObject = Cast<AInteractableObject>(OutHit.GetActor());
		if (InteractableObject)
		{
			if (CurrentInteractable != InteractableObject)
			{

				if ((CurrentInteractable && !CurrentInteractable->IsBeingInteractedWith()) || CurrentInteractable == nullptr)
				{
					if (CurrentInteractable)
					{
						CurrentInteractable->OnEndMouseOver();
						CurrentInteractable->ResetInteractable();
					}

					InteractableObject->OnMouseOver();
					CurrentInteractable = InteractableObject;
				}
			}

			//if so, display the HUD prompt
			PlayerHUD->DisplayPrompt(CurrentInteractable->GetPrompt());

			return;
		}
	}
	else if (CurrentInteractable && CurrentInteractable->IsBeingInteractedWith())
	{
		//if so, display the HUD prompt
		PlayerHUD->DisplayPrompt(CurrentInteractable->GetPrompt());
	}

	if (CurrentInteractable && (!CurrentInteractable->IsBeingInteractedWith() || (GetActorLocation() - CurrentInteractable->GetActorLocation()).Size() > 350))
	{
		CurrentInteractable->OnEndMouseOver();
		CurrentInteractable->ResetInteractable();
		CurrentInteractable = nullptr;
	}
}

void APlayerCharacter::Interact()
{
	if (CurrentInteractable)
	{
		CurrentInteractable->OnInteract();

		// special case for holdable, which should change player equipment and have its "gripped location" set
		if (AInteractableHoldable* Holdable = Cast<AInteractableHoldable>(CurrentInteractable))
		{
			PreviousPlayerEquipment = CurrentPlayerEquipment;
			CurrentPlayerEquipment = EPlayerEquipmentStates::HoldableObject;

			//define trace parameters, copied over from interaction check
			FVector Start = PlayerCameraComponent->GetComponentToWorld().GetLocation();
			float Distance = GetMovementComponent()->Velocity.Size() > (MaxWalkSpeed + 1.0f) ? 400 : 200;
			FVector End = PlayerCameraComponent->GetComponentToWorld().GetLocation() + (PlayerCameraComponent->GetForwardVector() * Distance);
			FHitResult OutHit;
			FCollisionQueryParams CollisionParams;
			
			GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, CollisionParams);

			if (OutHit.GetActor() == CurrentInteractable) //if its not, thats super weird
			{
				Holdable->SetGrabbedLocation(OutHit.Location);
				Holdable->SetOffset(OutHit.Distance);
			}
		}
	}
}

void APlayerCharacter::ReleaseInteract()
{
	if (CurrentInteractable)
	{
		// special case for holdable
		if (CurrentPlayerEquipment == EPlayerEquipmentStates::HoldableObject)
		{
			CurrentPlayerEquipment = PreviousPlayerEquipment;
		}
		CurrentInteractable->OnReleaseInteract();
	}
}

void APlayerCharacter::AddControllerYawInput(const float Val)
{
	if (IsControllingDoor)
	{
		Super::AddControllerYawInput(Val * 0.1f);
		return;
	}
	if (IsLookingAtLeftHand && CurrentPlayerEquipment == EPlayerEquipmentStates::Folder)
	{
		return;
	}

	// allow rotating held object while holding aim key
	if (CurrentPlayerEquipment == EPlayerEquipmentStates::HoldableObject && IsFocusingHoldable && CurrentInteractable) // TODO : && "CurrentInteractable" should not be necessary here but it sometimes crashes. Something's faulty with the state logic here. 
	{
		Cast<AInteractableHoldable>(CurrentInteractable)->AddYawRotation(Val);
		
		if (!Cast<AAlarmClock>(CurrentInteractable)) //alarm clocks dont rotate when focused, they just wind - only early return for non-alarmclocks
		{
			return;
		}
	}
	
	Super::AddControllerYawInput(Val);
}

void APlayerCharacter::AddControllerPitchInput(const float Val)
{
	if (IsControllingDoor)
	{
		Super::AddControllerPitchInput(Val * 0.1f);
		return;
	}
	if (IsLookingAtLeftHand && CurrentPlayerEquipment == EPlayerEquipmentStates::Folder)
	{
		return;
	}

	// allow rotating held object while holding aim key
	if (CurrentPlayerEquipment == EPlayerEquipmentStates::HoldableObject && IsFocusingHoldable && CurrentInteractable) // TODO : && "CurrentInteractable" should not be necessary here but it sometimes crashes. Something's faulty with the state logic here. 
	{
		Cast<AInteractableHoldable>(CurrentInteractable)->AddPitchRotation(Val);
		if (!Cast<AAlarmClock>(CurrentInteractable)) //alarm clocks dont rotate when focused, they just wind - only early return for non-alarmclocks
		{
			return;
		}
	}
	
	Super::AddControllerPitchInput(Val);
}

#pragma endregion

#pragma region UI

void APlayerCharacter::OnPressPause()
{
	if(UGameplayStatics::IsGamePaused(GetWorld()))
	{
		if(PauseMenu)
		{
			PauseMenu->RemoveFromParent();
		}

		Cast<APlayerController>(GetController())->bShowMouseCursor = false;

		//unpause game
		UGameplayStatics::SetGamePaused(GetWorld(), false);
	}
	else
	{
		// summon the pause menu
		if(!PauseMenu)
		{
			PauseMenu = CreateWidget<UUserWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), PauseMenuClass);
		}
		if (PauseMenu)
		{
			PauseMenu->AddToViewport();			
			Cast<APlayerController>(GetController())->bShowMouseCursor = true;
		}

		//puase game
		UGameplayStatics::SetGamePaused(GetWorld(), true);
	}
}

void APlayerCharacter::PutAwayAllLeftHandEquipment() const
{
	if (PhotoMesh->GetVisibleFlag())
	{
		PhotoMesh->SetVisibility(false);
	}
	if (FolderUISkeletalMesh->GetVisibleFlag())
	{
		FolderUISkeletalMesh->SetVisibility(false);
	}
	if (PhotoSkeletalMesh->GetVisibleFlag())
	{
		PhotoSkeletalMesh->SetVisibility(false);
	}
	RadioStaticMesh->SetVisibility(false, true);
}

void APlayerCharacter::LookCloselyAtLeftHand()
{
	if (IsLookingAtRightHand) { return; }
	
	IsLookingAtLeftHand = !IsLookingAtLeftHand;
	
	switch (CurrentPlayerEquipment)
	{
		case EPlayerEquipmentStates::Folder:
			//TODO : Once we have folder
			// hide dot when looking closely at folder
			PlayerHUD->SetShowingDot(!IsLookingAtLeftHand);
			break;
		case EPlayerEquipmentStates::Camera:
			UE_LOG(LogTemp, Warning, TEXT("Toggling look at polaroid"));
			// hide blur brackets and dot when looking closely at polaroid
			PlayerHUD->SetShowingBlurBrackets(!IsLookingAtLeftHand);
			PlayerHUD->SetShowingDot(!IsLookingAtLeftHand);
			break;
		default:
			break;
	}
}

#pragma endregion

#pragma region Folder

void APlayerCharacter::FolderSelect() 
{
	PlayerCharacterComponent->FolderUI->OnFolderSelect();
}

// Input handling functions for folder UI navigation
void APlayerCharacter::FolderNavigateUp()
{
	FolderVerticalInput(true);
}
void APlayerCharacter::FolderNavigateDown()
{
	FolderVerticalInput(false);
}


void APlayerCharacter::FolderVerticalInput(const bool bIsMoveUp) const
{
	if (GetCurrentPlayerEquipment() != EPlayerEquipmentStates::Folder)
	{
		return;
	}

	if (bIsMoveUp)
	{
		PlayerCharacterComponent->FolderUI->WebTraverseNotesList(1);
		PlayerCharacterComponent->FolderUI->WebTraversePhotosList(1);
	}
	else
	{
		PlayerCharacterComponent->FolderUI->WebTraverseNotesList(-1);
		PlayerCharacterComponent->FolderUI->WebTraversePhotosList(-1);
	}

	PlayerCharacterComponent->FolderUI->SetVisibility(ESlateVisibility::Visible);
}


void APlayerCharacter::SwapFromFolderToCamera()
{
	HasSwappedBetweenFolderAndCamera = true;

	PutAwayAllLeftHandEquipment();
	PhotoMesh->SetVisibility(true);
	PhotoSkeletalMesh->SetVisibility(true);
	CameraSkeletalMesh->SetVisibility(true);
	SupernaturalSensor->SetVisibility(true);

	FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
	if (AudioDevice && GetWorld())
	{
		AudioDevice->PostEvent("PlayPlayerRaiseCamera", CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
	}
}

void APlayerCharacter::SwapFromCameraToFolder()
{
	HasSwappedBetweenFolderAndCamera = true;

	PutAwayAllLeftHandEquipment();
	CameraSkeletalMesh->SetVisibility(false);
	FolderUISkeletalMesh->SetVisibility(true);
	SupernaturalSensor->SetVisibility(false);
	PlayerCharacterComponent->FolderUI->GetMysteryWeb()->UpdateBoundingIcons();
	PlayerCharacterComponent->FolderUI->ResetWebCursor();



	FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
	if (AudioDevice && GetWorld())
	{
		AudioDevice->PostEvent("PlayPlayerRaiseFolder", CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
	}
}

void APlayerCharacter::SwapToRadio()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, "Swap to Radio");

	PutAwayAllLeftHandEquipment();
	CameraSkeletalMesh->SetVisibility(false);
	SupernaturalSensor->SetVisibility(false);
	RadioStaticMesh->SetVisibility(true, true);
}

void APlayerCharacter::HandleFolderPanInput() const
{
	if (IsLookingAtLeftHand) 
	{
		PlayerCharacterComponent->FolderUI->ProcessPanZoomInput(
			FVector2D(GetLookMovement().X, GetLookMovement().Y),
			GetInputAxisValue(FName("FolderZoom"))
		);
	}
}

void APlayerCharacter::TickFolder(float DeltaTime)
{
	// Redraw the folder UI while the folder is open
	if (FolderUISkeletalMesh->GetVisibleFlag())
	{
		PlayerCharacterComponent->FolderUI->PsuedoTick(DeltaTime);
	}

	//temporary - control the folder (at all times lol)
	if (IsLookingAtLeftHand)
	{
		HandleFolderPanInput();
	}
}

void APlayerCharacter::TickCamera(float DeltaTime)
{
	PlayerCharacterComponent->TickCamera(DeltaTime);
#pragma region Motion Blur
	// determine blur strength
	float TargetBlurMagnitude = (GetLookMovement().Size() * CameraMotionBlur_LookScalar) + (FVector(GetWalkMovement().X, GetWalkMovement().Z + GetWalkMovement().Y, 0).Size() / MaxWalkSpeed * CameraMotionBlur_MovementScalar);
	
	if (PlayerMovementState == Sprinting && GetWalkMovement().Size() > 0)
	{
		TargetBlurMagnitude = 1.0f;
	}
	TargetBlurMagnitude = FMath::Clamp(TargetBlurMagnitude, 0.0f, 1.0f);

	// determine blur direction
	FVector TargetBlurDirection;
	if (GetLookMovement().Size() >= CameraMotionBlur_LookOverrideMagnitude || (GetWalkMovement() == FVector::ZeroVector && GetLookMovement().Size() > 0))
	{
		TargetBlurDirection = GetLookMovement();
	}
	else if (GetWalkMovement().Size() > 0)
	{
		TargetBlurDirection = GetWalkMovement();
	}
	TargetBlurDirection.Normalize();

	// set blur related things
	PlayerCharacterComponent->SetTargetMotionBlur(TargetBlurDirection * TargetBlurMagnitude);
	PlayerHUD->UpdateBlurBrackets(PlayerCharacterComponent->GetCurrentMotionBlur().Size());
#pragma endregion

	// --- Zoom
	if (IsLookingAtRightHand)
	{
		PlayerCharacterComponent->ZoomIn(0.1f * GetInputAxisValue(FName("FolderZoom")));
	}
	else
	{
		PlayerCharacterComponent->SetZoom(0.0f);
	}
}

void APlayerCharacter::OnPressFolderButton()
{
	if(CurrentPlayerEquipment == EPlayerEquipmentStates::Radio)
	{
		PlayerCharacterComponent->StopPlayingRadio();
	}
	SetShowingFolder(CurrentPlayerEquipment != EPlayerEquipmentStates::Folder);
}

void APlayerCharacter::SetShowingFolder(const bool Show)
{
	PlayerHUD->SetShowingDot(true);

	if (Show)
	{
		PlayerHUD->SetShowingBlurBrackets(false);

		IsLookingAtLeftHand = false;
		
		CurrentPlayerEquipment = EPlayerEquipmentStates::Folder;
		
		if (!PlayerCharacterComponent->FolderUI->GetMysteryWeb()->GetCurrentFolderWidget())
		{
			PlayerCharacterComponent->FolderUI->GetMysteryWeb()->SetCurrentFolderWidget(PlayerCharacterComponent->FolderUI);
		}
		
		PlayerCharacterComponent->FolderUI->GetMysteryWeb()->UpdateBoundingIcons();
		PlayerCharacterComponent->FolderUI->ResetWebCursor();

		FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
		if (AudioDevice && GetWorld())
		{
			//below conditional handles animation interruption
			if (!HasSwappedBetweenFolderAndCamera)
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, "PlayPlayerRaiseFolder");
				AudioDevice->PostEvent("PlayPlayerRaiseFolder", CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, "PlayPlayerLowerCamera");
				AudioDevice->PostEvent("PlayPlayerLowerCamera", CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
			}
		}
	}
	else
	{	
		PlayerHUD->SetShowingBlurBrackets(true);

		/* TODO : Visibility toggling should probably be handled animation-side so they disappear 
		* or reappear once offscreen (or animation should call event that toggles them) - currently
		* this functionality is handled all over the place
		*/
		IsLookingAtLeftHand = false;
		//Currently, closing the folder switches back to the polaroid
		//CurrentLeftHandEquipment = EPlayerLeftHandEquipment::LeftNone;
		CurrentPlayerEquipment = EPlayerEquipmentStates::Camera;
		
		FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
		if (AudioDevice && GetWorld())
		{
			// play ambient radio static - in case it stopped for some reason 
			RadioAkAudio->Stop();
			
			//below conditional handles animation interruption
			if (!HasSwappedBetweenFolderAndCamera)
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, "PlayPlayerRaiseCamera");
				AudioDevice->PostEvent("PlayPlayerRaiseCamera", CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, "PlayPlayerLowerFolder");
				AudioDevice->PostEvent("PlayPlayerLowerFolder", CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
			}
		}
	}
	HasSwappedBetweenFolderAndCamera = false; // see definition for more info
}

#pragma endregion

#pragma region Radio

void APlayerCharacter::TickRadio(float DeltaTime)
{
	PlayerCharacterComponent->TickRadio(DeltaTime);
	
	// TEMPORARY - currently using "scrub right" as a play toggle rather than a fast forward
	if (IsRadioHoldingPlay && GetInputAxisValue(FName("TuneRadioFrequencyUp")) <= 0)
	{
		IsRadioHoldingPlay = false;
	}
}

void APlayerCharacter::TuneRadioFrequencyUp(float Value)
{
	if (CurrentPlayerEquipment == EPlayerEquipmentStates::Radio)
	{
		if (Value > 0) //play
		{
			if (!IsRadioHoldingPlay) //TEMPORARY - currently using "tune radio frequency right" as a toggle for play/pause
			{
				IsRadioHoldingPlay = true;
				if (PlayerCharacterComponent->GetIsRadioPlaying())
				{
					PlayerCharacterComponent->StopPlayingRadio();
				}
				else
				{
					PlayerCharacterComponent->StartPlayingRadio();
				}
			}
		}
		else if (Value < 0 ) // rewinding
		{
			PlayerCharacterComponent->SetRadioFrequency( PlayerCharacterComponent->GetRadioFrequency() + (Value * RadioFrequencyTuneRate * GetWorld()->GetDeltaSeconds()));
			PlayerCharacterComponent->StopPlayingRadio();
		}
	}
}

void APlayerCharacter::OnPressRadioButton()
{
	//if putting the radio away, it stops playing
	if(CurrentPlayerEquipment == EPlayerEquipmentStates::Radio)
	{
		PlayerCharacterComponent->StopPlayingRadio();
	}
	
	SetShowingRadio(CurrentPlayerEquipment != EPlayerEquipmentStates::Radio);
}

void APlayerCharacter::SetShowingRadio(const bool Show)
{
	PlayerHUD->SetShowingDot(true);

	if(Show)
	{
		PlayerHUD->SetShowingBlurBrackets(false);

		IsLookingAtLeftHand = false;

		CurrentPlayerEquipment = EPlayerEquipmentStates::Radio;

		//TODO: Mark help me with animation interruption handling lol
		//OnRadioPoll.Broadcast(); // this should be already done in radio tick
	}
	else
	{		
		PlayerHUD->SetShowingBlurBrackets(true);

		IsLookingAtLeftHand = false;

		CurrentPlayerEquipment = EPlayerEquipmentStates::Camera;


		PlayerCharacterComponent->StopPlayingRadio();
	}
}

#pragma endregion 

#pragma region Control Schemes

void APlayerCharacter::LowHealthTickBehavior(float DeltaTime)
{	
	HeartbeatTimer -= DeltaTime;

	if (HeartbeatTimer < 0.0f)
	{
		if (IsShortHeartbeatNext)
		{
			HeartbeatTimer = HeartbeatDuration_Short;
		}
		else
		{
			HeartbeatTimer = HeartbeatDuration_Long;
		}
		//play heartbeat sound
		if ((PlayerCharacterComponent->MonsterPawn->GetActorLocation() - GetActorLocation()).Size() < HeartbeatMonsterPulseProximity)
		{
			PlayerCharacterComponent->MonsterPawn->PlayMonsterHeartbeatPulse();
			FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
			if(AudioDevice && GetWorld())
			{
				if (IsShortHeartbeatNext)
				{
					AudioDevice->PostEvent(*FString("PlayHeartbeatLight"), FootstepAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
				}
				else
				{
					AudioDevice->PostEvent(*FString("PlayHeartbeatHeavy"), FootstepAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
				}
			}
			IsPlayingBackgroundHeartbeat = false;
			OnHeartBeat(0.2f);
		}
		else if (!IsPlayingBackgroundHeartbeat)
		{
			IsPlayingBackgroundHeartbeat = true;
			FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
			if(AudioDevice && GetWorld())
			{
				AudioDevice->PostEvent(*FString("PlayHeartbeatBackground"), FootstepAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
			}
		}
		IsShortHeartbeatNext = !IsShortHeartbeatNext;
	}
}

FFormatNamedArguments APlayerCharacter::GetCurrentControlSchemeNamedArguments() const
{
	switch (CurrentControllerType)
	{
		case ECurrentControllerType::XboxController:
			return XboxControllerNamedArguments;
			break;
		default:
		case ECurrentControllerType::KeyboardMouse:
			return KeyboardMouseNamedArguments;
			break;
	}
}


void APlayerCharacter::CreateControlFormatArguments()
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	// reset mappings and recreate them later
	XboxControllerNamedArguments.Empty();
	KeyboardMouseNamedArguments.Empty();

	// Action Mappings
	TArray<FName> ActionNames;
	GetDefault<UInputSettings>()->GetActionNames(ActionNames);
	for (const FName ActionName : ActionNames)
	{
		TArray<FInputActionKeyMapping> KeyMappings = PlayerController->PlayerInput->GetKeysForAction(ActionName);

		bool bGamepadFound = false;
		bool bKeyboardMouseFound = false;
		for (FInputActionKeyMapping KeyMapping : KeyMappings)
		{
			//first gamepad mapping
			if(KeyMapping.Key.IsGamepadKey() && !bGamepadFound) // TODO: eventually we need to detect controller types here, but right now we only support xbox
			{
				bGamepadFound = true;
				XboxControllerNamedArguments.Add(ActionName.ToString(), FText::FromString("<img id=\"" + KeyMapping.Key.GetFName().ToString() + "\"/>"));
			}
			else if(!bKeyboardMouseFound) // first keyboard/mouse mapping
			{
				bKeyboardMouseFound = true;
				KeyboardMouseNamedArguments.Add(ActionName.ToString(), FText::FromString("<img id=\"" + KeyMapping.Key.GetFName().ToString() + "\"/>"));
				
			}
		}
	}

	// Axis Mappings
	TArray<FName> AxisNames;
	GetDefault<UInputSettings>()->GetAxisNames(AxisNames);
	for (const FName AxisName : AxisNames)
	{
		TArray<FInputAxisKeyMapping> KeyMappings = PlayerController->PlayerInput->GetKeysForAxis(AxisName);

		FString KeyboardMouseIcons = "";
		FString XboxIcons = "";

		for (FInputAxisKeyMapping KeyMapping : KeyMappings)
		{
			//first gamepad mapping
			if(KeyMapping.Key.IsGamepadKey()) // TODO: eventually we need to detect controller types here, but right now we only support xbox
			{
				if(XboxIcons != "")
				{
					XboxIcons += "/";
				}

				XboxIcons += "<img id=\"" + KeyMapping.Key.GetFName().ToString() + "\"/>";
			}
			else // keyboard/mouse mapping
			{
				if(KeyboardMouseIcons != "")
				{
					KeyboardMouseIcons += "/";
				}

				KeyboardMouseIcons += "<img id=\"" + KeyMapping.Key.GetFName().ToString() + "\"/>";				
			}
		}

		KeyboardMouseNamedArguments.Add(AxisName.ToString(), FText::FromString(KeyboardMouseIcons));
		XboxControllerNamedArguments.Add(AxisName.ToString(), FText::FromString(XboxIcons));
	}

	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Magenta, "Finished creating control format arguments");
}

void APlayerCharacter::DetermineControlScheme(const FKey Key)
{
	// TODO: This is not exhaustive
	if(Key.IsGamepadKey())
	{
		CurrentControllerType = ECurrentControllerType::XboxController;
		OnControlSchemeChange.Broadcast();
	}
	else
	{
		CurrentControllerType = ECurrentControllerType::KeyboardMouse;
		OnControlSchemeChange.Broadcast();
	}
}

#pragma endregion 

#pragma region Bookkeeping

EPlayerEquipmentStates APlayerCharacter::GetCurrentPlayerEquipment() const
{
	return CurrentPlayerEquipment;
}

bool APlayerCharacter::GetIsLookingAtLeftHand() const
{
	return IsLookingAtLeftHand;
}

bool APlayerCharacter::GetIsLookingAtRightHand() const
{
	return IsLookingAtRightHand;
}

void APlayerCharacter::TakeHealthDamage()
{
	PlayerHandsMaterialInstance->SetScalarParameterValue(FName("HP_Amount"), PlayerCharacterComponent->GetHealth());
	PlayerHUD->PlayTakeDamageAnimation( GetActorRotation().UnrotateVector(PlayerCharacterComponent->MonsterPawn->GetActorLocation() - GetActorLocation()));
	OnHeartBeat(0.5f);
	
	FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
	if (AudioDevice && GetWorld())
	{
		AudioDevice->PostEvent("PlayTakeDamage", CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
	}
}

void APlayerCharacter::HealDamage()
{
	PlayerHandsMaterialInstance->SetScalarParameterValue(FName("HP_Amount"), PlayerCharacterComponent->GetHealth());
}

void APlayerCharacter::OnDeath()
{
	PlayerHUD->SetFadeOpacity(1.0f);
	FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
	if (AudioDevice && GetWorld())
	{
		AudioDevice->PostEvent("PlayPlayerDeath", FootstepAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
	}
}

void APlayerCharacter::OnEndDeath()
{
	//fade in ui
	PlayerHUD->FadeInFromBlack(1.0f);
}

void APlayerCharacter::PlayMonologue(FString MonologueName, FText SubtitleText)
{
	PlayerHUD->AddSubtitle(SubtitleText, 3.0f);
	FAkAudioDevice* AudioDevice = FAkAudioDevice::Get();
	if (AudioDevice && GetWorld())
	{
		AudioDevice->PostEvent(MonologueName, CameraAudioComponent, 0, NULL, 0, TArray<AkExternalSourceInfo>());
	}
}

#pragma endregion

#pragma region Currently Unused

void APlayerCharacter::StartAimDownSights()
{
	if (CurrentPlayerEquipment == EPlayerEquipmentStates::HoldableObject)
	{
		IsFocusingHoldable = true;
	}
	if (IsLookingAtLeftHand || CurrentPlayerEquipment != EPlayerEquipmentStates::Camera) 
	{
		return;
	}
	//can't sprint while aiming down sights
	EndSprint();
	
	IsLookingAtRightHand = true;
}

void APlayerCharacter::EndAimDownSights()
{
	IsFocusingHoldable = false;
	IsLookingAtRightHand = false;
}

void APlayerCharacter::LerpCamera(const FVector NewPosition)
{
	//setup latent action info
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;

	//move the camera
	UKismetSystemLibrary::MoveComponentTo(PlayerCameraComponent, NewPosition, PlayerCameraComponent->GetComponentRotation(), true, true, .2f, false, EMoveComponentAction::Move, LatentInfo);
}

void APlayerCharacter::UpdateVignette() const
{
	//calculate angle from player's right vector to monster's relative position
	if (PlayerCharacterComponent->MonsterController && PlayerCharacterComponent->MonsterPawn)
	{
		FVector MonsterRelativePosition = PlayerCharacterComponent->MonsterPawn->GetActorLocation() - GetActorLocation();
		
		const float Distance = MonsterRelativePosition.Size();

		//display vignette only if within the max vignette distance
		if (Distance < MaxVignetteDistance) {

			//get unit vectors for calculations
			MonsterRelativePosition.Normalize();
			FVector rightVec = GetActorRightVector();
			rightVec.Z = 0;
			rightVec.Normalize();
			FVector forwardVec = GetActorForwardVector();
			forwardVec.Z = 0;
			forwardVec.Normalize();

			//calculate angles from players forward vector to monster relative position and right vectro to mrp
			const float AngleFromRight = acos(FVector::DotProduct(rightVec, MonsterRelativePosition));
			const float AngleFromForward = acos(FVector::DotProduct(forwardVec, MonsterRelativePosition));

			//calculate theta in degrees to send to the material parameter collection
			float theta = AngleFromRight;
			if (AngleFromForward >= (PI / 2)) { theta = (2 * PI) - AngleFromRight; }
			theta = (theta / PI) * 180;

			const float IntensityMultiplier = 1 - (Distance / MaxVignetteDistance);

			//assign the angle value to the material parameter collection
			if (VignetteParameterCollection) {
				UMaterialParameterCollectionInstance* Instance = GetWorld()->GetParameterCollectionInstance(VignetteParameterCollection);

				Instance->SetScalarParameterValue("Theta", theta);
				Instance->SetScalarParameterValue("IntensityMultiplier", IntensityMultiplier);
			}
		}
		else {
			//do not display vignette
			if (VignetteParameterCollection) {
				UMaterialParameterCollectionInstance* Instance = GetWorld()->GetParameterCollectionInstance(VignetteParameterCollection);
				Instance->SetScalarParameterValue("Theta", 0.0f);
			}
		}
	}
}