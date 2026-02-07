#pragma once

#include <AkAudioDevice.h>
#include <AkComponent.h>
#include <Engine/TextureRenderTarget2D.h>
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

#pragma region Enums

UENUM(BlueprintType)
enum EPlayerMovementStates
{
	Walking,
	Sprinting,
	Sneaking
};

UENUM(BlueprintType)
enum EPlayerEquipmentStates
{
	Camera,
	Folder,
	Radio,
	HoldableObject
};

UENUM(BlueprintType)
enum EPlayerFolderPage
{
	Maps,
	Mystery,
	Mission,
};

UENUM(BlueprintType)
enum ECurrentControllerType
{
	XboxController,
	KeyboardMouse,
};

#pragma endregion

//forward declarations
class AInteractableKey;
class AInteractableMap;

/**
 * The player pawn class for traditional, flatscreen play in TCTB.
 */
UCLASS()
class SPOOKYGAME_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	/**
	 * The Level Manager for the current level
	 */
	UPROPERTY()
	class ALevelManager *LevelManager;
	
	/**
	 * Called every frame
	 * @param DeltaTime Time since the last Tick
	 */
	virtual void Tick(const float DeltaTime) override;

	UFUNCTION()
	void TickFolder(float DeltaTime);

	UFUNCTION()
	void TickCamera(float DeltaTime);
	
	UFUNCTION()
	void TickRadio(float DeltaTime);

#pragma region Setup
public:

	APlayerCharacter();

	/**
	 * Handles most player character functionality not unique to flatscreen
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UPlayerCharacterComponent* PlayerCharacterComponent;
	
	/**
	 * Player's head POV camera
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraComponent* PlayerCameraComponent;

	/**
	 * Rigged mesh containing both players hands
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class USkeletalMeshComponent* MasterPlayerRiggedMesh;

	/**
	 * Rigged mesh for the player's polaroid camera
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class USkeletalMeshComponent* CameraSkeletalMesh;

	/**
	 * Rigged mesh for the polaroid photo (distinct from PhotoMesh, which is a plane that rendters the photo
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class USkeletalMeshComponent* PhotoSkeletalMesh;

	/**
	 * Called to bind functionality to inputs
	 * @param PlayerInputComponent The component managing player input
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/**
	 * Called when the game starts or when spawned
	 */
	virtual void BeginPlay() override;

#pragma endregion	

#pragma region Movement
public:

	/**
	 * Max player movement speed while walking
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed = 300;
	/**
	 * Max player movement speed while sprinting
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SprintMaxWalkSpeed = 600;
	/**
	 * Max player movement speed while sneaking
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SneakMaxWalkSpeed = 150;

	/**
	 * Audio source for foot-sourced sfx
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UAkComponent* FootstepAudioComponent;

	/**
	 * Event name to play a "sneak" footstep on hard wood floor
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	FString AkEvent_Footstep_Sneak_Hardwood;
	/**
	 * Event name to play a "walk" footsteop on hard wood floor
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	FString AkEvent_Footstep_Walk_Hardwood;
	/**
	 * Event name to play a "sprint" footstep on hard wood floor
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	FString AkEvent_Footstep_Sprint_Hardwood;
	
	/**
	 *	Default position of the player's camera (when not crouched)
	 */
	UPROPERTY()
	FVector CameraDefaultPosition;

	/**
	 *	Position of the player's head camera when crouched
	 */
	UPROPERTY()
	FVector CameraCrouchPosition;

	/**
	 * Gets input look-left and look-up in x and y fields
	 */
	UFUNCTION(BlueprintCallable)
	FVector GetLookMovement() const;

	/**
	 * Get player's input/attempted walk movmement in worldspace
	 * @param bIsLocal 
	 * @return 
	 */
	UFUNCTION(BlueprintCallable)
	FVector GetWalkMovement(const bool bIsLocal = false) const;

	/**
	 *	Moves player head to a new position. Used for crouching.
	 */
	UFUNCTION()
	void LerpCamera(const FVector NewPosition);

protected:
	
	/**
	 * Player's current movement state
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TEnumAsByte<EPlayerMovementStates> PlayerMovementState;

	/**
	 * Maximum stamina the player can have
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxStamina = 5;
	
	/**
	 * Current stamina
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Stamina = 5;
	
	/**
	 * Time in seconds since the last time the player was sprinting
	 */
	UPROPERTY()
	float TimeSinceSprinting;

	/**
	 * Time since last footstep scaled each frame by the movement speed of the player. Used to prevent duplicate footstep events from firing in a short window.
	 */
	float ScaledTimeSinceLastFootstep;

	/**
	 * Move forward or backward based on the direction the player is facing
	 * @param Value Input value from axis
	 */
	UFUNCTION()
	void MoveForward(float Value);
	/**
	 * Move right or left based on the direction the character is facing
	 * @param Value Input value from axis
	 */
	UFUNCTION()
	void MoveRight(const float Value);

	/**
	 * Put the player in sprint mode
	 */
	UFUNCTION()
	void StartSprint();
	/**
	 * Take the player out of sprint mode
	 */
	UFUNCTION()
	void EndSprint();

	/**
	 * Put the player in and out of sneak mode (crouch)
	 */
	UFUNCTION()
	void ToggleSneak();

	/**
	 * Sets the walk speed of the player character based on the direction the character is facing
	 */
	void SetWalkSpeed() const;

	/**
	 * Plays footstep and performs associated functionality (like notifying monster) when called
	 */
	UFUNCTION(BlueprintCallable)
	void PlayWalkingSound();

#pragma endregion	

#pragma region Polaroid

#pragma region Polaroid - Motion Blur
	/**
	 * This value multiplies with the player movement vector to get the motion blur vector
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CameraMotionBlur_MovementScalar;

	/**
	 * This value multiplies with the player look vector to get the motion blur vector
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CameraMotionBlur_LookScalar;

	/**
	 * If the magnitude of the player's look motion is >= to this value, it is used as the direction of the blur
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CameraMotionBlur_LookOverrideMagnitude;
#pragma endregion

#pragma region Polaroid - Ghost Sensor
public:
	//--------| Supernatural sensor
	/**
	 * Light whose intensity signals nearby supernatural objects - CURRENTLY USED TO INDICATE CHARGED PHOTO
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UPointLightComponent* SupernaturalSensor;
	
	/**
	 * Spark particles for the supernatural sensor. TODO : Currently unused, but we'll likely want particle effects for the camera soon.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UParticleSystemComponent* SensorParticleSystem;

#pragma endregion

public:
	/**
	 * Polaroid camera's game-camera. Used to capture the scene for photographs
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USceneCaptureComponent2D* HandheldCameraCapture;

	/**
	 *	Used for camera zoom until we have a proper camera rig
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* CameraFrontPlate_Temp;

	/**
	 * Mesh for the plane that the photo is rendered to
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* PhotoMesh;

	//--------| Flash
	/**
	 * The flash on the player's polaroid camera
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class USpotLightComponent* HandheldCameraFlash;
	
	//--------| Audio
	/**
	 * Audio source for camera-sourced sfx
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UAkComponent* CameraAudioComponent;

protected:
	/**
	 *	Start all functions for aiming down sights
	 */
	UFUNCTION()
	void StartAimDownSights();

	/**
	 *	Start all functions for returning to normal after aiming down sights
	 */
	UFUNCTION()
	void EndAimDownSights();

	/**
	 * Called when input for taking photo detected, filters state before calling TakePhoto on 
	 * PlayerCharacterComponent.
	 */
	UFUNCTION()
	void TakePhoto();

	/**
	 * Called when input for charging the camera is detected. Does some filtering before calling 
	 * the same function on PlayerCharacterComponent.
	 */
	UFUNCTION()
	void StartChargingCamera();

#pragma endregion	
	
#pragma region Interaction
public:
	
	/**
	 * The interactable object currently within the player's range
	 */
	UPROPERTY()
	class AInteractableObject* CurrentInteractable;

	/**
	 *	Whether player is currently holding a holdable, and is also "focused" on it - rotating it and moving it closer + further
	 */
	bool IsFocusingHoldable = false;
	
	/**
	 * Whether player is currently interacting with a door - dampens angular movment
	 * Controlled by SetControllingDoor()
	 */
	bool IsControllingDoor = false;

	/**
	 * Set whether the player is controlling the door
	 */
	UFUNCTION()
	void SetControllingDoor(const bool IsControlling);

protected:

	UPROPERTY(EditAnywhere)
	float HoldableThrowMagnitude = 500000.0f;

	UPROPERTY(EditAnywhere)
	float HoldableKickSneakMagnitude = 50000.0f;

	UPROPERTY(EditAnywhere)
	float HoldableKickWalkMagnitude = 100000.0f;

	UPROPERTY(EditAnywhere)
	float HoldableKickRunMagnitude = 500000.0f;

	/**
	 * Holdable objects are "kicked" when the player walks into them.
	 * @param Holdable 
	 * @param HitLocation 
	 */
	UFUNCTION(BlueprintCallable)
	void KickHoldable(class AInteractableHoldable* Holdable, FVector HitLocation);
	
	/**
	 * Check for interactable objects in front of the player
	 */
	void InteractionCheck();

	/**
	 * Interact with the current target interactable object
	 */
	void Interact();

	/**
	 * Runs when the player stops holding the interact button
	 */
	void ReleaseInteract();

	/**
	 * Override in order to change magnitude for interaction
	 * @param Val Value of yaw input received from controller
	 */
	virtual void AddControllerYawInput(const float Val) override;

	/**
	 * Override in order to change magnitude for interaction
	 * @param Val Value of pitch input received from controller
	 */
	virtual void AddControllerPitchInput(const float Val) override;

#pragma endregion
	
#pragma region UI
public:
	
	/**
	 * The widget class to use for the player HUD
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UUserWidget> PlayerHUDClass;

	/**
	 * The player's HUD
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UPlayerHUD* PlayerHUD;

	/**
	 * The widget class to use for the pause menu
	 */	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UUserWidget> PauseMenuClass;
	
	/**
	 * The pause menu
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UUserWidget* PauseMenu;
	
	/**
	 * @brief Runs when the player presses pause. Input should be setup to execute while the game is paused
	 */
	UFUNCTION()
	void OnPressPause();

protected:
	/**
	 * Material instance which renders the players hands. Modified at runtime to display player HP
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* PlayerHandsMaterialInstance;
	
	/**
	 * Hides all equipment that can be held in the left hand
	 */
	void PutAwayAllLeftHandEquipment() const;

	/**
	 * Look closely at whatever is in the player's left hand (polaroid or folder)
	 */
	void LookCloselyAtLeftHand();

	// --- Control Reminders
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Reminders")
	TArray<FText> ControlReminders_CameraHip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Reminders")
	TArray<FText> ControlReminders_CameraAim;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Reminders")
	TArray<FText> ControlReminders_CameraLookAtPhoto;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Reminders")
	TArray<FText> ControlReminders_FolderHip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Reminders")
	TArray<FText> ControlReminders_FolderLookClose;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Reminders")
	TArray<FText> ControlReminders_RadioHip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Reminders")
	TArray<FText> ControlReminders_InteractableAlarmClock;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Reminders")
	TArray<FText> ControlReminders_InteractableDoor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Reminders")
	TArray<FText> ControlReminders_InteractableHoldable;

#pragma endregion	

#pragma region Folder
public:
	/**
	 * Mesh for folder UI to display on
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	USkeletalMeshComponent* FolderUISkeletalMesh;

	/**
	 * Widget component for rendering Folder
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UWidgetComponentOverride* FolderWidgetComponent;
	
	// Folder UI input handling functions
	UFUNCTION()
	void FolderSelect();
	UFUNCTION()
	void FolderNavigateUp();
	UFUNCTION()
	void FolderNavigateDown();

	/**
	 * Handles context-sensitive changes for folder UI based on current page (cycle through notes, maps, or polaroids)
	 * - called by navigate up & down
	 * @param bIsMoveUp True if player is traversing "upward"
	 */
	void FolderVerticalInput(const bool bIsMoveUp) const;

	//===========| Animation helper members
	/**
	 * Used primarily by animations - immediately after changing what the player is holding, this is false until
	 * the "transition away" animation is complete, at which point it becomes true
	 *
	 * This specifically is used to let the folder know when to begin playing its transition animation
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool HasSwappedBetweenFolderAndCamera;

	/**
	 * Called by animations & controls HasSwappedBetweenFolderAndCamera - toggles the visibility of objects the
	 * player holds at the point when the player's hands are offscreen
	 */
	UFUNCTION(BlueprintCallable)
	void SwapFromFolderToCamera();

	/**
	 * Called by animations & controls HasSwappedBetweenFolderAndCamera - toggles the visibility of objecs the
	 * player holds at the point when the player's hands are offscreen
	 */
	UFUNCTION(BlueprintCallable)
	void SwapFromCameraToFolder();

	/**
	* Called by animations & controls HasSwappedBetweenFolderAndCamera - toggles the visibility of objecs the
	* player holds at the point when the player's hands are offscreen
	*/
	UFUNCTION(BlueprintCallable)
	void SwapToRadio();

	/**
	 * Takes input relating to pan/zoom in folder and passes it to Folder UI to handle
	 */
	UFUNCTION()
	void HandleFolderPanInput() const;

protected:
	/**
	 * Material instance which renders the folder UI on the folder mesh. Not currently used since we only have one page, but may be useful if we want to change folder material params in the future.
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* FolderMaterialInstance;
	
	/**
	 * Runs when the player presses the button to open or close the folder
	 */
	void OnPressFolderButton();

	/**
	* Open or close the player's folder - SPECIFICALLY from a pure state/gameplay perspective. Does NOT
	* control the visibility of either, though it does initiate the animation transition
	 */
	UFUNCTION()
	void SetShowingFolder(const bool Show);

#pragma endregion

#pragma region Radio
public:

	/**
	* Placeholder radio mesh
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* RadioStaticMesh;

	/**
	 * Plane that renders the histogram readout
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* RadioHistogramReadoutMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UAkComponent* RadioAkAudio;
	
	/**
	* Signal direction + strength readout plane
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Radio")
	UStaticMeshComponent* RadioSignalReadoutStaticMesh;

	/**
	*	Rate at which frequency changes when tune input is maxed out. Units are "full scale per second".
	*	i.e. with a value of 0.5, if player starts at lowest value and climbs frequency, they will be at
	*	the halfway point in one second.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Radio")
	float RadioFrequencyTuneRate = 0.2f;
	
	/**
	 *	Turns the frequency lower (negative) or higher (positive). Scaled by deltatime and RadioFrequencyTuneRate.
	 */
	UFUNCTION()
	void TuneRadioFrequencyUp(float Value);

	// --- Fields relating to the minigame that plays while listening to a radio sound

protected:
		
	/**
	 *  Whether the player is holding "play/pause" toggle. Avoids toggling each tick, only.  
	 */
	UPROPERTY()
	bool IsRadioHoldingPlay = false;
	
	/**
	 * Runs when the player presses the button to take out their radio
	 */
	void OnPressRadioButton();

	/**
	* Open or close the player's radio - SPECIFICALLY from a pure state/gameplay perspective. Does NOT
	* control the visibility of either, though it does initiate the animation transition
	 */
	UFUNCTION()
	void SetShowingRadio(const bool Show);

#pragma endregion 

#pragma region Bookkeeping
public:
		
	/**
	 * Gets what the player currently has equipped
	 * @return Player's current equipment
	 */
	UFUNCTION(BlueprintCallable)
	EPlayerEquipmentStates GetCurrentPlayerEquipment() const;

	/**
	 * Whether the player is looking closely at their left hand equipment - polaroid or folder
	 */
	UFUNCTION(BlueprintCallable)
	bool GetIsLookingAtLeftHand() const;

	//TODO: how does this affect radio?
	/**
	 * Returns whether the player is aiming or not. Currently only really "aiming" since the camera is the only right-hand
	 * gear but for scalability this name is appropriate
	 */
	UFUNCTION(BlueprintCallable)
	bool GetIsLookingAtRightHand() const;

	/**
	 * Take damage to the player's health
	 */
	UFUNCTION()
	void TakeHealthDamage();

	/**
	 * Heal the player's health if the player is not at full health
	 */
	UFUNCTION()
	void HealDamage();
	
	/**
	 * Called the moment the player dies
	 */
	UFUNCTION()
	void OnDeath();

	/**
	 * Called when the death state is over and the player is "back alive"
	 */
	UFUNCTION()
	void OnEndDeath();

	UFUNCTION(BlueprintCallable)
	void PlayMonologue(FString MonologueName, FText SubtitleText);

protected:

	/**
	 * Whether the player is currently looking closely at what is in their left hand
	 */
	bool IsLookingAtLeftHand = false;

	/**
	 * Whether the player is currently looking closely at what is in thier right hand (usually a camera)
	 * Righ now this is essentially IsADS
	 */
	bool IsLookingAtRightHand = false;

	//TODO: needs some renaming obviously
	/**
	 * What equipment is the player currently holding in both hands
	 */
	UPROPERTY()
	TEnumAsByte<EPlayerEquipmentStates> CurrentPlayerEquipment;

	/**
	*	What equipment player was previously holding which should be switched back to. Currently
	*	used specifically for holdables - when player stops holding object, should reopen whatever
	*	tool they were using prior to picking up the object
	*/
	UPROPERTY()
	TEnumAsByte<EPlayerEquipmentStates> PreviousPlayerEquipment;

	// --- Heartbeat pulse management
	
	UPROPERTY()
	float HeartbeatTimer = 0.0f;

	/**
	 *	If player is within this many cm of the monster, and is at 1HP, monster will pulse visibility 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HeartbeatMonsterPulseProximity = 700.0f;
	
	/**
	 *	Duration of the time between heartbeat pairs
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HeartbeatDuration_Long = 0.9f;

	/**
	 *	Duration of the time between individual thumbs of a pair of beats
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HeartbeatDuration_Short = 0.3f;

	bool IsShortHeartbeatNext = false;

	bool IsPlayingBackgroundHeartbeat = false;

	UFUNCTION(BlueprintImplementableEvent)
	void OnHeartBeat(float DurationOfVFX);

	UFUNCTION()
	void LowHealthTickBehavior(float DeltaTime);
	
#pragma endregion

#pragma region Control Schemes
public:
	
	/**
	 * Event to broadcast when the player detects that the control scheme has changed
	 */
	DECLARE_EVENT(APlayerCharacter, FEventControlSchemeChange)
	FEventControlSchemeChange OnControlSchemeChange;
	
	/**
	 * Gets the appropriate set of named arguments based on the current control scheme
	 * @return The set of named arguments
	 */
	FFormatNamedArguments GetCurrentControlSchemeNamedArguments() const;
	
protected:
	
	/**
	 * The most recent control scheme used
	 */
	ECurrentControllerType CurrentControllerType = ECurrentControllerType::XboxController;
	/**
	 * Bound input to icon mapping for xbox controller
	 */
	FFormatNamedArguments XboxControllerNamedArguments;
	/**
	 * Bound input to icon mapping for keyboard and mouse
	 */
	FFormatNamedArguments KeyboardMouseNamedArguments;
	
	/**
	 * Creates the FFormatNamedArguments for converting bound inputs to icons
	 */
	void CreateControlFormatArguments();
	
	/**
	 * Determines the most recently used control scheme based on a key
	 * @param Key The most recently used key (passed in by bluepritns) TODO: This needs to be changed as it doesn't work for mouse movement or gamepad sticks
	 */
	UFUNCTION(BlueprintCallable)
	void DetermineControlScheme(const FKey Key);	
	
#pragma endregion 
	
//TODO: obviously some of this needs to be killed, but some of it is good logic that we migh reuse at some future point (ex: vignette might be used for accessibility
#pragma region Currently Unused
public:

	/// <summary>
	/// Include reference to material collection for vignette
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialParameterCollection* VignetteParameterCollection;

	/// <summary>
	/// Max distance away the monster can be for having the vignette appear
	/// </summary>
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxVignetteDistance = 3000;

protected:
	/**
	 *	Update the monster danger vignette. Keeping this around cuz there's some good math here, and we may eventually want to reintroduce screenspace directional sensing?
	 */
	void UpdateVignette() const;	
#pragma endregion
};