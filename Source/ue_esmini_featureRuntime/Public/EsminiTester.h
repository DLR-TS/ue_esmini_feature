// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EsminiManager.h"
#include "GameFramework/Actor.h"
#include "VehicleDataUpdate.h"
#include "EsminiTester.generated.h"

UCLASS()
class AEsminiTester : public AActor, public IVehicleDataUpdate
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEsminiTester();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "GEMSTAR")
	AEsminiManager* esminiManager;

protected:
	virtual void VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) override;
};
