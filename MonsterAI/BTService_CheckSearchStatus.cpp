// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_CheckSearchStatus.h"

#include "Monster.h"
#include "MonsterAIController.h"
#include "MonsterStates.h"
#include "NavigationSystem/Public/NavigationPath.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTService_CheckSearchStatus::UBTService_CheckSearchStatus()
{
	bCreateNodeInstance = true;
}

void UBTService_CheckSearchStatus::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	//get the AIController associated with this behavior tree
	AMonsterAIController* AIController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	//increment time since pursue
	const float TimeSincePursue = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Float>(AIController->TimeSincePursueKeyID);
	OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->TimeSincePursueKeyID, TimeSincePursue + DeltaSeconds);

	//after an amount of time searching, switch to wandering
	if (TimeSincePursue >= AIController->GetDesiredSearchDuration())
	{
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Enum>(AIController->StateKeyID, EGurneyMonsterStates::GMS_Wander);
		// play transition sound
		AMonster* Monster = Cast<AMonster>(AIController->GetPawn());
		Monster->PlayMonsterSound("PlayMonsterIdleLoop");

		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Float>(AIController->WanderRadiusKeyID, AIController->GetDesiredWanderRadius());
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID, AIController->GetPawn()->GetActorLocation());
		return;
	}

	//get the wander radius
	const float WanderRadius = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Float>(AIController->WanderRadiusKeyID);

	//get distance to current target point
	const float Distance = FVector::Distance(AIController->GetPawn()->GetActorLocation(), OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID));

	//if within a tolerable radius of the target location, choose a new target
	if (Distance < 150)
	{
		//create vectors to use in the GetRandomReachablePointInRadius
		const FVector MonsterLocation = AIController->GetPawn()->GetActorLocation();

		//search around the search focus area
		const FVector WanderCenter = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(AIController->SearchCenterPointKeyID);

		//define a vector to be updated later
		FVector RandomPoint(1, 1, 1);

		//get the current nav system
		const UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

		//find a valid point in range that can be navigated to
		bool bFound = UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), WanderCenter, RandomPoint, WanderRadius, NavSys->MainNavData, nullptr);

		//determine if there is a valid path to the chose point
		const UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), MonsterLocation, RandomPoint, NULL);
		bool bValid = NavPath != nullptr && NavPath->IsValid() && !NavPath->IsPartial();
		if (!bValid)
		{
			//if the search around the target point failed, try to search around self
			bFound = UNavigationSystemV1::K2_GetRandomReachablePointInRadius(GetWorld(), MonsterLocation, RandomPoint, WanderRadius, NavSys->MainNavData, nullptr);

			//check for point validity
			NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), MonsterLocation, RandomPoint, NULL);
			bValid = NavPath != nullptr && NavPath->IsValid() && !NavPath->IsPartial();
			if (!bValid)
			{
				//last reseort: chose a runaway location
				RandomPoint = AIController->GetClosestRunawayLocation();
			}
		}

		//set the target location to the newly chosen target point
		OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID, RandomPoint);
	}
}
