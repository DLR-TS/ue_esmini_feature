// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GEMSTARRoot.h"
#include "EsminiManager.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GEMSTAR_ESMINI_Loader.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE_ESMINI_FEATURERUNTIME_API UGEMSTAR_ESMINI_Loader : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGEMSTAR_ESMINI_Loader();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
