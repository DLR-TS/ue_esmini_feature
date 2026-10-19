/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GemstarSimulationSubsystem.h"
#include "GemstarManager.h"
#include "EsminiManager.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Movement/VehicleDataUpdate.h"
#include "Interfaces/Movement/VRUDataUpdate.h"
#include "Interfaces/Weather/WeatherDataUpdate.h"

#include "EsminiTester.generated.h"

UCLASS()
class AEsminiTester : public AActor, public IVehicleDataUpdate, public IVRUDataUpdate, public IWeatherDataUpdate, public IGemstarManager
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
	TObjectPtr<AEsminiManager> EsminiManager;

protected:
	virtual void VehicleDataUpdate_Implementation(FVehicleDataUpdateData& data) override;
	virtual void VRUDataUpdate_Implementation(FVRUDataUpdateData& data) override;
	virtual void WeatherDataUpdate_Implementation(FWeatherData& data, int32 targetIndex) override;

private:
	bool bRegistered = false;
	
};
