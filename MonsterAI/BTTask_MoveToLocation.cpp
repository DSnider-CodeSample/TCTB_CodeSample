// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MoveToLocation.h"

#include "Monster.h"
#include "MonsterAIController.h"
#include "MonsterStates.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem/Public/NavigationPath.h"

EBTNodeResult::Type UBTTask_MoveToLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	//get the AIController of the monster
	AMonsterAIController* AIController = Cast<AMonsterAIController>(OwnerComp.GetAIOwner());

	if (AIController)
	{
		//set walk speed to desired
		const AMonster* Monster = Cast<AMonster>(AIController->GetPawn());
		if (Monster)
		{

			if (OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Enum>(AIController->StateKeyID) == EGurneyMonsterStates::GMS_GoToPlayer || OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Enum>(AIController->StateKeyID) == EGurneyMonsterStates::GMS_Pursue)
			{
				Cast<UCharacterMovementComponent>(Monster->GetMovementComponent())->MaxWalkSpeed = AIController->GetDesiredRunSpeed();
			}
			else
			{
				Cast<UCharacterMovementComponent>(Monster->GetMovementComponent())->MaxWalkSpeed = AIController->GetDesiredWalkSpeed();
			}
		}

		if (OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Enum>(AIController->StateKeyID) != EGurneyMonsterStates::GMS_GoToPlayer && OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Enum>(AIController->StateKeyID) != EGurneyMonsterStates::GMS_Pursue)
		{
			//ensure there is a path to the point being told to move to
			const FVector MonsterLocation = AIController->GetPawn()->GetActorLocation();
			const FVector TargetLocation = OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID);

			const UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), MonsterLocation, TargetLocation, NULL);
			const bool bValid = NavPath != nullptr && NavPath->IsValid() && !NavPath->IsPartial();
			//if there is no point, instead move to the closest runaway location
			if (!bValid)
			{
				OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID, AIController->GetClosestRunawayLocation());
			}
		}
		

		//move to the location stored in "TargetLocation"
		AIController->MoveToLocation(OwnerComp.GetBlackboardComponent()->GetValue<UBlackboardKeyType_Vector>(AIController->TargetLocationKeyID), 5.f, true, true, false, true, 0, true);
	}
	
	//return from this task as successful
	return EBTNodeResult::Succeeded;
}
