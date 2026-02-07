// Fill out your copyright notice in the Description page of Project Settings.

#include "BTService_CheckWanderStatus.h"
#include <Kismet/GameplayStatics.h>

#include "MonsterAIController.h"
#include "NavigationSystem.h"
#include "NavigationSystem/Public/NavigationPath.h"
#include "PlayerCharacterComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Character.h"

UBTService_CheckWanderStatus::UBTService_CheckWanderStatus()
{
	bCreateNodeInstance = true;
}

void UBTService_CheckWanderStatus::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	//get the AIController associated with this behavior tree
	AMonsterAIController* AIController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	//increment time since pursue
	const float TimeSincePursue = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Float>(AIController->TimeSincePursueKeyID);
	if (TimeSincePursue >= 0)
	{
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->TimeSincePursueKeyID, TimeSincePursue + DeltaSeconds);
	}
	   
	//get the wander radius
	const float WanderRadius = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Float>(AIController->WanderRadiusKeyID);

	//increment wander radius
	if (WanderRadius < AIController->GetDesiredWanderRadius())
	{
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->WanderRadiusKeyID, WanderRadius + (DeltaSeconds * 3));
	}

	//get distance to current target point
	const float Distance = FVector::Distance(AIController->GetPawn()->GetActorLocation(), OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID));
	
	//if within a tolerable radius of the target location, choose a new target
	if (Distance < 150)
	{
		//create vectors to use in the GetRandomReachablePointInRadius
		const FVector MonsterLocation = AIController->GetPawn()->GetActorLocation();

		//set wanderCenter to monsterLocation by default
		FVector WanderCenter = MonsterLocation;

		//define a vector to be updated later
		FVector RandomPoint(1, 1, 1);

		//get the current nav system
		const UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

		const UPlayerCharacterComponent* Player = Cast<UPlayerCharacterComponent>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetComponentByClass(UPlayerCharacterComponent::StaticClass()));
		if (Player && Player->IsPlayerProtectedBySafetyVolume())
		{
			RandomPoint = AIController->GetFarthestRunawayLocationFromPlayer();
		} 
		else if (Player)
		{
			//determine player proximity to monster
			const FVector PlayerLocation = Player->GetOwner()->GetActorLocation();
			const float DistanceToPlayer = (PlayerLocation - MonsterLocation).Size();
			const float WanderBiasStartRadius = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Float>(AIController->WanderBiasStartRadiusKeyID);
			
			//if player is far from monster, monster "cheats" in it's wander algorithm
			//monster does not "cheat" if player is in a safe zone 
			if (DistanceToPlayer >= WanderBiasStartRadius && !Player->IsPlayerProtectedBySafetyVolume())
			{
				WanderCenter = FMath::Lerp(MonsterLocation, PlayerLocation, .15f);
			}

			//in case monster gets close without you ever making a sound, this makes him leave eventually anyway
			//TODO: Magic number??
			if (DistanceToPlayer <= 1100 && TimeSincePursue < 0)
			{
				OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->TimeSincePursueKeyID, 1);
			}

			//determine how far the target point should be from the player
			bool bFound = false;
			if (TimeSincePursue >= 4) //4 is an old number and this will only ever happen out of the search algo and it'll be like ~30, but this still runs
			{
				RandomPoint = AIController->GetFarthestRunawayLocation();
				OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->TimeSincePursueKeyID, -1);
			}
			else
			{
				//this is what runs to get the wander point in all situations except the first wander out of goto/pursue->search->wander
				bFound = UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), WanderCenter, RandomPoint, WanderRadius, NavSys->MainNavData, nullptr);
			}

			//determine point validity
			const UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), MonsterLocation, RandomPoint, NULL);
			const bool bValid = NavPath != nullptr && NavPath->IsValid() && !NavPath->IsPartial();
			if (!bValid)
			{
				RandomPoint = AIController->GetClosestRunawayLocation();
			}
		}
		
		//set the target location to the newly chosen target point
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID, RandomPoint);
	}
}