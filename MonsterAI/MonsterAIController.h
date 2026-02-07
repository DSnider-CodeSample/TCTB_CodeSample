// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MonsterAIController.generated.h"

UCLASS()
class SPOOKYGAME_API AMonsterAIController : public AAIController
{
	GENERATED_BODY()

#pragma region Desired AI Values
protected:
	UPROPERTY(EditAnywhere)
	float DesiredWanderRadius = 2000.0f;
	UPROPERTY(EditAnywhere)
	float DesiredWanderBiasRadius = 1200.0f;
	UPROPERTY(EditAnywhere)
	float DesiredSearchRadius = 600.0f;
	UPROPERTY(EditAnywhere)
	float DesiredWalkSpeed = 100.0f;
	UPROPERTY(EditAnywhere)
	float DesiredRunSpeed = 200.0f;
	UPROPERTY(EditAnywhere)
	float DesiredPursueInsteadOfSearchRadius = 400.0f;
	UPROPERTY(EditAnywhere)
	float DesiredSearchDuration = 30.0f;
#pragma endregion
	
protected:
	/**
	 * The blackboard component used by the BehaviorTree the monster runs on
	 */
	UPROPERTY(Transient)
	UBlackboardComponent* BlackboardComponent;

	/**
	 * The BehaviorTree the monster runs on
	 */
	UPROPERTY(Transient)
	class UBehaviorTreeComponent* BehaviorTreeComponent;

	/**
	 * List of locations that the monster can "run away" to
	 * These are designer placed objects that are in convenient locations
	 */
	TArray<FVector> RunAwayLocations;

	/**
	 * Reference to current player's playercomponent. 
	 */
	UPROPERTY()
	class UPlayerCharacterComponent* Player;

#pragma region Blackboard Keys
public:
	uint8 StateKeyID;
	uint8 TargetLocationKeyID;
	uint8 WanderRadiusKeyID;
	uint8 WanderBiasStartRadiusKeyID;
	uint8 TimeSincePursueKeyID;
	uint8 GoToPlayerTotalTimeKeyID;
	uint8 WalkSpeedKeyID;
	uint8 HowLongToGoToPlayerKeyID;
	uint8 PursueInsteadOfSearchRadiusKeyID;
	uint8 SearchCenterPointKeyID;
#pragma endregion

public:
	AMonsterAIController();
	
	virtual void Tick(float DeltaTime) override;

	/// <summary>
	/// Additional logic to execute after this controller has possessed the monster character
	/// </summary>
	/// <param name="InPawn">The monster character being possesed</param>
	;
	/**
	 * Additional logic to execute after this controller has possessed the monster character
	 * @param InPawn The monster character being possesed
	 */
	virtual void OnPossess(APawn* InPawn) override;

	/**
	 * Report a sound to the monster controller
	 * @param Origin Origin point of the sound
	 * @param HearableRadius Radius around the origin point within which the monster should be able to hear it
	 * @param IsOngoing True if the sound is continuously playing. Defaults to false - a one off sound
	 * @param IsAudioLog True if the sound is an "audio log", allows for the monster to react specially to hearing "john's" voice TODO: I don't think this is used
	 * @param HowLongToGoToPlayer Time in seconds to directly purse the player after hearing this sound
	 * @param bOverrideSafeZone If true, the monster will pursue the player even if they are in a safe zone after hearing the sound
	 */
	virtual void ReportSound(FVector Origin, float HearableRadius, bool IsOngoing = false, bool IsAudioLog = false, float HowLongToGoToPlayer = -1.0f, bool bOverrideSafeZone = false);
	
	/**
	 * Set the monster to follow the player's position for a set amount of time
	 */
	virtual void SetFollowPlayer();

	/**
	 * Reset AI when player dies. Called from Monster.cpp
	 */
	virtual void AIOnPlayerDeath();

	/**
	 * Activates the monster for the first time
	 */
	UFUNCTION(BlueprintCallable)
	virtual void ActivateMonster();

	/**
	 * Returns the list of "runaway locations"
	 * @return The list
	 */
	TArray<FVector> GetRunAwayLocations() { return RunAwayLocations; }

	FVector GetClosestRunawayLocation();
	FVector GetFarthestRunawayLocation();
	FVector GetFarthestRunawayLocationFromPlayer();

	/**
	 * Returns the monster's current state. 
	 * @return The monster's state as a uint8, can be assigned to the monster state enum
	 */
	uint8 GetMonsterCurrentState() const;

#pragma region Get Desired AI Values
public:
		float GetDesiredWanderRadius() const { return DesiredWanderRadius; }
		float GetDesiredWanderBiasRadius() const { return DesiredWanderBiasRadius; }
		float GetDesiredSearchRadius() const { return DesiredSearchRadius; }
		float GetDesiredWalkSpeed() const { return DesiredWalkSpeed; }
		float GetDesiredRunSpeed() const { return DesiredRunSpeed; }
		float GetDesiredPursueInsteadOfSearchRadius() const { return DesiredPursueInsteadOfSearchRadius; }
		float GetDesiredSearchDuration() const { return DesiredSearchDuration; }
#pragma endregion
};
