/*
 * Copyright (c) 2026 DLR e.V.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 */


#include "EsminiTester.h"

#include "EngineUtils.h"

// Sets default values
AEsminiTester::AEsminiTester()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEsminiTester::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AEsminiTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bRegistered) {
		return;
	}

	if (!IsValid(EsminiManager)) {
		
		for (TActorIterator<AEsminiManager> It(GetWorld()); It; ++It) {
			EsminiManager = *It;
			break;
		}
	}
	
	if (!IsValid(EsminiManager)) {
		return;
	}
	
	const FString Inputs = "[ue_esmini_feature]";
	FDataTarget Dt;
	Dt.Actor = this;
	ConnectInputs(&Inputs, EsminiManager->GemstarSimulationSubsystem, Dt, GemstarInterfaceType::VRUUPDATE);
	ConnectInputs(&Inputs, EsminiManager->GemstarSimulationSubsystem, Dt, GemstarInterfaceType::WEATHERDATA);
	ConnectInputs(&Inputs, EsminiManager->GemstarSimulationSubsystem, Dt, GemstarInterfaceType::TRAFFICUPDATE);

	bRegistered = true;

	UE_LOG(Esmini, Log, TEXT("Esmini Tester: observing %s"), *EsminiManager->GetName());
}

void AEsminiTester::VehicleDataUpdate_Implementation(FVehicleDataUpdateData& Data) {
	UE_LOG(Esmini, Log,
		TEXT("VehicleDataUpdateData { Provider: %s, Name: %s, Position: %s, Speed: %.2f, Accel: %.2f, Signals: %d, VehicleClass: %s, BoundingBox: %s }"),
		*Data.Provider.ToString(),
		*Data.Name.ToString(),
		*Data.Position.ToString(),
		Data.speed,
		Data.accel,
		Data.signals,
		*Data.VehicleClass.ToString(),
		*Data.BoundingBox.ToString()
	);
}


void AEsminiTester::WeatherDataUpdate_Implementation(FWeatherData& Data, int32 TargetIndex) {
	UE_LOG(Esmini, Log,
		TEXT("WeatherData { ID: %s, Enabled: %d, Type: %s, MOR: %.2f, TargetIndex: %d }"),
		*Data.ID.ToString(),
		Data.WeatherState.bWeatherEffectEnabled,
		*Data.WeatherState.ActiveWeatherType.ToString(),
		Data.WeatherState.MOR,
		TargetIndex
	);
}

void AEsminiTester::VRUDataUpdate_Implementation(FVRUDataUpdateData& Data) {
	UE_LOG(Esmini, Log,
		TEXT("VRUData {Provider: %s, Name: %s, Position: %s, BoundingBox: %s, Speed: %f}"),
		*Data.Provider.ToString(),
		*Data.Name.ToString(),
		*Data.Position.ToString(),
		*Data.BoundingBox.ToString(),
		Data.speed
		);
}
