// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterAIController.h"
#include "DrawDebugHelpers.h"
#include "PlayerCharacterComponent.h"
#include "Monster.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyAllTypes.h"
#include <MonsterAI/MonsterStates.h>
#include "Engine/Engine.h"
#include "MonsterRunAwayLocation.h"
#include "Kismet/GameplayStatics.h"

AMonsterAIController::AMonsterAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
}

void AMonsterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	//get the monster character this controller is controlling
	const AMonster* Monster = Cast<AMonster>(InPawn);

	//get runaway locations
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMonsterRunAwayLocation::StaticClass(), FoundActors);
	for (const AActor* FoundActor : FoundActors)
	{
		RunAwayLocations.Add(FoundActor->GetActorLocation());
	}


	if (Monster && Monster->MonsterBehavior)
	{
		//set up the BlackboardComponent
		BlackboardComponent->InitializeBlackboard(*Monster->MonsterBehavior->BlackboardAsset);

		//get KeyIDs for the Blackboard variables
		StateKeyID = BlackboardComponent->GetKeyID("State");
		TargetLocationKeyID = BlackboardComponent->GetKeyID("TargetLocation");
		WanderRadiusKeyID = BlackboardComponent->GetKeyID("WanderRadius");
		WanderBiasStartRadiusKeyID = BlackboardComponent->GetKeyID("WanderBiasStartRadius");
		TimeSincePursueKeyID = BlackboardComponent->GetKeyID("TimeSincePursue");
		GoToPlayerTotalTimeKeyID = BlackboardComponent->GetKeyID("GoToPlayerTotalTime");
		WalkSpeedKeyID = BlackboardComponent->GetKeyID("WalkSpeed");
		HowLongToGoToPlayerKeyID = BlackboardComponent->GetKeyID("HowLongToGoToPlayer");
		PursueInsteadOfSearchRadiusKeyID = BlackboardComponent->GetKeyID("PursueInsteadOfSearchRadius");
		SearchCenterPointKeyID = BlackboardComponent->GetKeyID("SearchCenterPoint");

		//setup ai properties with defaults
		BlackboardComponent->SetValue<UBlackboardKeyType_Float>(WanderRadiusKeyID, DesiredWanderRadius);
		BlackboardComponent->SetValue<UBlackboardKeyType_Float>(WanderBiasStartRadiusKeyID, DesiredWanderBiasRadius);
		BlackboardComponent->SetValue<UBlackboardKeyType_Float>(WalkSpeedKeyID, DesiredWalkSpeed);
		BlackboardComponent->SetValue<UBlackboardKeyType_Float>(PursueInsteadOfSearchRadiusKeyID, DesiredPursueInsteadOfSearchRadius);
		
		//setup ai properties
		//BlackboardComponent->SetValue<UBlackboardKeyType_Enum>(StateKeyID, EGurneyMonsterStates::GMS_Wander);
		BlackboardComponent->SetValue<UBlackboardKeyType_Enum>(StateKeyID, EGurneyMonsterStates::GMS_Inactive);
		//BlackboardComponent->SetValue<UBlackboardKeyType_Vector>(TargetLocationKeyID, GetPawn()->GetActorLocation());
		FVector test(-2400, 95.5, -363);
		BlackboardComponent->SetValue<UBlackboardKeyType_Vector>(TargetLocationKeyID, test);
		BlackboardComponent->SetValue<UBlackboardKeyType_Float>(TimeSincePursueKeyID, -1);
		BlackboardComponent->SetValue<UBlackboardKeyType_Float>(GoToPlayerTotalTimeKeyID, 0);
		BlackboardComponent->SetValue<UBlackboardKeyType_Float>(HowLongToGoToPlayerKeyID, 0);
		BlackboardComponent->SetValue<UBlackboardKeyType_Vector>(SearchCenterPointKeyID, GetPawn()->GetActorLocation());
		
		//begin the behavior tree
		BehaviorTreeComponent->StartTree(*Monster->MonsterBehavior);

		// Play wander groan
		//Monster->PlayMonsterSound("PlayMonsterIdleLoop");
	}
}

void AMonsterAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//debug printing
	const AMonster* Monster = Cast<AMonster>(GetPawn());
	if (Monster->drawDebugInfo)
	{
		//target location
		DrawDebugSphere(GetWorld(), BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(TargetLocationKeyID), 50, 4, FColor::Blue, false, -1.0f, 0, 2);	
		
		//state
		FString state;
		switch (BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID))
		{
		case EGurneyMonsterStates::GMS_Wander:
			state = "Current monster AI state: Wander";
			//wander radius
			//DrawDebugSphere(GetPawn()->GetWorld(), GetPawn()->GetActorLocation(), BlackboardComponent->GetValue<UBlackboardKeyType_Float>(WanderRadiusKeyID), 13, FColor::Emerald, false, -1.0f, 0, 2);
			break;
		case EGurneyMonsterStates::GMS_Pursue:
			state = "Current monster AI state: Pursue";
			break;
		case EGurneyMonsterStates::GMS_GoToPlayer:
			state = "Current monster AI state: Go To Player";
			break;
		case EGurneyMonsterStates::GMS_Search:

			DrawDebugSphere(GetWorld(), BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(SearchCenterPointKeyID), 50, 4, FColor::Orange, false, -1.0f, 0, 2);
			GEngine->AddOnScreenDebugMessage(-1, -1.0f, FColor::Orange, "Search Center Point: (" + FString::SanitizeFloat(BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(SearchCenterPointKeyID).X) +
				", " + FString::SanitizeFloat(BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(SearchCenterPointKeyID).Y) +
				", " + FString::SanitizeFloat(BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(SearchCenterPointKeyID).Z) + ")");
			state = "Current monster AI state: Search";
			break;
		case EGurneyMonsterStates::GMS_ListenToLog:
			state = "Current monster AI state: ListenToLog";
			break;
		case EGurneyMonsterStates::GMS_Inactive:
			state = "Current monster AI state: Inactive";
			break;
		default:
			break;
		}
		GEngine->AddOnScreenDebugMessage(-1, -1.0f, FColor::Orange, state);

		// time since pursue
		GEngine->AddOnScreenDebugMessage(-1, -1.0f, FColor::Orange, "Time since pursue: " + FString::SanitizeFloat(BlackboardComponent->GetValue<UBlackboardKeyType_Float>(TimeSincePursueKeyID)));
		GEngine->AddOnScreenDebugMessage(-1, -1.0f, FColor::White, "Target Location: (" + FString::SanitizeFloat(BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(TargetLocationKeyID).X) + 
																	", " + FString::SanitizeFloat(BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(TargetLocationKeyID).Y) + 
																	", " + FString::SanitizeFloat(BlackboardComponent->GetValue<UBlackboardKeyType_Vector>(TargetLocationKeyID).Z) + ")");
	}
}

void AMonsterAIController::ReportSound(FVector Origin, float HearableRadius, bool IsOngoing /*= false*/, bool IsAudioLog /*= false*/, float HowLongToGoToPlayer /*= -1.0f*/, bool bOverrideSafeZone /*= false*/)
{
	//don't respond to sounds while inactive
	if (BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) == EGurneyMonsterStates::GMS_Inactive) { return; }

	//todo react to audio log
	//this might need to be moved so that it has to hear it before reacting
	if (IsAudioLog) { return; }

	//if player is safe and this sound isn't specifically set to ignore safe zones, don't respond
	if (!Player)
	{
		Player = Cast<UPlayerCharacterComponent>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetComponentByClass(UPlayerCharacterComponent::StaticClass()));
	}
	if (Player)
	{
		if (Player->IsPlayerProtectedBySafetyVolume())
		{
			return;
		}
	}

	//debug printing
	const AMonster* Monster = Cast<AMonster>(GetPawn());
	if (Monster->drawDebugInfo)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Orange, "Sound Reported and not early return");
	}

	//calculate distance to sound
	const float VerticalDistance = FMath::Abs(Origin.Z - GetPawn()->GetActorLocation().Z) * 2; //vertical distance counts double
	Origin.Z = GetPawn()->GetActorLocation().Z; //remove Z from further calculations
	const float Distance = VerticalDistance + FVector::Distance(Origin, GetPawn()->GetActorLocation());

	//debug stuff
	const AMonster* Pawn = Cast<AMonster>(GetPawn());
	if (Pawn->drawDebugInfo)
	{
		DrawDebugSphere(GetPawn()->GetWorld(), GetPawn()->GetActorLocation(), HearableRadius, 15, FColor::Red, false, 1.5f, 0, 2);
		DrawDebugSphere(GetPawn()->GetWorld(), GetPawn()->GetActorLocation(), BlackboardComponent->GetValue<UBlackboardKeyType_Float>(PursueInsteadOfSearchRadiusKeyID), 15, FColor::Green, false, 1.5f, 0, 2);
	}

	if (Distance < HearableRadius)
	{
		if (Distance < BlackboardComponent->GetValue<UBlackboardKeyType_Float>(PursueInsteadOfSearchRadiusKeyID))
		{
			//follow player if told to
			if (HowLongToGoToPlayer > 0)
			{
				//set monster to pursue the player
				if (BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_GoToPlayer &&
					BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_Pursue)
				{
					// play transition sound
					AMonster* MonsterPawn = Cast<AMonster>(GetPawn());
					MonsterPawn->PlayMonsterSound("PlayMonsterDetected");
				}
				BlackboardComponent->SetValue<UBlackboardKeyType_Enum>(StateKeyID, EGurneyMonsterStates::GMS_GoToPlayer);
				BlackboardComponent->SetValue<UBlackboardKeyType_Float>(HowLongToGoToPlayerKeyID, HowLongToGoToPlayer);
				BlackboardComponent->SetValue<UBlackboardKeyType_Float>(GoToPlayerTotalTimeKeyID, 0);
			}
			// don't respond to sound if chasing player directly
			else if (BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_GoToPlayer)
			{
				//set monster to pursue the sound
				if (BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_Pursue
					&& BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_GoToPlayer)
				{
					// play transition sound
					AMonster* MonsterPawn = Cast<AMonster>(GetPawn());
					MonsterPawn->PlayMonsterSound("PlayMonsterDetected");
				}
				BlackboardComponent->SetValue<UBlackboardKeyType_Enum>(StateKeyID, EGurneyMonsterStates::GMS_Pursue);
				BlackboardComponent->SetValue<UBlackboardKeyType_Vector>(TargetLocationKeyID, Origin);
			}
		}

		//dont go into search mode while in a more agressive mode
		if (BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_GoToPlayer &&
			BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_Pursue)
		{
			if (BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_Search)
			{
				AMonster* Monster = Cast<AMonster>(GetPawn());
				Monster->PlayMonsterSound("PlayMonsterSearchLoop");
			}

			//investigate the general area
			BlackboardComponent->SetValue<UBlackboardKeyType_Enum>(StateKeyID, EGurneyMonsterStates::GMS_Search);
			BlackboardComponent->SetValue<UBlackboardKeyType_Float>(WanderRadiusKeyID, DesiredSearchRadius);
			BlackboardComponent->SetValue<UBlackboardKeyType_Vector>(TargetLocationKeyID, GetPawn()->GetActorLocation());
			BlackboardComponent->SetValue<UBlackboardKeyType_Float>(TimeSincePursueKeyID, 0.0f);
			BlackboardComponent->SetValue<UBlackboardKeyType_Vector>(SearchCenterPointKeyID, Origin);

		}
	}
}

void AMonsterAIController::SetFollowPlayer()
{
	if (BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_GoToPlayer &&
	BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) != EGurneyMonsterStates::GMS_Pursue)
	{
		// play transition sound
		AMonster* Monster = Cast<AMonster>(GetPawn());
		Monster->PlayMonsterSound("PlayMonsterDetected");
	}
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Purple, "SetFollowPlayer");
	BlackboardComponent->SetValue<UBlackboardKeyType_Enum>(StateKeyID, EGurneyMonsterStates::GMS_GoToPlayer);
	BlackboardComponent->SetValue<UBlackboardKeyType_Float>(GoToPlayerTotalTimeKeyID, 0);
}

void AMonsterAIController::AIOnPlayerDeath()
{
	BlackboardComponent->SetValue<UBlackboardKeyType_Enum>(StateKeyID, EGurneyMonsterStates::GMS_Wander);
	BlackboardComponent->SetValue<UBlackboardKeyType_Float>(WanderRadiusKeyID, DesiredWanderRadius);
	BlackboardComponent->SetValue<UBlackboardKeyType_Vector>(TargetLocationKeyID, GetPawn()->GetActorLocation());
	BlackboardComponent->SetValue<UBlackboardKeyType_Float>(WanderBiasStartRadiusKeyID, DesiredWanderBiasRadius);
	BlackboardComponent->SetValue<UBlackboardKeyType_Float>(TimeSincePursueKeyID, -1);
	BlackboardComponent->SetValue<UBlackboardKeyType_Float>(GoToPlayerTotalTimeKeyID, 0);
}

void AMonsterAIController::ActivateMonster()
{
	if (BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID) == EGurneyMonsterStates::GMS_Inactive)
	{ 
		// play transition sound
		AMonster* Monster = Cast<AMonster>(GetPawn());
		Monster->PlayMonsterSound("PlayMonsterDetected");
		BlackboardComponent->SetValue<UBlackboardKeyType_Enum>(StateKeyID, EGurneyMonsterStates::GMS_Wander);
		BlackboardComponent->SetValue<UBlackboardKeyType_Float>(GoToPlayerTotalTimeKeyID, 0);
	}
	else
	{
		//set status and new target
		SetFollowPlayer();
	}
}

uint8 AMonsterAIController::GetMonsterCurrentState() const
{
	return BlackboardComponent->GetValue<UBlackboardKeyType_Enum>(StateKeyID);
}

FVector AMonsterAIController::GetClosestRunawayLocation()
{
	const FVector MonsterLocation = GetPawn()->GetActorLocation();

	if (RunAwayLocations.Num() < 1) { return GetPawn()->GetActorLocation(); }

	FVector Location = MonsterLocation;
	float Distance = INT_MAX;
	for (int i = 0; i < RunAwayLocations.Num(); ++i)
	{
		const float NewDistance = FVector::Distance(MonsterLocation, RunAwayLocations[i]);
		if (NewDistance < Distance && NewDistance > 50)
		{
			Location = RunAwayLocations[i];
			Distance = NewDistance;
		}
	}

	return Location;
}

FVector AMonsterAIController::GetFarthestRunawayLocation()
{
	const FVector MonsterLocation = GetPawn()->GetActorLocation();

	if (RunAwayLocations.Num() < 1) { return GetPawn()->GetActorLocation(); }

	FVector Location = RunAwayLocations[0];
	float Distance = 0;
	for (int i = 0; i < RunAwayLocations.Num(); ++i)
	{
		const float NewDistance = FVector::Distance(MonsterLocation, RunAwayLocations[i]);
		if (NewDistance > Distance)
		{
			Location = RunAwayLocations[i];
			Distance = NewDistance;
		}
	}

	return Location;
}

FVector AMonsterAIController::GetFarthestRunawayLocationFromPlayer()
{
	const FVector PlayerLocation = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetActorLocation();

	if (RunAwayLocations.Num() < 1) { return GetPawn()->GetActorLocation(); }

	FVector Location = RunAwayLocations[0];
	float Distance = 0;
	for (int i = 0; i < RunAwayLocations.Num(); ++i)
	{
		const float NewDistance = FVector::Distance(PlayerLocation, RunAwayLocations[i]);
		if (NewDistance > Distance)
		{
			Location = RunAwayLocations[i];
			Distance = NewDistance;
		}
	}

	return Location;
}

